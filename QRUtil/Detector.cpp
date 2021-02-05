#include "Detector.h"

int Detector::DetectQR(HDC hdc, int width, int height, vector<DecodedObject>& decoded_objects)
{
	Mat im = DC2Mat(hdc, width, height);
	// create zbar scanner
	ImageScanner scanner;

	// configure scanner
	scanner.set_config(ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

	// convert image to grayscale
	Mat imGray;

	for (int thrd = 5; thrd < 255; thrd+=50) {
		cvtColor(im, imGray, cv::COLOR_RGBA2GRAY);
		//blur(imGray, imGray, Size(3, 3));
		//equalizeHist(imGray, imGray);
		threshold(imGray, imGray, thrd, 255, THRESH_BINARY);
		//cv::adaptiveThreshold(imGray, imGray, 255, ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 33, 0);

		// wrap image data in a zbar image
		Image image(im.cols, im.rows, "Y800", // 8 bit monochrome format 
			reinterpret_cast<uchar*>(imGray.data), im.cols * im.rows);

		// scan the image for barcodes and QRCodes
		int n = scanner.scan(image);

		for (Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
			DecodedObject obj;

			obj.type = symbol->get_type_name();
			obj.data = symbol->get_data();

			for (int i = 0; i < symbol->get_location_size(); i++) {
				Point tmp = Point(symbol->get_location_x(i), symbol->get_location_y(i));
				cout << "Point " << i << " : " << tmp.x << ", " << tmp.y << endl;
				obj.location.push_back(tmp);
			}

			const Point* pts = reinterpret_cast<const cv::Point*>(Mat(obj.location).data);
			int npts = Mat(obj.location).rows;
			polylines(im, &pts, &npts, 1, true, Scalar(0, 0, 255), 2);

			cout << endl;

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
