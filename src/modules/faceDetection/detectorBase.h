#ifndef _DETECTOR_BASE_H
#define _DETECTOR_BASE_H

#include "faceDetection.h"
#ifdef USE_OPENCV
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#endif
#include <vector>

class DetectorBase
{
public:
    enum DetectorType {
        DETECTOR_CascadeDetector,
    };

    static DetectorBase *createDetector(FaceDetection *faceDetection,
                                        DetectorType type = DETECTOR_CascadeDetector);

public:
    typedef FaceDetection::Frame Frame;

    virtual ~DetectorBase();

    virtual int start() = 0;
    virtual int stop() = 0;
#ifdef USE_OPENCV
    virtual bool detect(const Frame* f, std::vector<cv::Rect>&, cv::Mat &) = 0;
#endif

protected:
    DetectorBase(FaceDetection *faceDetection);

    FaceDetection *mFaceDetection;
};

#endif
