#include <android/log.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "cascadeDetector.h"
#include "haarcascade_frontalface_alt.hpp"

#define LOG_NDEBUG 0
#define LOG_TAG "cascadeDetector"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

#ifdef USE_OPENCV
using namespace cv;
#endif

// -----------------------------------------------
CascadeDetector::CascadeDetector(FaceDetection* faceDetection)
    : DetectorBase(faceDetection)
    , mStarted(false)
#ifdef USE_OPENCV
    , mCascadeDetector(NULL)
#endif
    , mAbsoluteFaceSize(0)
    , mRelativeFaceSize(0.2)
{

}

CascadeDetector::~CascadeDetector()
{
    if (mStarted) {
        stop();
    }
}

int CascadeDetector::start()
{
    if (mStarted)
        return 0;

    if (mAbsoluteFaceSize == 0) {
        mAbsoluteFaceSize = static_cast<int>(mFaceDetection->height() * mRelativeFaceSize + 0.5);
        ALOGI("mAbsoluteFaceSize:%d", mAbsoluteFaceSize);
    }

#ifdef USE_OPENCV
    mCascadeDetector = new cv::CascadeClassifier();
    if (mCascadeDetector == NULL){
        ALOGI("create CascadeClassifier failed!");
        return -1;
    }

    if (!mCascadeDetector->load(getClassifierPath())) {
        ALOGI("load classifier file failed:%s", getClassifierPath().c_str());
        return -1;
    }
#endif

    mStarted = true;

    return 0;
}

int CascadeDetector::stop()
{
    if (!mStarted)
        return 0;

#ifdef USE_OPENCV
    if (mCascadeDetector) {
        delete mCascadeDetector;
        mCascadeDetector = NULL;
    }
#endif

    mAbsoluteFaceSize = 0;

    mStarted = false;

    return 0;
}

#ifdef USE_OPENCV
bool CascadeDetector::detect(const Frame* f, std::vector<cv::Rect>&faces, cv::Mat &frame_out)
{
    struct timespec beg_time, end_time;
    cv::Mat frame_gray;

    clock_gettime(CLOCK_MONOTONIC, &beg_time);

    Mat image;
    switch (mFaceDetection->pixelFormat()) {
    case FaceDetection::PIXEL_FORMAT_NV21:
        image = Mat(mFaceDetection->height() * 3 / 2, mFaceDetection->width(), CV_8UC1, f->mData);
        // equalizeHist(frame_gray, frame_gray);
        // cv::flip(image, image, 0);
        cvtColor(image, frame_out, CV_YUV2BGR_NV21);
        cvtColor(image, frame_gray, CV_YUV2GRAY_NV21);
        break;

    case FaceDetection::PIXEL_FORMAT_RGBA:
        image = Mat(mFaceDetection->height(),  mFaceDetection->width(), CV_8UC4, f->mData);
        cvtColor(image, frame_out, CV_RGBA2BGR);
        cvtColor(image, frame_gray, CV_RGBA2GRAY);
        break;

    default:
        break;
    }

    //-- Detect faces
    mCascadeDetector->detectMultiScale(frame_gray, faces, 1.1, 2,
                                0 | CASCADE_SCALE_IMAGE, Size(mAbsoluteFaceSize, mAbsoluteFaceSize));

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int64_t cost_time = (end_time.tv_sec - beg_time.tv_sec) * 1000000 +
                        (end_time.tv_nsec - beg_time.tv_nsec) / 1000;

    // ALOGI("detect cost time:%.3f ms", cost_time/1e3);

    return faces.size() > 0;
}
#endif

std::string CascadeDetector::getPackageName()
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

std::string CascadeDetector::getClassifierPath()
{
    char path[200];

    snprintf(path, sizeof(path),
             "/data/data/%s/haarcascade_frontalface_alt.xml",
             getPackageName().c_str());

    if (access(path, F_OK) < 0) {
        if (!tryCreateClassifierFile(path)) {
            ALOGI("getClassifierPath failed!");
            memset(path, 0, sizeof(path));
        }
    }

    ALOGI("classifier path:%s", path);
    return path;
}

bool CascadeDetector::tryCreateClassifierFile(std::string path)
{
    int fd;

    fd = open(path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (fd < 0) {
        ALOGI("open %s failed:%s", path.c_str(), strerror(errno));
        return false;
    }

    int left_size = sizeof(haarcascade_frontalface_alt) / sizeof(*haarcascade_frontalface_alt);
    int write_size = 0;
    int size;
    while  (left_size > 0) {
        size = size > left_size ? 4096 : left_size;
        size = write(fd, haarcascade_frontalface_alt+write_size, size);
        if (size < 0) {
            if (errno == EINTR)
                size = 0;
            else {
                ALOGI("write %s error:%s", path.c_str(), strerror(errno));
                close(fd);
                unlink(path.c_str());
                return false;
            }
        }
        left_size -= size;
        write_size += size;
    }

    fsync(fd);
    close(fd);

    return true;
}
