#ifndef _CASCADE_DETECTOR_H
#define _CASCADE_DETECTOR_H

#include "detectorBase.h"

class CascadeDetector : public DetectorBase
{
public:
    CascadeDetector(FaceDetection *faceDetection);
    virtual ~CascadeDetector();

    virtual int start();
    virtual int stop();
#ifdef USE_OPENCV
    virtual bool detect(const Frame *f, std::vector<cv::Rect> &faces, cv::Mat &gray);
#endif

private:
    std::string getPackageName();
    std::string getClassifierPath();
    bool tryCreateClassifierFile(std::string);

    bool mStarted;
#ifdef USE_OPENCV
    cv::CascadeClassifier *mCascadeDetector;
#endif
    int mAbsoluteFaceSize;
    const double mRelativeFaceSize;
};


#endif
