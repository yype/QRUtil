#pragma once
#include <Windows.h>
#include <zbar.h>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/wechat_qrcode.hpp>
#include <vector>
#include "Detector.h"

using namespace std;
using namespace cv;
using namespace zbar;

class Detector {
   public:
    struct DecodedObject {
        //string type;
        string data;
        vector<cv::Point> location;
        bool operator==(const DecodedObject& right) const {
            if (this->data == right.data) {
                for (int i = 0; i < 4; i++) {
                    if (abs(location[i].x - right.location[i].x) > 4) {
                        return false;
                    }
                    if (abs(location[i].y - right.location[i].y) > 4) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        };
    };
    int DetectQRZbar(HDC hdc, int width, int height, int thrd, vector<DecodedObject>& decoded_objects);

    // Although wechat detector can detect multiple targets, we only pick one.
    bool DetectQRWeChat(HDC hdc, int x1, int y1, int x2, int y2, std::string& result);

    Detector();
    ~Detector();

   private:
    Mat DC2Mat(HDC hdc, int src_width, int src_height);
};
