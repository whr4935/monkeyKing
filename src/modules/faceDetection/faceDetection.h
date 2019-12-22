#ifndef _FACE_DETECTION_H
#define _FACE_DETECTION_H

#include <stdint.h>
#include <list>
#include <vector>
#include <string>
#include <mutex>
#include "transfer.h"

class GenderClassifier;
class AgeClassifier;

class FaceDetectionListener
{
public:
    static const int RESULT_SIZE = 6;
    static const int MALE_INDEX = 0;
    static const int FEMALE_INDEX = 1;

    virtual ~FaceDetectionListener() {};

    virtual int onFrameSize(int width, int height) = 0;
    virtual int onUpdateFacePosition(const std::vector<int>& result) = 0;
    virtual int onMessage(const std::string& msg) {return 0;}; 
};

#define NO_PTS_VALUE ((int64_t)0x8000000000000000ull)

class DetectorBase;

class FaceDetection
{
public:

    enum PixelFormat {
        PIXEL_FORMAT_RGBA,
        PIXEL_FORMAT_NV21,
    };


    enum {
        kMaxDetectedQueueDepth = 1,
        kDetectRate = 4,
        kTransferTickMax = 1,
    };


    struct Frame {
        void reset()
        {
            mData = NULL;
            mTimeStamp = NO_PTS_VALUE;
            cookie = NULL;
        }

        uint8_t *mData;
        int64_t mTimeStamp;
        void *cookie;
    };

    struct FrameListener {
        virtual void onFrameReleased(void *cookie) = 0;
    };

public:
    FaceDetection(int width, int height, PixelFormat pix_fmt, FrameListener *frameListener = NULL);
    ~FaceDetection();

    int start();
    int stop();

    static void setModelsDir(const char* dir);
    void enableGenderClassifier(bool enable);
    void enableAgeClassifier(bool enable);

    bool isBusy() {return 0 == __sync_fetch_and_or(&mSem, 0);}
    int sendFrame(uint8_t *data, int64_t timestamp, void *cookie);
    void setListener(FaceDetectionListener *listener);
    void setDetectRate(int rate);

    void setTrain(bool train, std::string label);

    //getter
    int width() const {return mWidth;}
    int height() const {return mHeight;}
    int pixelFormat() const {return mPixelFormat;}

private:
    FaceDetection(const FaceDetection &);
    FaceDetection& operator=(const FaceDetection &);

    Frame *acqurireFrame();
    void releaseFrame(Frame *);
    void recycleFrame(Frame *, FrameListener*);

    static void *wrapper_thread(void *arg);
    void threadLoop();
    void doExitLoop();
    static int64_t getNowUs();
    static std::string timeStampToHHMMSS(int64_t timestampUs);
    static std::string getPackageName();

    bool mStarted;
    int mWidth;
    int mHeight;
    int mPixelFormat;
    std::vector<FaceDetectionListener*> mListener;
    FrameListener *mFrameListener;
    int mDetectRate;
    double mActualDetectRate;

    pthread_t mWorkThreadId;
    pthread_mutex_t mLock;
    pthread_cond_t mFrameCond;
    bool mRequestExit;
    std::list<Frame *> mFrames;

    pthread_mutex_t mPoolLock;
    std::list<Frame *> mFramePool;

    int mSem;
    DetectorBase *mDetector;

    class DetectResultFilter;
    DetectResultFilter *mFilter;

    static char* sModelsDir;

    GenderClassifier *mGenderClassifiler;
    bool mGCEnabled;

    AgeClassifier *mAgeClassifier;
    bool mACEnabled;

    Transfer *mTransfer = nullptr;

    std::recursive_mutex mMemberLock;
    std::string mReceivedMessage;
    std::string mReceivedLabel;

    bool mTrain = false;
    std::map<std::string, int> mLabelMap;
    std::map<int, std::string> mIdentityMap;
    int mCurIdentity = -1;

    int mTransferTick = 0;

    void restoreLabel(std::string labels);

};

#endif


