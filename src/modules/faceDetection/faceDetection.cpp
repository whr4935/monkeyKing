#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#endif

#include "faceDetection.h"
#include "detectorBase.h"

#include <genderclassifier.hpp>
#include <ageclassifier.hpp>

#define LOG_NDEBUG 0
#define LOG_TAG "faceDetecton"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

#ifdef USE_OPENCV
using namespace cv;
#endif
using std::string;


static const cv::Size g_faceSize(160, 160);

// -----------------------------------------------
class FaceDetection::DetectResultFilter
{
public:
    enum {
        kHasFaceThresHold = 1,
        kFacePositionThresHold = 1,
        kDelta = 5,
    };

    DetectResultFilter();
    ~DetectResultFilter();

#ifdef USE_OPENCV
    void filterResult(const std::vector<Rect>& faces);
#endif
    bool hasResult() const;
    const std::vector<int> result();
#ifdef USE_OPENCV
    const std::vector<Rect>& getResult();
#endif

private:
#ifdef USE_OPENCV
    std::vector<Rect> mResult;
    std::vector<Rect> mLastResult;
#endif

    int mHasFaceCount;
    int mFacePositionCount;

    bool mHasFace;
    bool mResultUpdated;
    bool mShouldUpdateResult;
};

FaceDetection::DetectResultFilter::DetectResultFilter()
    : mHasFaceCount(0)
    , mFacePositionCount(0)
    , mHasFace(false)
    , mResultUpdated(false)
    , mShouldUpdateResult(false)
{

}

FaceDetection::DetectResultFilter::~DetectResultFilter()
{

}


#ifdef USE_OPENCV
void FaceDetection::DetectResultFilter::filterResult(const std::vector<Rect>& faces)
{
    mLastResult = mResult;
    mResult = faces;

    if (mLastResult.size() != mResult.size()) {
        mHasFaceCount = 0;
        mResultUpdated = false;
        mHasFace = false;
    }

    if (mHasFaceCount < kHasFaceThresHold) {
        ++mHasFaceCount;
    } else {
        if (!mResultUpdated) {
            mShouldUpdateResult = true;
        }

        if (mResult.size() > 0)
            mHasFace = true;
    }

    if (mHasFace && mLastResult.size()==mResult.size()) {
        for (int i = 0; i != mResult.size(); ++i) {
            if (abs(mResult[i].x - mLastResult[i].x) >= kDelta ||
                abs(mResult[i].y - mLastResult[i].y) >= kDelta ||
                abs(mResult[i].width - mLastResult[i].width) >= kDelta ||
                abs(mResult[i].height - mLastResult[i].height) >= kDelta) {
                mFacePositionCount = 0;
                mResultUpdated = false;
            } else {
                mResult[i] = mLastResult[i];
            }
        }

        if (mFacePositionCount < kFacePositionThresHold) {
            ++mFacePositionCount;
        } else {
            if (!mResultUpdated) {
                mShouldUpdateResult = true;
            }
        }
    }
}
#endif

inline bool FaceDetection::DetectResultFilter::hasResult() const
{
    return mShouldUpdateResult;
}

inline const std::vector<int> FaceDetection::DetectResultFilter::result()
{
    std::vector<int> result;

#ifdef USE_OPENCV
    for (int i = 0; i < mResult.size(); ++i) {
       result.push_back(mResult[i].x);
       result.push_back(mResult[i].y);
       result.push_back(mResult[i].width);
       result.push_back(mResult[i].height);
    }

    mShouldUpdateResult = false;
    mResultUpdated = true;
#endif

    return result;
}

#ifdef USE_OPENCV
const std::vector<Rect>& FaceDetection::DetectResultFilter::getResult() {
    mShouldUpdateResult = false;
    mResultUpdated = true;

    return mResult;

}
#endif

// -----------------------------------------------
char* FaceDetection::sModelsDir = NULL;

FaceDetection::FaceDetection(int width, int height, PixelFormat pix_fmt, FrameListener *frameListener)
    : mStarted(false),
      mWidth(width),
      mHeight(height),
      mPixelFormat(pix_fmt),
      mFrameListener(frameListener),
      mDetectRate(kDetectRate),
      mActualDetectRate(0),
      mWorkThreadId(-1),
      mRequestExit(false),
      mSem(kMaxDetectedQueueDepth),
      mFilter(new DetectResultFilter),
      mGenderClassifiler(NULL),
      mGCEnabled(false),
      mAgeClassifier(NULL),
      mACEnabled(false)
{
    mDetector = DetectorBase::createDetector(this);
    mTransfer = new Transfer();
    mTransfer->onMessage([this](char* data, size_t length, bool isText) {
            if (isText) {
                std::lock_guard<std::recursive_mutex> _l(mMemberLock);

                string result = std::string(data, length);
                if (result[0]=='L'&&result[1]==':') {
                    restoreLabel(result);
                    return;
                }

                if (result.compare(0, 5, "Attr:")==0) {
                    std::stringstream ss(result);
                    std::string s;
                    std::string gender, age;

                    int cnt = 0;
                    while (ss) {
                        std::getline(ss, s, ':');
                        switch (cnt++) {
                        case 1:
                            gender = s;
                            break;

                        case 2:
                            age = s;
                            break;

                        default:
                            break;
                        }
                    }

                    mReceivedMessage = mReceivedLabel;
                    if (gender.size() > 0) {
                        mReceivedMessage += ", " + gender;
                    }
                    if (age.size() > 0) {
                        mReceivedMessage += ", " + age;
                    }
                } else {
                    int id = atoi(result.c_str());
                    if (mIdentityMap.find(id) != mIdentityMap.end()) {
                        mReceivedLabel = mIdentityMap[id];
                    } else {
                        if (id == -1) {
                            mReceivedLabel = "Unknown";
                        } else {
                            mReceivedLabel = "Error";
                        }
                    }
                    mReceivedMessage = mReceivedLabel;
                }

                ALOGI("received WebSocket: id=%s, result = %s", result.c_str(), mReceivedMessage.c_str());
            } });

    mTransfer->connectAsync();

    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mFrameCond, NULL);
    pthread_mutex_init(&mPoolLock, NULL);
}

FaceDetection::~FaceDetection()
{
    if (mTransfer) {
        delete mTransfer;
        mTransfer = nullptr;
    }

    if (mStarted) {
        stop();
    }

    if (mFrameListener) {
        delete mFrameListener;
        mFrameListener = NULL;
    }

    for (auto p : mListener) {
        delete p;
    }

    if (mFilter) {
        delete mFilter;
        mFilter = NULL;
    }

    if (mDetector) {
        delete mDetector;
        mDetector = NULL;
    }

    if (mGenderClassifiler) {
        delete mGenderClassifiler;
        mGenderClassifiler = NULL;
    }

    if (mAgeClassifier) {
        delete mAgeClassifier;
        mAgeClassifier = NULL;
    }

    pthread_mutex_destroy(&mLock);
    pthread_mutex_destroy(&mPoolLock);
    pthread_cond_destroy(&mFrameCond);

    ALOGI("dtor FaceDetection");
}

int FaceDetection::start()
{
    int err = -1;

    if (mStarted) {
        ALOGI("FaceDetection already started!");
        return 0;
    }

    err = mDetector->start();
    if (err < 0) {
        ALOGE("mDetector start failed!");
        return -1;
    }

    ALOGI("starting faceDetection...");
    err = pthread_create(&mWorkThreadId, NULL, wrapper_thread, this);
    if (err != 0) {
        ALOGE("create work_thread failed! err= %d", err);

        return -1;
    }

    mStarted = true;
    ALOGI("faceDetection started!");
    return 0;
}

int FaceDetection::stop()
{
    if (!mStarted)
        return 0;

    ALOGI("stoping faceDetection...");
    pthread_mutex_lock(&mLock);
    mRequestExit = true;
    pthread_mutex_unlock(&mLock);
    pthread_cond_signal(&mFrameCond);
    pthread_join(mWorkThreadId, NULL);

    mDetector->stop();

    mStarted = false;
    ALOGI("faceDetection stoped!");
    return 0;
}

// -----------------------------------------------
void FaceDetection::setModelsDir(const char* dir)
{
    if (sModelsDir) {
        free(sModelsDir);
        sModelsDir = NULL;
    }

    sModelsDir = strdup(dir);
}

void FaceDetection::enableGenderClassifier(bool enable) {
    pthread_mutex_lock(&mLock);
    mGCEnabled = enable;
    pthread_mutex_unlock(&mLock);
}
void FaceDetection::enableAgeClassifier(bool enable) {
    pthread_mutex_lock(&mLock);
    mACEnabled = enable;
    pthread_mutex_unlock(&mLock);
}

int FaceDetection::sendFrame(uint8_t *data, int64_t timestamp, void *cookie)
{
    int sem = __sync_fetch_and_or(&mSem, 0);
    if (!mStarted || sem ==0) {
        if (mFrameListener) {
            mFrameListener->onFrameReleased(cookie);
        }
        return 0;
    }

    Frame *f = acqurireFrame();
    f->mData = data;
    f->mTimeStamp = timestamp;
    f->cookie = cookie;

    pthread_mutex_lock(&mLock);
    mFrames.push_back(f);
    pthread_mutex_unlock(&mLock);
    pthread_cond_signal(&mFrameCond);

    __sync_sub_and_fetch(&mSem, 1);

    return 0;
}

void FaceDetection::setListener(FaceDetectionListener *listener)
{
    if (mStarted) {
        ALOGE("we have started, can not set listener now!");
        return;
    }

    if (listener != nullptr)
        mListener.push_back(listener);
}

void *FaceDetection::wrapper_thread(void *arg)
{
    FaceDetection *d = static_cast<FaceDetection *>(arg);

    d->threadLoop();

    return (void *)0;
}

void FaceDetection::threadLoop()
{
    for (auto p : mListener) {
        p->onFrameSize(mWidth, mHeight);
    }

#ifdef USE_OPENCV
    std::vector<Rect> faces;
#endif

    int64_t beginTime = getNowUs();
    int64_t currentTime;
    int64_t diffTime;
    int64_t expectedTime = 1000000LL / mDetectRate;

    for (;;) {
        pthread_mutex_lock(&mLock);
        while (!mRequestExit && mFrames.empty()) {
            pthread_cond_wait(&mFrameCond, &mLock);
        }
        if (mRequestExit) {
            mRequestExit = false;
            doExitLoop();
            pthread_mutex_unlock(&mLock);
            break;
        }

        Frame *f = *mFrames.begin();
        mFrames.pop_front();

#ifdef USE_OPENCV
        if (mGCEnabled) {
            if (!mGenderClassifiler) {
                std::string dir(sModelsDir);
                dir.append("/fisher64eq.yml");
                mGenderClassifiler = new GenderClassifier(dir);
            }
        } else {
            if (mGenderClassifiler != NULL) {
                delete mGenderClassifiler;
                mGenderClassifiler = NULL;
            }
        }

        if (mACEnabled) {
            if (!mAgeClassifier) {
                std::string dir(sModelsDir);
                dir.append("/age_cropped.yml");
                mAgeClassifier = new AgeClassifier(dir);
            }
        } else {
            if (mAgeClassifier != NULL) {
                delete mAgeClassifier;
                mAgeClassifier = NULL;
            }
        }
#endif

        pthread_mutex_unlock(&mLock);

        // do some work here
        // ALOGI("get a frame:%p, timestamp:%lld, cookie:%p", f->mData, f->mTimeStamp, f->cookie);

#ifdef USE_OPENCV
        if (mDetector) {
            {
                std::lock_guard<std::recursive_mutex> _l(mMemberLock);

                if (mReceivedMessage.size() != 0) {
                    for (auto p : this->mListener) {
                        p->onMessage(mReceivedMessage);
                    }
                    mTransferTick = 0; //enable transfer
                }
                mReceivedMessage.clear();
            }

            cv::Mat gray;
            mDetector->detect(f, faces, gray);
            mFilter->filterResult(faces);
            if (mFilter->hasResult()) {
                std::vector<int> msg;
                const std::vector<Rect>& result = mFilter->getResult();
                for (int i = 0; i < result.size(); ++i) {
                    double confidence;
                    int gender = -1;
                    // if (mGenderClassifiler) {
                        // cv::Mat face = gray(result[i]);
                        // gender = mGenderClassifiler->classify(face, confidence);
                    // }
                    int age = -1;
/*                     if (mAgeClassifier) { */
                        // cv::Mat face = gray(result[i]);
                        // age = mAgeClassifier->classify(face, confidence);
                    // }

                    msg.push_back(result[i].x);
                    msg.push_back(result[i].y);
                    msg.push_back(result[i].width);
                    msg.push_back(result[i].height);
                    msg.push_back(gender);
                    msg.push_back(age);
                }

                for (auto p : mListener) {
                    p->onUpdateFacePosition(msg);
                }

                {
                    int cnt = 0;
                    for (const cv::Rect& r : faces) {
                        cv::Rect r2 = r;
                        int expand_width =  r2.width/5;
                        int expand_height = r2.height/5;
                        r2.x -= expand_width;           if (r2.x < 0) r2.x = 0;
                        r2.y -= expand_height;          if (r2.y < 0) r2.y = 0;
                        r2.width += 2*expand_width;     if (r2.width  + r2.x >= mWidth)  r2.width  = mWidth  - r2.x;
                        r2.height += 2*expand_height;   if (r2.height + r2.y >= mHeight) r2.height = mHeight - r2.y;
                        cv::Mat faceImage = gray(r2).clone();

                        // cv::resize(faceImage, faceImage, g_faceSize);

                        // std::string filename = "/data/data/";
                        std::string filename;
                        // filename += getPackageName();
                        // filename += "/";
                        // if (f->mTimeStamp == 0ll) {
                            f->mTimeStamp = time(NULL);
                        // }
                        filename += timeStampToHHMMSS(f->mTimeStamp);
                        // if (cnt > 0) filename += std::to_string(cnt);
                        filename += ".jpg";
                        ALOGI("%s", filename.c_str());
                        // cv::imwrite(filename, faceImage);
                        std::vector<uchar> jpgImage;
                        cv::imencode(".jpg", faceImage, jpgImage);
                        if (mTransfer) {
                            if (mTransferTick >=0)
                                ++mTransferTick;
                            if (mTransferTick >= kTransferTickMax) {
                                mTransferTick = 0;

                                // ALOGI("filename:%s", filename.c_str());
                                mTransfer->send(filename, jpgImage.data(), jpgImage.size(), mTrain, mCurIdentity);
                                mTransferTick = -1; //disable transer
                            }
                        }

                        break;
                        ++cnt;
                    }
                }
            }
        }
#endif

        currentTime = getNowUs();
        diffTime = currentTime - beginTime;
        if (diffTime < expectedTime) {
            usleep(expectedTime - diffTime);
            currentTime = getNowUs();
            diffTime = currentTime - beginTime;
        }
        mActualDetectRate = 1000000*1.0 / diffTime;
        // ALOGI("mActualDetectRate = %.2f", mActualDetectRate);
        beginTime = currentTime;

        __sync_add_and_fetch(&mSem, 1);

        // finished
        recycleFrame(f, mFrameListener);
    }
}

void FaceDetection::recycleFrame(Frame *f, FrameListener *listener)
{
    if (listener) {
        listener->onFrameReleased(f->cookie);
    }

    // ALOGI("release frame:%p", f->mData);
    releaseFrame(f);
}

void FaceDetection::doExitLoop()
{
    for (std::list<Frame *>::iterator it = mFrames.begin();
         it != mFrames.end(); ++it) {
        recycleFrame(*it, mFrameListener);
    }
    mSem = kMaxDetectedQueueDepth;

    pthread_mutex_lock(&mPoolLock);
    for (std::list<Frame *>::iterator it = mFramePool.begin();
         it != mFramePool.end(); ++it) {
        delete *it;
    }
    mFramePool.clear();
    pthread_mutex_unlock(&mPoolLock);
}

FaceDetection::Frame *FaceDetection::acqurireFrame()
{
    Frame *f = NULL;
    pthread_mutex_lock(&mPoolLock);
    if (!mFramePool.empty()) {
        f = *mFramePool.begin();
        mFramePool.pop_front();
    }
    pthread_mutex_unlock(&mPoolLock);

    if (f == NULL) {
        f = new Frame();
    }

    return f;
}

void FaceDetection::releaseFrame(Frame *f)
{
    pthread_mutex_lock(&mPoolLock);
    mFramePool.push_back(f);
    pthread_mutex_unlock(&mPoolLock);
}

int64_t FaceDetection::getNowUs()
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec*1000000 + t.tv_nsec/1000;
}

std::string FaceDetection::timeStampToHHMMSS(int64_t timestampUs)
{
    ALOGI("timestampUs:%lld", timestampUs);

    int sec = timestampUs;

    int hour = sec / 3600;
    int min = sec % 3600 / 60;
    sec %= 60;

    char buffer[20];
    sprintf(buffer, "%.2d_%.2d_%.2d", hour, min, sec);

    return buffer;
}

std::string FaceDetection::getPackageName()
{
    char path[100], buf[256];
    std::string pkgName;
    int fd = -1;

    snprintf(path, sizeof(path), "/proc/%d/cmdline", getpid());

    fd = open(path, O_RDONLY);
    if (fd < 0)
        goto exit;

    if (read(fd, buf, sizeof(buf)) < 0) {
        ALOGI("read package name failed!");
        goto exit;
    }

    pkgName = buf;

exit:
    if (fd >= 0)
        close(fd);

    return pkgName;
}

void FaceDetection::setTrain(bool train, std::string label)
{
    int identity = -1;

    if (train && label.size() != 0 && label != "") {
        auto it = mLabelMap.find(label);
        if (it != mLabelMap.end()) {
            identity = it->second;
        } else {
            identity = mLabelMap.size();
            mLabelMap.insert({label, identity});
            ALOGI("inset identity map...");
            mIdentityMap.insert({identity, label});
        }

        mTransfer->setLabel(mIdentityMap);
    }

    std::lock_guard<std::recursive_mutex> _l(mMemberLock);

    mCurIdentity = identity;

    if (label.size() != 0) {
        ALOGI("label:%s, identity:%d", label.c_str(), mCurIdentity);
    }

    mTrain = train;

    if (train) {
        mReceivedMessage = "Training...";
    }
}

void FaceDetection::restoreLabel(std::string labels)
{
    int pos, pos2;
    std::string label;
    int identity = -1;

    pos = labels.find_first_of(':');
    if (pos != 1 || labels[0] != 'L') {
        ALOGI("restore label failed!");
        return;
    }

    mLabelMap.clear();

    while (pos != std::string::npos) {
        pos2 = labels.find_first_of(':', pos+1);
        if (pos2 != std::string::npos)
            label = labels.substr(pos+1, pos2-1-pos);
        else
            label = labels.substr(pos+1);

        ALOGI("restore get label:%s", label.c_str());
        identity = mLabelMap.size();
        mLabelMap.insert({label, identity});
        mIdentityMap.insert({identity, label});

        pos = pos2;
    }
}



