#pragma once
#include<iostream>
#include <vector>
#include <zbar.h>
#include <Windows.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Detector.h"

using namespace std;
using namespace cv;
using namespace zbar;


class Detector {
public:
	typedef struct
	{
		string type;
		string data;
		vector <cv::Point> location;
	} DecodedObject;
	int DetectQR(HDC hdc, int width, int height, vector<DecodedObject>& decoded_objects);

	Detector();
	~Detector();
private:
	Mat DC2Mat(HDC hdc, int src_width, int src_height);
};
