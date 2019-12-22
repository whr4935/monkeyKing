#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#ifdef USE_OPENCV
#include <opencv2/face.hpp>

using namespace cv::face;
#endif

class Classifier
{
    public:
#ifdef USE_OPENCV
        virtual int classify(cv::Mat& face, double &confidence) = 0;
#endif
};

#endif // CLASSIFIER_H
