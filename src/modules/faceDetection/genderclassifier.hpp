#ifndef GENDERCLASSIFIER_H
#define GENDERCLASSIFIER_H

#include <classifier.hpp>
#include <face_attributes.hpp>
#ifdef USE_OPENCV
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/dnn.hpp>
#endif
#include <iostream>
#include <string>

using namespace std;
#ifdef USE_OPENCV
using namespace cv;
#endif
//using namespace cv::dnn;

class GenderClassifier: public Classifier
{
public:
    GenderClassifier(bool high_performances=false);
    GenderClassifier(string model_path);
#ifdef USE_OPENCV
    int classify(cv::Mat &face, double &confidence);
#endif

private:
#ifdef USE_OPENCV
    Ptr<FaceRecognizer> model;
#endif
    string model_path = "models/gender/fisher64eq.yml";
    string deep_paths[2] = {"models/gender/gender.prototxt",
                            "models/gender/gender.caffemodel"};
    //Net net;
    bool high_performances;
    int imwidth;
    int imheight;
};

#endif // GENDERCLASSIFIER_H
