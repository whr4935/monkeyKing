#ifndef AGECLASSIFIER_H
#define AGECLASSIFIER_H

#include <classifier.hpp>
#include <face_attributes.hpp>
#ifdef USE_OPENCV
//#include <opencv2/dnn.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <iostream>
#include <string>

#ifdef USE_OPENCV
using namespace cv;
#endif
//using namespace cv::dnn;
using namespace std;

class AgeClassifier: public Classifier
{
    string high_performances_model_path = "!!! INSERT HIGH PERFORMANCES MODEL PATH HERE !!!";
    string low_performances_model_path = "models/age/Age_cropped.yml";
#ifdef USE_OPENCV
    Ptr<FaceRecognizer> model;
#endif
    //Net net;
    bool high_performances;
    int imwidth;
    int imheight;

public:
    AgeClassifier(bool high_performances=false);
    AgeClassifier(string model_path);
#ifdef USE_OPENCV
    int classify(cv::Mat &face, double &confidence);
#endif
    void setModel(string);
};

#endif // AGECLASSIFIER_H
