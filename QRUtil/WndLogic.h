
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

#include "Detector.h"
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
	void SelectAllQRCodes(bool select);
	bool SetClipboardText(string text);
	bool ARGBString2Color(string s, Gdiplus::Color& color);
	bool CheckSingle();
	void ShowCaptureWindow();
	void RefreshWindow();
	void OnMouseMove(WPARAM wParam, LPARAM lParam);
	void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	void OnRButtonDown(WPARAM wParam, LPARAM lParam);
	void ParseConfig();
	void SetDefaultConfig();
	bool RegHotkey();
	void InitConstants();
	void InitGDIPlus();
	void ShutdownGdiPlus();
	void CaptureScreen();
	void DetectQRCodes();
	bool CrWindow();
	void RegClass();
	void SetFullScreen();
	void HideWindow();
	void GetScreenRes(int& height, int& width);
	void OnPaint(HDC hdc);
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

	const wchar_t *CAPTION_NAME = L"QRUtil";
	HWND invisible_parent_window;
	bool is_capture_window_shown = false;
	bool is_showing_msgbox = NULL;
	int screen_width = 0;
	int screen_height = 0;
	const int timer1_interval = 30;
	const int timer2_interval = 300;
	wchar_t CLASS_NAME[MAX_CLASS_NAME]; 
	HINSTANCE hInstance;
	HDC screen_hdc;
	HBITMAP hbDesktop;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;
	enum TIMER_EVENTS {
		ID1, ID2
	};

	vector<Detector::DecodedObject> decoded_objects;
	bool* hover_state = nullptr;
	bool* select_state = nullptr;
	
	// Configurable parameters
	Gdiplus::Color T_DARK;
	Gdiplus::Color T_QR_BOARDER;
	float BOARDER_THICKNESS;
	Gdiplus::Color T_QR_NO_HOVER;
	Gdiplus::Color T_QR_HOVER;
	Gdiplus::Color QR_TEXT_BACKGROUND;
	Gdiplus::Color QR_TEXT_COLOR;
	bool hotkey_ctrl;
	bool hotkey_alt;
	bool hotkey_shift;
	UINT hotkey_vkey;
	UINT key_select_all;
	UINT key_deselect_all;
	UINT key_copy_selected;
	UINT key_exit_program;
	wstring config_filename;


	static MainWnd* g_this;

};