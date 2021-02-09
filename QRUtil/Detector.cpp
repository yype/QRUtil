#include "Detector.h"

int Detector::DetectQRZbar(HDC hdc, int width, int height, int thrd, vector<DecodedObject>& decoded_objects)
{



	Mat im = DC2Mat(hdc, width, height);
	// convert image to grayscale
	Mat imGray;
	cvtColor(im, imGray, cv::COLOR_RGBA2GRAY);
	//blur(imGray, imGray, Size(3, 3));
	equalizeHist(imGray, imGray);

	if (thrd) {
		threshold(imGray, imGray, thrd, 255, THRESH_BINARY);
	}
	else {
		cv::adaptiveThreshold(imGray, imGray, 255, ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 33, 0);
	}
	/*
	vector<Mat> points;

	auto info = detector.detectAndDecode(im, points);
	for (int i = 0; i < info.size();i++) {
		//string tmp;
		//tmp << points[i];
		//MessageBoxA(NULL, tmp.c_str(), "detected", MB_OK);

		auto mat = points[i];
		std::vector<cv::Point> location;
		for (int j = 0; j < 4; j++) {
			auto x = mat.at<float>(j, 0);
			auto y = mat.at<float>(j, 1);
			location.push_back(Point(x,y));
		}
		DecodedObject obj;
		obj.data = info[i];
		obj.location = location;

		bool found = false;
		for (auto& each : decoded_objects) {
			if (each == obj) {
				found = true;
			}
		}
		if (!found)
			decoded_objects.push_back(obj);
	}
	*/
		

	// create zbar scanner
	ImageScanner scanner;

	// configure scanner

	// disable all
	scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 0);
	// enable qr
	scanner.set_config(ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

	

	for (int round = 0; round < 2; round++) {
		

		switch (round) {
		case 0:
			
			break;
		case 1:
			// invert black & white
			imGray = ~imGray;
			break;
		default:
			exit(1);
		}

		// wrap image data in a zbar image
		Image image(im.cols, im.rows, "Y800", // 8 bit monochrome format 
			reinterpret_cast<uchar*>(imGray.data), im.cols * im.rows);

		// scan the image for barcodes and QRCodes
		scanner.scan(image);

		for (Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
			DecodedObject obj;

			// obj.type = symbol->get_type_name();
			obj.data = symbol->get_data();
			
			for (int i = 0; i < symbol->get_location_size(); i++) {
				Point tmp = Point(symbol->get_location_x(i), symbol->get_location_y(i));
				//cout << "Point " << i << " : " << tmp.x << ", " << tmp.y << endl;
				obj.location.push_back(tmp);
			}
			/*
			const Point* pts = reinterpret_cast<const cv::Point*>(Mat(obj.location).data);
			int npts = Mat(obj.location).rows;
			polylines(im, &pts, &npts, 1, true, Scalar(0, 0, 255), 2);

			cout << endl;
			*/

			bool found = false;
			for (auto& each : decoded_objects) {
				if (each == obj) {
					found = true;
				}
			}
			if (!found)
				decoded_objects.push_back(obj);
		}
	}
	

	return 0;
}

bool Detector::DetectQRWeChat(HDC hdc, int x1, int y1, int x2, int y2, std::string& result)
{
	if (x1 > x2) {
		auto tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y1 > y2) {
		auto tmp = y2;
		y2 = y1;
		y1 = tmp;
	}
	if ((x1 == x2) || (y1 == y2)) {
		return false;
	}
	auto width = x2 - x1, height = y2 - y1;
	HDC hMemDc = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, width, height);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDc, hBmp);
	BitBlt(hMemDc, 0, 0, x2 - x1, y2 - y1, hdc, x1, y1, SRCCOPY);
	Mat im = DC2Mat(hMemDc, width, height);
	// convert image to grayscale
	//Mat imGray;
	//cvtColor(im, imGray, cv::COLOR_RGBA2GRAY);

	vector<Mat> points;
	auto detector = wechat_qrcode::WeChatQRCode("E:\\opencv\\opencv\\newbuild\\downloads\\wechat_qrcode\\detect.prototxt",
		"E:\\opencv\\opencv\\newbuild\\downloads\\wechat_qrcode\\detect.caffemodel",
		"E:\\opencv\\opencv\\newbuild\\downloads\\wechat_qrcode\\sr.prototxt",
		"E:\\opencv\\opencv\\newbuild\\downloads\\wechat_qrcode\\sr.caffemodel");
	auto info = detector.detectAndDecode(im, points);
	if (info.size()) {
		result = string(info[0].c_str());
		return true;
	}

	return false;
}

Detector::Detector()
{
}

Detector::~Detector()
{
}

Mat Detector::DC2Mat(HDC hdc, int src_width, int src_height)
{
	HDC hwindowCompatibleDC;

	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;
	hwindowCompatibleDC = CreateCompatibleDC(hdc);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	src.create(src_height, src_width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hdc, src_width, src_height);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = src_width;
	bi.biHeight = -src_height;  // upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	BitBlt(hwindowCompatibleDC, 0, 0, src_width, src_height, hdc, 0, 0, SRCCOPY);
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, src_height, src.data,
		reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);  // copy from hwindowCompatibleDC to hbwindow

	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);

	return src;
}
