#include <android/log.h>

#include "detectorBase.h"
#include "cascadeDetector.h"


#define LOG_NDEBUG 0
#define LOG_TAG "detectorBase"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

DetectorBase::DetectorBase(FaceDetection *faceDetection)
    :mFaceDetection(faceDetection)
{

}

DetectorBase::~DetectorBase()
{

}

DetectorBase* DetectorBase::createDetector(FaceDetection *faceDetection, DetectorType type)
{
    DetectorBase *detector = NULL;

    switch (type) {
    case DETECTOR_CascadeDetector:
        detector = new CascadeDetector(faceDetection);
        break;

    default:
        ALOGE("unkown detector type: %d", type);
        break;
    }

    return detector;
}
