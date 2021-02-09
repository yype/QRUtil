
// #define DEBUG
#include <locale>
#include <codecvt>
#include <string>
#include <json/json.h>
#include <windows.h>
#include <windowsx.h>
#include <Unknwn.h> 
#include <gdiplus.h>
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <cctype>
#include <thread>
#include <mutex>

#include "Detector.h"
#include "encoding/encoding.h"
#pragma comment(lib, "gdiplus.lib")

using namespace std;
using namespace Gdiplus;

class MainWnd {
public:
	MainWnd(HINSTANCE hInstance);
	MainWnd(HINSTANCE hInstance, wstring config_filename);
	~MainWnd();
	int DoEvent(); // begin listening for events
	HWND hwnd;
	bool init_error = false;
private:
	bool CopySelectedQRData2Clipboard();
	void SwitchMode();
	void SelectAllQRCodes(bool select);
	bool SetClipboardText(string text);
	bool ARGBString2Color(string s, Gdiplus::Color& color);
	bool CheckSingle();
	void ShowCaptureWindow();
	void RefreshWindow();
	void OnMouseMove(WPARAM wParam, LPARAM lParam);
	void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	void OnLButtonUp(WPARAM wParam, LPARAM lParam);
	void OnRButtonDown(WPARAM wParam, LPARAM lParam);
	void ParseConfig(bool use_default = false);
	void SetDefaultConfig();
	bool RegHotkey();
	void InitConstants();
	void InitGDIPlus();
	void ShutdownGdiPlus();
	void CaptureScreen();
	void DetectQRCodes();
	void BeginDetectQRCodes();
	bool CrWindow();
	void RegClass();
	void SetFullScreen();
	void HideWindow();
	void GetScreenRes(int& height, int& width);
	void OnPaint(HDC hdc);
	void SetHintText(Gdiplus::Graphics& graphics, std::string text);
	void SetModeText(Gdiplus::Graphics& graphics, std::string text);
	bool CheckHover(Gdiplus::Point* point, int mouse_x, int mouse_y);
	static void OnTimer(
		HWND hwnd,
		UINT msg, // the WM_TIMER message.
		UINT_PTR identifier,
		DWORD elapsed_time // in ms
	);
	static LRESULT CALLBACK WindowProc(
		HWND hwnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
	);

	const wchar_t* CAPTION_NAME = L"QRUtil";
	HWND invisible_parent_window;
	bool is_capture_window_shown = false;
	bool is_showing_msgbox = NULL;
	int screen_width = 0;
	int screen_height = 0;
	const int timer1_interval = 30;
	const int timer2_interval = 300;
	wchar_t CLASS_NAME[MAX_CLASS_NAME];
	HINSTANCE hInstance;

	HBITMAP hbDesktop_screen_hdc = nullptr;
	HBITMAP hbDesktop_hdc_used_to_detect_qrcodes = nullptr;
	HBITMAP hbDesktop_displayed_screen_hdc = nullptr;
	HDC screen_hdc = nullptr; // the original captured screen
	HDC hdc_used_to_detect_qrcodes = nullptr; // perform detection on this DC
	HDC displayed_screen_hdc = nullptr; // the hdc being drawn

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	enum TIMER_EVENTS {
		ID1, ID2
	};

	enum class DETECT_MODE {
		MANUAL_DETECT,
		AUTO_DETECT
	};

	DETECT_MODE current_mode = DETECT_MODE::MANUAL_DETECT;

	// ------------------------------------------------
	// AUTO_DETECT
	vector<Detector::DecodedObject> decoded_objects;
	bool* hover_state = nullptr;
	bool* select_state = nullptr;
	// ------------------------------------------------

	// ------------------------------------------------
	// MANUAL_DETECT
	bool waiting_for_ending_point = false;
	cv::Point prev_starting_point, prev_ending_point; // two corners specified by the user
	cv::Point starting_point, ending_point; // two corners specified by the user
	bool successfully_detected = false;
	std::string decoded_string;
	// ------------------------------------------------

	// Configurable parameters
	Gdiplus::Color DARK_COVER;
	Gdiplus::Color AUTO_QR_BOARDER;
	float BOARDER_THICKNESS;
	Gdiplus::Color AUTO_QR_NO_HOVER;
	Gdiplus::Color AUTO_QR_HOVER;
	Gdiplus::Color QR_TEXT_BACKGROUND;
	Gdiplus::Color QR_TEXT_COLOR;
	Gdiplus::Color MODE_TEXT_COLOR;
	Gdiplus::Color MANUAL_OK_COLOR;
	Gdiplus::Color MANUAL_NOT_OK_COLOR;
	Gdiplus::Color MANUAL_SELECT_FILL;
	Gdiplus::Color MANUAL_QR_BORDER;
	Gdiplus::Color AUTO_QR_SELECT;

	bool hotkey_ctrl;
	bool hotkey_alt;
	bool hotkey_shift;
	UINT hotkey_vkey;
	UINT key_select_all;
	UINT key_deselect_all;
	UINT key_copy_selected;
	UINT key_exit_program;
	UINT key_switch_mode;
	wstring config_filename;

	std::mutex detect_qr_lock;
	std::mutex decoded_objects_mutex;

	static MainWnd* g_this;

	const string DEFAULT_CONFIG = R"V0G0N(
// -- default configuration file with default settings --
// QRUtil will fall back to the internal hard-coded config upon parsing failure on this file
{
    "COLOR":{ // The color format is AARRGGBB in hexadecimal form
       "DARK_COVER":"46000000", // A dark color that covers the entire screen
       "AUTO_QR_BOARDER":"FF007ACC", // Border color of the QR boxes (AUTO MODE)
       "BOARDER_THICKNESS":2.0, 
       "AUTO_QR_NO_HOVER":"1E007ACC", // Color of the QR boxes when not under hover (AUTO MODE)
       "AUTO_QR_HOVER":"64007ACC",
       "AUTO_QR_SELECT": "7DF5B84C", // Color of the QR boxes when selected (AUTO MODE)
       "QR_TEXT_BACKGROUND":"50000000", // Color of the background for the decoded hint text
       "QR_TEXT_COLOR":"FFFFFFFF", // Color of the decoded hint text
       "MANUAL_OK_COLOR": "80699C52", // Color of the selected range when successfully detected (MANUAL MODE)
       "MANUAL_NOT_OK_COLOR": "80DD5145", 
       "MANUAL_QR_BORDER": "FF007ACC", // Color of the selected range's border (MANUAL MODE)
       "MANUAL_SELECT_FILL":"1E007ACC", // Color of the selected range when still selecting (MANUAL MODE)
       "MODE_TEXT_COLOR":"80FFFFFF" // Color of the text color for the mode text (i.e. `MANUAL MODE` or `AUTO MODE`)
    },
    "HOTKEY":{ // hot key for capturing QR codes
       "CTRL":true,
       "ALT":true,
       "SHIFT":true,
       "VKEY":"I" // https://docs.microsoft.com/windows/desktop/inputdev/virtual-key-codes
    },
    "KEY":{ // define the function of keys pressed on the capture screen
       "SELECT_ALL":"A", // KEYs are not case-sensitive
       "DESELECT_ALL":"D",
       "COPY_SELECTED":"C",
       "SWITCH_MODE":"Z",
       "EXIT_PROGRAM": "71" // hexidecimal form of the virtual key code, in this case 0x71(F2)
    },
    "DEFAULT_MODE": "MANUAL" // `MANUAL` or `AUTO`
 }
)V0G0N";


};