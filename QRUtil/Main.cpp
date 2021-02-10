
#include "WndLogic.h"

// Enabling Visual Styles
#pragma comment(linker, \
R"V0G0N("/manifestdependency:type='win32' 
name='Microsoft.Windows.Common-Controls'
version='6.0.0.0' 
processorArchitecture='*'
publicKeyToken='6595b64144ccf1df'
language='*'")V0G0N")

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd) {

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    MainWnd* main_window = nullptr;

    if (argc == 2) {
        main_window = new MainWnd(hInstance, argv[1]);
    } else if (argc == 1) {
        main_window = new MainWnd(hInstance);
    } else {
        wstring err = L"Invalid arguments.\n";
        err += argv[0];
        err += L" [config_filename] (default: ./config.json).";
        MessageBox(0, err.c_str(), L"QRUtil", MB_OK);
        return 0;
    }
    int ret = 0;
    if (!main_window->init_error) {
        ret = main_window->DoEvent();

    } else {
        ret = EXIT_FAILURE;
    }
    delete main_window;
    return ret;
	
}