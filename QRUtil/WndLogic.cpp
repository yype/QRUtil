#include "WndLogic.h"

// static member initialization
MainWnd* MainWnd::g_this = nullptr;

MainWnd::MainWnd(HINSTANCE hInstance) : MainWnd(hInstance, L"config.json")
{
}

MainWnd::MainWnd(HINSTANCE hInstance, wstring config_filename)
{
	this->config_filename = config_filename;

	wstring error_msg;
	g_this = this;

	if (!this->CheckSingle()) {
		error_msg += L"Another instance is already running.";
		goto _error;
	}

	this->InitConstants();
	this->RegClass();
	this->InitGDIPlus();
	this->ParseConfig();
	if (!this->CrWindow()) {
		error_msg += L"CreateWindow failed.";
		goto _error;
	}
	if (!this->RegHotkey()) {
		error_msg += L"RegisterHotKey failed.";
		goto _error;
	}

	return;

_error:
	init_error = true;

	error_msg += L"\nInitialization aborted.";
	MessageBox(0, error_msg.c_str(),
		CAPTION_NAME, MB_ICONEXCLAMATION | MB_OK
	);

}

MainWnd::~MainWnd()
{
	if (hover_state) delete(hover_state);
	if (select_state) delete(select_state);
}

int MainWnd::DoEvent()
{
	if (init_error) {
		MessageBox(0, L"DoEvent failed due to initialization error.",
			CAPTION_NAME, MB_ICONEXCLAMATION | MB_OK
		);
		return -1;
	}

	// Message dispatcher
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Exit
	this->ShutdownGdiPlus();
	return 0;
}

bool MainWnd::CopySelectedQRData2Clipboard()
{
	string result;
	for (int i = 0; i < decoded_objects.size(); i++) {
		if (select_state[i]) {
			result += decoded_objects[i].data + "\n\n";
		}
	}

	if (result.length() != 0) {
		result.pop_back(); // strip
		result.pop_back();
		// if no qrcode is selected then the clipboard will be cleared+
		return SetClipboardText(result);
	}

	return false;

}

void MainWnd::SwitchMode()
{
	if (current_mode == DETECT_MODE::AUTO_DETECT) {
		current_mode = DETECT_MODE::MANUAL_DETECT;
		starting_point = ending_point;
		waiting_for_ending_point = false;
		successfully_detected = false;
	}
	else {
		decoded_objects.clear();
		current_mode = DETECT_MODE::AUTO_DETECT;
		this->BeginDetectQRCodes();
	}
	RefreshWindow();
}

void MainWnd::SelectAllQRCodes(bool select)
{
	for (int i = 0; i < decoded_objects.size(); i++) {
		select_state[i] = select;
	}
	RefreshWindow();
}

bool MainWnd::SetClipboardText(string text)
{

	if (!OpenClipboard(hwnd)) {
		return false;
	}

	EmptyClipboard();

	// convert from multiple byte to wide char (UTF8 support)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wstring wide = converter.from_bytes(text.c_str());

	size_t sz = (wide.length() + 1) * sizeof(wchar_t);
	HGLOBAL hglb_copy = GlobalAlloc(GMEM_MOVEABLE, sz);
	if (!hglb_copy) return false;
	LPWSTR lpstr_copy = reinterpret_cast<LPWSTR>(GlobalLock(hglb_copy));
	if (!lpstr_copy) return false;
	memcpy(lpstr_copy, wide.c_str(), sz);
	GlobalUnlock(hglb_copy);
	SetClipboardData(CF_UNICODETEXT, hglb_copy);
	CloseClipboard();
	return true;
}

bool MainWnd::ARGBString2Color(string s, Gdiplus::Color& color)
{
	// The color format is AARRGGBB in hexadecimal form
	if (s.length() != 8) return false;
	try {
		auto color_dword = static_cast<DWORD>(stoul(s, nullptr, 16));
		color = Gdiplus::Color(static_cast<Gdiplus::ARGB>(color_dword));
		return true;
	}
	catch (exception) {
		return false;
	}
	return true;
}

bool MainWnd::CheckSingle()
{
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, CAPTION_NAME);
	if (hMutex) {
		return false;
	}
	else {
		CreateMutex(0, 0, CAPTION_NAME);
		return true;
	}
}

void MainWnd::ShowCaptureWindow()
{
	if (is_capture_window_shown) return;
	this->SetFullScreen();
	is_capture_window_shown = true;
	this->CaptureScreen();

	if (current_mode == DETECT_MODE::AUTO_DETECT) {
		decoded_objects.clear();
		this->BeginDetectQRCodes();
	}

	if (current_mode == DETECT_MODE::MANUAL_DETECT) {
		starting_point = ending_point;
		waiting_for_ending_point = false;
		successfully_detected = false;
	}

	// stay on top
	SetTimer(hwnd, TIMER_EVENTS::ID2, this->timer2_interval, (TIMERPROC)this->OnTimer);
}

void MainWnd::RefreshWindow()
{
	RECT rt;
	GetWindowRect(hwnd, &rt);
	InvalidateRect(hwnd, &rt, false);
}

void MainWnd::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	auto x = GET_X_LPARAM(lParam);
	auto y = GET_Y_LPARAM(lParam);
	if (current_mode == DETECT_MODE::AUTO_DETECT) {
		decoded_objects_mutex.lock();
		auto decoded_objects_copy = decoded_objects;
		auto sz = decoded_objects_copy.size();
		bool* hover_state_tmp = new bool[sz];
		bool* select_state_tmp = new bool[sz];
		memcpy(hover_state_tmp, hover_state, sz);
		memcpy(select_state_tmp, select_state, sz);
		decoded_objects_mutex.unlock();

		
		bool found = false;
		for (int i = 0; i < decoded_objects_copy.size(); i++) {
			if (!found && CheckHover((Gdiplus::Point*)(&decoded_objects_copy[i].location[0]), x, y)) {
				found = true;
				hover_state[i] = true;
			}
			else {
				hover_state[i] = false;
			}
		}

		decoded_objects_mutex.lock();
		memcpy(hover_state, hover_state_tmp, sz);
		memcpy(select_state, select_state_tmp, sz);
		decoded_objects_mutex.unlock();
		delete[] hover_state_tmp;
		delete[] select_state_tmp;

		RefreshWindow();
	}
	if (current_mode == DETECT_MODE::MANUAL_DETECT) {
		if (waiting_for_ending_point) {
			ending_point.x = x;
			ending_point.y = y;

			successfully_detected = false;
			RefreshWindow();
		}
	}

}

void MainWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	auto x = GET_X_LPARAM(lParam);
	auto y = GET_Y_LPARAM(lParam);
	if (current_mode == DETECT_MODE::AUTO_DETECT) {
		decoded_objects_mutex.lock();
		auto decoded_objects_copy = decoded_objects;
		decoded_objects_mutex.unlock();

		
		for (int i = 0; i < decoded_objects_copy.size(); i++) {
			if (CheckHover(reinterpret_cast<Gdiplus::Point*>(&decoded_objects_copy[i].location[0]), x, y)) {
				SetClipboardText(decoded_objects_copy[i].data);
				HideWindow();
				break;
			}
		}
	}

	if (current_mode == DETECT_MODE::MANUAL_DETECT) {
		if (successfully_detected) {
			prev_starting_point = starting_point;
			prev_ending_point = ending_point;
		}
		starting_point.x = ending_point.x = x;
		starting_point.y = ending_point.y = y;
		waiting_for_ending_point = true;
	}
}

void MainWnd::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (current_mode == DETECT_MODE::MANUAL_DETECT) {
		auto x = GET_X_LPARAM(lParam);
		auto y = GET_Y_LPARAM(lParam);
		if (waiting_for_ending_point) {
			ending_point.x = x;
			ending_point.y = y;
			waiting_for_ending_point = false;
			if (starting_point != ending_point) {
				Detector dt;
				this->successfully_detected = dt.DetectQRWeChat(hdc_used_to_detect_qrcodes,
					starting_point.x, starting_point.y,
					ending_point.x, ending_point.y,
					this->decoded_string
				);
			}
			else {
				if (successfully_detected) {
					if ( ((x-prev_starting_point.x)*(x-prev_ending_point.x) <0) && 
						((y-prev_ending_point.y)*(y-prev_starting_point.y)<0) ) { // clicked on the selected range
						SetClipboardText(decoded_string);
						HideWindow();
					}
				}
			}
			
		}
		RefreshWindow();
	}
}

void MainWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (current_mode == DETECT_MODE::AUTO_DETECT) {
		if (!detect_qr_lock.try_lock()) {
			return;
		}
		detect_qr_lock.unlock();
		decoded_objects_mutex.lock();
		auto decoded_objects_copy = decoded_objects;
		auto sz = decoded_objects_copy.size();
		bool* hover_state_tmp = new bool[sz];
		bool* select_state_tmp = new bool[sz];
		memcpy(hover_state_tmp, hover_state, sz);
		memcpy(select_state_tmp, select_state, sz);
		decoded_objects_mutex.unlock();


		auto x = GET_X_LPARAM(lParam);
		auto y = GET_Y_LPARAM(lParam);
		for (int i = 0; i < decoded_objects_copy.size(); i++) {
			if (CheckHover(reinterpret_cast<Gdiplus::Point*>(&decoded_objects_copy[i].location[0]), x, y)) {
				select_state_tmp[i] = !select_state_tmp[i];
				RefreshWindow();
				break;
			}
		}

		decoded_objects_mutex.lock();
		memcpy(hover_state, hover_state_tmp, sz);
		memcpy(select_state, select_state_tmp, sz);
		decoded_objects_mutex.unlock();
		delete[] hover_state_tmp;
		delete[] select_state_tmp;
	}
}

void MainWnd::ParseConfig()
{
	wstring error_msg;
	ifstream fs;
	stringstream str_stream;
	string raw_json;

	fs.open(config_filename);
	if (fs.is_open()) {
		str_stream << fs.rdbuf();
		raw_json = str_stream.str();

		const auto raw_json_length = static_cast<int>(raw_json.length());

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		JSONCPP_STRING err;
		Json::Value root;
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		if (!reader->parse(raw_json.c_str(), raw_json.c_str() + raw_json_length, &root,
			&err)) {
			std::wstring wide = converter.from_bytes(err.c_str());
			error_msg += L"Json parsing error:\n";
			error_msg += wide;
		}
		else {
			try {
				bool success = true;

				success &= ARGBString2Color(root["COLOR"]["DARK_COVER"].asString(), DARK_COVER);
				success &= ARGBString2Color(root["COLOR"]["AUTO_QR_BOARDER"].asString(), AUTO_QR_BOARDER);
				success &= ARGBString2Color(root["COLOR"]["AUTO_QR_NO_HOVER"].asString(), AUTO_QR_NO_HOVER);
				success &= ARGBString2Color(root["COLOR"]["AUTO_QR_HOVER"].asString(), AUTO_QR_HOVER);
				success &= ARGBString2Color(root["COLOR"]["AUTO_QR_SELECT"].asString(), AUTO_QR_SELECT);
				success &= ARGBString2Color(root["COLOR"]["QR_TEXT_BACKGROUND"].asString(), QR_TEXT_BACKGROUND);
				success &= ARGBString2Color(root["COLOR"]["QR_TEXT_COLOR"].asString(), QR_TEXT_COLOR);
				success &= ARGBString2Color(root["COLOR"]["MODE_TEXT_COLOR"].asString(), MODE_TEXT_COLOR);
				success &= ARGBString2Color(root["COLOR"]["MANUAL_OK_COLOR"].asString(), MANUAL_OK_COLOR);
				success &= ARGBString2Color(root["COLOR"]["MANUAL_NOT_OK_COLOR"].asString(), MANUAL_NOT_OK_COLOR);
				success &= ARGBString2Color(root["COLOR"]["AUTO_SELECT_FILL"].asString(), AUTO_SELECT_FILL);
				success &= ARGBString2Color(root["COLOR"]["MANUAL_QR_BORDER"].asString(), MANUAL_QR_BORDER);
				success &= root["COLOR"].isMember("BOARDER_THICKNESS");

				// check key existence
				success &= root.isMember("HOTKEY");
				success &= root["HOTKEY"].isMember("CTRL");
				success &= root["HOTKEY"].isMember("ALT");
				success &= root["HOTKEY"].isMember("SHIFT");
				success &= root["HOTKEY"].isMember("VKEY");

				success &= root.isMember("KEY");
				success &= root["KEY"].isMember("SELECT_ALL");
				success &= root["KEY"].isMember("DESELECT_ALL");
				success &= root["KEY"].isMember("COPY_SELECTED");
				success &= root["KEY"].isMember("EXIT_PROGRAM");
				success &= root["KEY"].isMember("SWITCH_MODE");
				success &= root.isMember("DEFAULT_MODE");
				auto mode = root["DEFAULT_MODE"].asString();
				if (mode != "AUTO" && mode != "MANUAL") {
					success &= false;
				}
				current_mode = (mode == "AUTO") ? DETECT_MODE::AUTO_DETECT : DETECT_MODE::MANUAL_DETECT;

				BOARDER_THICKNESS = root["COLOR"]["BOARDER_THICKNESS"].asFloat();
				hotkey_ctrl = root["HOTKEY"]["CTRL"].asBool();
				hotkey_alt = root["HOTKEY"]["ALT"].asBool();
				hotkey_shift = root["HOTKEY"]["SHIFT"].asBool();
				hotkey_vkey = root["HOTKEY"]["VKEY"].asString()[0];
				key_select_all = root["KEY"]["SELECT_ALL"].asString()[0];
				key_deselect_all = root["KEY"]["DESELECT_ALL"].asString()[0];
				key_copy_selected = root["KEY"]["COPY_SELECTED"].asString()[0];
				key_switch_mode = root["KEY"]["SWITCH_MODE"].asString()[0];

				auto ep_key = root["KEY"]["EXIT_PROGRAM"].asString();
				key_exit_program = stoul(ep_key, nullptr, 16) % 0x100;

				if (success) {
					return;
				}
			}
			catch (exception& e) {
				std::wstring wide = converter.from_bytes(e.what());
				error_msg += wide;
			}
		}

	}

	// will reach this point when an error occurs
	// _parse_error:
	error_msg += L"\nParsing config failed, will use default settings instead.";
	MessageBox(hwnd, error_msg.c_str(), CAPTION_NAME, MB_OK);
	SetDefaultConfig();
	return;
}

void MainWnd::SetDefaultConfig()
{
	DARK_COVER = Gdiplus::Color(70, 0, 0, 0);
	AUTO_QR_BOARDER = Gdiplus::Color(255, 0, 122, 204);
	AUTO_QR_NO_HOVER = Gdiplus::Color(30, 0, 122, 204);
	QR_TEXT_BACKGROUND = Gdiplus::Color(80, 0, 0, 0);
	AUTO_QR_HOVER = Gdiplus::Color(100, 0, 122, 204);
	QR_TEXT_COLOR = Gdiplus::Color(255, 255, 255, 255);
	BOARDER_THICKNESS = 2.0f;
	hotkey_ctrl = true;
	hotkey_alt = true;
	hotkey_shift = true;
	hotkey_vkey = 'I';
	key_copy_selected = 'C';
	key_select_all = 'A';
	key_deselect_all = 'D';
}

bool MainWnd::RegHotkey()
{
	UINT fsModifiers = NULL;
	if (this->hotkey_ctrl) {
		fsModifiers |= MOD_CONTROL;
	}
	if (this->hotkey_shift) {
		fsModifiers |= MOD_SHIFT;
	}
	if (this->hotkey_alt) {
		fsModifiers |= MOD_ALT;
	}

	return RegisterHotKey(hwnd, 0, fsModifiers, this->hotkey_vkey);
}

void MainWnd::InitConstants()
{
	wcscpy_s(this->CLASS_NAME, CAPTION_NAME);

	this->screen_width = GetSystemMetrics(SM_CXSCREEN);
	this->screen_height = GetSystemMetrics(SM_CYSCREEN);
}

void MainWnd::InitGDIPlus()
{
	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void MainWnd::ShutdownGdiPlus()
{
	GdiplusShutdown(gdiplusToken);
}

void MainWnd::CaptureScreen()
{
	GetScreenRes(screen_height, screen_width);
	HDC hdc = GetDC(NULL);
	if (screen_hdc) {
		ReleaseDC(NULL, screen_hdc);
		screen_hdc = nullptr;
	}
	if (hdc_used_to_detect_qrcodes) {
		ReleaseDC(NULL, hdc_used_to_detect_qrcodes);
		hdc_used_to_detect_qrcodes = nullptr;
	}
	if (displayed_screen_hdc) {
		ReleaseDC(NULL, displayed_screen_hdc);
		displayed_screen_hdc = nullptr;
	}

	screen_hdc = CreateCompatibleDC(hdc);
	hdc_used_to_detect_qrcodes = CreateCompatibleDC(hdc);
	displayed_screen_hdc = CreateCompatibleDC(hdc);

	if (hbDesktop_screen_hdc) {
		DeleteObject(hbDesktop_screen_hdc);
		hbDesktop_screen_hdc = nullptr;
	}
	if (hbDesktop_hdc_used_to_detect_qrcodes) {
		DeleteObject(hbDesktop_hdc_used_to_detect_qrcodes);
		hbDesktop_hdc_used_to_detect_qrcodes = nullptr;
	}
	if (hbDesktop_displayed_screen_hdc) {
		DeleteObject(hbDesktop_displayed_screen_hdc);
		hbDesktop_displayed_screen_hdc = nullptr;
	}

	hbDesktop_screen_hdc = CreateCompatibleBitmap(hdc, screen_width, screen_height);
	hbDesktop_hdc_used_to_detect_qrcodes = CreateCompatibleBitmap(hdc, screen_width, screen_height);
	hbDesktop_displayed_screen_hdc = CreateCompatibleBitmap(hdc, screen_width, screen_height);

	SelectObject(screen_hdc, hbDesktop_screen_hdc);
	SelectObject(hdc_used_to_detect_qrcodes, hbDesktop_hdc_used_to_detect_qrcodes);
	SelectObject(displayed_screen_hdc, hbDesktop_displayed_screen_hdc);

	BitBlt(screen_hdc, 0, 0, screen_width, screen_height, hdc, 0, 0, SRCCOPY);
	BitBlt(hdc_used_to_detect_qrcodes, 0, 0, screen_width, screen_height, hdc, 0, 0, SRCCOPY);
	BitBlt(displayed_screen_hdc, 0, 0, screen_width, screen_height, hdc, 0, 0, SRCCOPY);

	ReleaseDC(NULL, hdc);
	// MessageBox(0, L"done", L"none", MB_OK);
}

void MainWnd::DetectQRCodes()
{
	if (!detect_qr_lock.try_lock()) {
		return;
	}
	Detector dt;
	vector<Detector::DecodedObject> decoded_objects_tmp;
	for (int thrd = 0; thrd < 256; thrd += 5) {
		if (!is_capture_window_shown) break;
		dt.DetectQRZbar(hdc_used_to_detect_qrcodes, screen_width, screen_height, thrd, decoded_objects_tmp);

		decoded_objects_mutex.lock();
		auto sz = decoded_objects_tmp.size();
		if (sz) {
			hover_state = new bool[sz];
			ZeroMemory(hover_state, sz * sizeof(bool));
			select_state = new bool[sz];
			ZeroMemory(select_state, sz * sizeof(bool));
		}
		decoded_objects = decoded_objects_tmp;
		decoded_objects_mutex.unlock();
		
		RefreshWindow();
	}
	detect_qr_lock.unlock();
}

void MainWnd::BeginDetectQRCodes()
{
	std::thread([this] { this->DetectQRCodes(); }).detach();
}

bool MainWnd::CrWindow()
{
	invisible_parent_window = CreateWindow(CLASS_NAME, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	if (!invisible_parent_window) return false;

	this->hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		CAPTION_NAME,
		NULL,
		0, 0,
		screen_width,
		screen_height,
		invisible_parent_window,
		NULL,
		hInstance,
		NULL
	);

	return hwnd ? true : false;
}

void MainWnd::RegClass()
{
	// register the window class
	WNDCLASS wc = { };
	wc.lpfnWndProc = this->WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = CreatePatternBrush(hbDesktop_screen_hdc); // use the captured screen as the background image
	RegisterClass(&wc);
}

void MainWnd::SetFullScreen()
{
	LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
	SetWindowLong(hwnd, GWL_EXSTYLE, ex_style | WS_EX_TOPMOST);

	LONG style = ::GetWindowLong(hwnd, GWL_STYLE);
	style &= ~WS_BORDER & ~WS_DLGFRAME & ~WS_THICKFRAME;
	style |= WS_POPUP;
	SetWindowLong(hwnd, GWL_STYLE, style);
	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

}

void MainWnd::HideWindow()
{
	ShowWindow(hwnd, SW_HIDE);
	KillTimer(hwnd, TIMER_EVENTS::ID2);
	is_capture_window_shown = false;
}

void MainWnd::GetScreenRes(int& height, int& width)
{
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
}

void MainWnd::OnPaint(HDC hdc)
{
	if (current_mode == DETECT_MODE::AUTO_DETECT) {

		// captured screen
		auto ret = BitBlt(hdc, 0, 0, this->screen_width, this->screen_height, displayed_screen_hdc, 0, 0, SRCCOPY);


		Gdiplus::Graphics graphics(hdc);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		// dark hover
		Gdiplus::Brush* dark_bs = new Gdiplus::SolidBrush(DARK_COVER);
		graphics.FillRectangle(dark_bs, 0, 0, screen_width, screen_height);
		graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

		SetModeText(graphics, "AUTO MODE");

		decoded_objects_mutex.lock();
		auto decoded_objects_copy = decoded_objects;
		POINT point;
		GetCursorPos(&point);
		bool found = false;
		for (int i = 0; i < decoded_objects_copy.size(); i++) {
			if (!found && CheckHover((Gdiplus::Point*)(&decoded_objects_copy[i].location[0]), point.x, point.y)) {
				found = true;
				hover_state[i] = true;
			}
			else {
				hover_state[i] = false;
			}
		}
		auto sz = decoded_objects_copy.size();
		bool* hover_state_tmp = new bool[sz];
		bool* select_state_tmp = new bool[sz];
		memcpy(hover_state_tmp, hover_state, sz);
		memcpy(select_state_tmp, select_state, sz);
		decoded_objects_mutex.unlock();

		if (!detect_qr_lock.try_lock()) {
			// check if on hover
			bool not_hovered = true;
			for (int i = 0; i < sz; i++) {
				if (hover_state_tmp[i] == true) {
					not_hovered = false;
					break;
				}
			}
			if (not_hovered) {
				SetHintText(graphics, "Detecting...");
			}

		}
		else {
			detect_qr_lock.unlock();
		}



		Gdiplus::Pen pen(AUTO_QR_BOARDER, BOARDER_THICKNESS);
		Gdiplus::SolidBrush bs_no_hover(AUTO_QR_NO_HOVER);
		Gdiplus::SolidBrush bs_hover(AUTO_QR_HOVER);
		Gdiplus::SolidBrush bs_select(AUTO_QR_SELECT);

		for (int i = 0; i < sz; i++) {
			Gdiplus::Point* p = reinterpret_cast<Gdiplus::Point*>(&decoded_objects_copy[i].location[0]);
			int count = static_cast<int>(decoded_objects_copy[i].location.size()); // must be 4 (edges of a rectangle)
			graphics.DrawPolygon(&pen, p, count);
			if (hover_state_tmp[i])
				graphics.FillPolygon(&bs_hover, p, count);
			else
				graphics.FillPolygon(&bs_no_hover, p, count);

			if (select_state_tmp[i])
				graphics.FillPolygon(&bs_select, p, count);

		}
		for (int i = 0; i < sz; i++) {
			if (hover_state_tmp[i]) {
				SetHintText(graphics, decoded_objects_copy[i].data);
				break;
			}
		}
		delete[] select_state_tmp;
		delete[] hover_state_tmp;

	}

	if (current_mode == DETECT_MODE::MANUAL_DETECT) {
		// captured screen
		auto ret = BitBlt(hdc, 0, 0, this->screen_width, this->screen_height, displayed_screen_hdc, 0, 0, SRCCOPY);


		Gdiplus::Graphics graphics(hdc);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		// dark hover
		Gdiplus::Brush* dark_bs = new Gdiplus::SolidBrush(DARK_COVER);
		graphics.FillRectangle(dark_bs, 0, 0, screen_width, screen_height);
		graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

		SetModeText(graphics, "MANUAL MODE");

		Gdiplus::Pen pen(AUTO_QR_BOARDER, BOARDER_THICKNESS);
		Gdiplus::SolidBrush bs_fill(AUTO_SELECT_FILL);
		Gdiplus::SolidBrush bs_ok(MANUAL_OK_COLOR);
		Gdiplus::SolidBrush bs_notok(MANUAL_NOT_OK_COLOR);


		if (starting_point != ending_point) {
			auto min_x = MIN(starting_point.x, ending_point.x);
			auto max_x = MAX(starting_point.x, ending_point.x);
			auto min_y = MIN(starting_point.y, ending_point.y);
			auto max_y = MAX(starting_point.y, ending_point.y);
			graphics.DrawRectangle(&pen, min_x, min_y, max_x - min_x, max_y - min_y);

			if (!waiting_for_ending_point) {
				if (successfully_detected) {
					graphics.FillRectangle(&bs_ok, min_x, min_y, max_x - min_x, max_y - min_y);
					SetHintText(graphics, decoded_string);
				}
				else {
					graphics.FillRectangle(&bs_notok, min_x, min_y, max_x - min_x, max_y - min_y);
				}
			}
			else {
				graphics.FillRectangle(&bs_fill, min_x, min_y, max_x - min_x, max_y - min_y);
			}
			
		}
		
		
	}

}


void MainWnd::SetHintText(Gdiplus::Graphics& graphics, std::string text)
{
	Gdiplus::SolidBrush qr_text_background(QR_TEXT_BACKGROUND);
	FontFamily   fontFamily(L"Microsoft YaHei");
	Font         font(&fontFamily, 36, FontStyleBold, UnitPoint);
	RectF        rectF(0, 0, screen_width * 10.0f, screen_height * 1.0f);
	RectF outrect;
	SolidBrush   solidBrush(QR_TEXT_COLOR);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	
	std::wstring wide;
	auto enc = Encoding::getEncoding((const uchar*)text.c_str(), text.size());
	uchar dst[1000];
	auto len = Encoding::decode((short*)dst, 1000, (const uchar*)text.c_str(), text.size(), enc);
	wide = wstring((wchar_t*)dst, len);
	// wide = converter.from_bytes(text.c_str(), text.c_str()+text.size());

	while (true) {
		// trim the text
		graphics.MeasureString(wide.c_str(), static_cast<int>(wide.length()), &font, rectF, &outrect);
		int right = static_cast<int>(outrect.GetRight());
		if (right < screen_width) {
			break;
		}
		wide.pop_back();
		wide[wide.length() - 1] = '.';
		wide[wide.length() - 2] = '.';
		wide[wide.length() - 3] = '.';
	}

	outrect.Width = static_cast<REAL>(screen_width);
	graphics.FillRectangle(&qr_text_background, outrect);
	graphics.DrawString(wide.c_str(), -1, &font, outrect, NULL, &solidBrush);
}

void MainWnd::SetModeText(Gdiplus::Graphics& graphics, std::string text)
{
	Gdiplus::SolidBrush qr_text_background(QR_TEXT_BACKGROUND);
	FontFamily   fontFamily(L"Microsoft YaHei");
	Font         font(&fontFamily, 28, FontStyleBold, UnitPoint);
	RectF        rectF(0, 0, screen_width * 10.0f, screen_height * 1.0f);
	RectF outrect;
	SolidBrush   solidBrush(MODE_TEXT_COLOR);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(text.c_str());

	while (true) {
		// trim the text
		graphics.MeasureString(wide.c_str(), static_cast<int>(wide.length()), &font, rectF, &outrect);
		int right = static_cast<int>(outrect.GetRight());
		if (right < screen_width / 2) {
			break;
		}
		wide.pop_back();
		wide[wide.length() - 1] = '.';
		wide[wide.length() - 2] = '.';
		wide[wide.length() - 3] = '.';
	}

	//outrect.Width = static_cast<REAL>(screen_width);
	outrect.X = (screen_width /2) - (outrect.Width /2 );
	outrect.Y = screen_height - outrect.Height*2;
	// graphics.FillRectangle(&qr_text_background, outrect);
	graphics.DrawString(wide.c_str(), -1, &font, outrect, NULL, &solidBrush);
}

bool MainWnd::CheckHover(Gdiplus::Point* point, int mouse_x, int mouse_y)
{
	// add all the angles together and check if it equals 2*PI
	const double ERROR_RANGE = 0.01;
	const double PI = 3.14159265;
	double sum = 0;
	for (int i = 0; i < 4; i++) {
		double x1 = point[i].X, y1 = point[i].Y, x2 = point[(i + 1) % 4].X, y2 = point[(i + 1) % 4].Y;
		double x3 = mouse_x, y3 = mouse_y;
		double a = sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
		double b = sqrt(pow(x3 - x2, 2) + pow(y3 - y2, 2));
		double c = sqrt(pow(x3 - x1, 2) + pow(y3 - y1, 2));
		double CosA = (pow(b, 2) + pow(c, 2) - pow(a, 2)) / (2 * b * c);
		double A = acos(CosA);
		sum += A;
	}
	return fabs(sum - 2 * PI) < ERROR_RANGE;
}

void MainWnd::OnTimer(HWND hwnd, UINT msg, UINT_PTR identifier, DWORD elapsed_time)
{
	if (identifier == TIMER_EVENTS::ID2) {
		if (g_this->is_showing_msgbox) return;
		SetForegroundWindow(hwnd);
	}
}

LRESULT MainWnd::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_MOUSEMOVE:
		g_this->OnMouseMove(wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		g_this->OnLButtonDown(wParam, lParam);
		break;
	case WM_LBUTTONUP:
		g_this->OnLButtonUp(wParam, lParam);
		break;
	case WM_RBUTTONDOWN:
		g_this->OnRButtonDown(wParam, lParam);
		break;
	case WM_HOTKEY:
		g_this->ShowCaptureWindow();
		break;
	case WM_CHAR:
		if (tolower(static_cast<int>(wParam)) == tolower(static_cast<int>(g_this->key_copy_selected))) {
			if(g_this->current_mode == MainWnd::DETECT_MODE::AUTO_DETECT) {
				bool ret = g_this->CopySelectedQRData2Clipboard();
				if (!ret) {
					MessageBox(hwnd, L"Copy to clipboard failed.\nPlease select the QR codes that you want to copy first.", g_this->CAPTION_NAME, MB_OK | MB_ICONEXCLAMATION);
				}
				else {
					g_this->HideWindow();
				}
			}
			
		}
		if (tolower(static_cast<int>(wParam)) == tolower(static_cast<int>(g_this->key_select_all))) {
			g_this->SelectAllQRCodes(true);
		}
		if (tolower(static_cast<int>(wParam)) == tolower(static_cast<int>(g_this->key_deselect_all))) {
			g_this->SelectAllQRCodes(false);
		}
		if (tolower(static_cast<int>(wParam)) == tolower(static_cast<int>(g_this->key_switch_mode))) {
			g_this->SwitchMode();
		}
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			g_this->HideWindow();
			break;
		default:
			if (tolower(static_cast<int>(wParam)) == tolower(static_cast<int>(g_this->key_exit_program))) {
				if (g_this->is_showing_msgbox) break;
				g_this->is_showing_msgbox = true;

				if (MessageBox(hwnd,
					L"Are you sure you want to quit?\nThis will stop the hotkey monitoring service.",
					g_this->CAPTION_NAME, MB_YESNO | MB_ICONQUESTION) == IDYES) {
					exit(0);
				}
				g_this->is_showing_msgbox = false;
			}
			break;
		}
		break;
	case WM_CLOSE:
		return 0;
	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_MINIMIZE: // disable minimize event
			return 0;
		default:
			break;
		}
		break;
	case WM_NCCALCSIZE:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		// double buffering
		HDC hMemDc = CreateCompatibleDC(hdc);
		HBITMAP hBmp = CreateCompatibleBitmap(hdc, g_this->screen_width, g_this->screen_height);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDc, hBmp);
		g_this->OnPaint(hMemDc);
		BitBlt(hdc, 0, 0, g_this->screen_width, g_this->screen_height, hMemDc, 0, 0, SRCCOPY);
		SelectObject(hMemDc, hOldBmp);
		DeleteObject(hBmp);
		DeleteObject(hMemDc);
		EndPaint(hwnd, &ps);
		g_this->RefreshWindow();
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);

}
