#include <shobjidl_core.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <dwmapi.h>
#include <codecvt>
#include <map>
#include <memory>
#include <sstream>
#include <windows.h>
#include <wchar.h>
#pragma warning(disable:4996)
#pragma comment(lib, "dwmapi.lib")

#define STATE_NORMAL 0
#define STATE_MAXIMIZED 1
#define STATE_MINIMIZED 2
#define STATE_FULLSCREEN_ENTERED 3

#define DWMWA_USE_IMMERSIVE_DARK_MODE 19

namespace WindowManagerOriginPlugin{
    ///////////////////////// WindowManager Plugin
    const flutter::EncodableValue* ValueOrNull(const flutter::EncodableMap& map,
        const char* key) {
        auto it = map.find(flutter::EncodableValue(key));
        if (it == map.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    class WindowManager {
    public:
        WindowManager();

        virtual ~WindowManager();

        HWND native_window;

        int last_state = STATE_NORMAL;

        bool has_shadow_ = false;
        bool is_frameless_ = false;
        bool is_prevent_close_ = false;
        double aspect_ratio_ = 0;
        POINT minimum_size_ = { 0, 0 };
        POINT maximum_size_ = { -1, -1 };
        bool is_resizable_ = true;
        bool is_skip_taskbar_ = true;
        std::string title_bar_style_ = "hidden";
        double opacity_ = 1;

        bool is_resizing_ = false;
        bool is_moving_ = false;

        HWND GetMainWindow();
        void WindowManager::ForceRefresh();
        void WindowManager::ForceChildRefresh();
        void WindowManager::SetAsFrameless();
        void WindowManager::WaitUntilReadyToShow();
        void WindowManager::Destroy();
        void WindowManager::Close();
        bool WindowManager::IsPreventClose();
        void WindowManager::SetPreventClose(const flutter::EncodableMap& args);
        void WindowManager::Focus();
        void WindowManager::Blur();
        bool WindowManager::IsFocused();
        void WindowManager::Show();
        void WindowManager::Hide();
        bool WindowManager::IsVisible();
        bool WindowManager::IsMaximized();
        void WindowManager::Maximize(const flutter::EncodableMap& args);
        void WindowManager::Unmaximize();
        bool WindowManager::IsMinimized();
        void WindowManager::Minimize();
        void WindowManager::Restore();
        bool WindowManager::IsFullScreen();
        void WindowManager::SetFullScreen(const flutter::EncodableMap& args);
        void WindowManager::SetAspectRatio(const flutter::EncodableMap& args);
        void WindowManager::SetBackgroundColor(const flutter::EncodableMap& args);
        flutter::EncodableMap WindowManager::GetBounds(
            const flutter::EncodableMap& args);
        void WindowManager::SetBounds(const flutter::EncodableMap& args);
        void WindowManager::SetMinimumSize(const flutter::EncodableMap& args);
        void WindowManager::SetMaximumSize(const flutter::EncodableMap& args);
        bool WindowManager::IsResizable();
        void WindowManager::SetResizable(const flutter::EncodableMap& args);
        bool WindowManager::IsMinimizable();
        void WindowManager::SetMinimizable(const flutter::EncodableMap& args);
        bool WindowManager::IsMaximizable();
        void WindowManager::SetMaximizable(const flutter::EncodableMap& args);
        bool WindowManager::IsClosable();
        void WindowManager::SetClosable(const flutter::EncodableMap& args);
        bool WindowManager::IsAlwaysOnTop();
        void WindowManager::SetAlwaysOnTop(const flutter::EncodableMap& args);
        std::string WindowManager::GetTitle();
        void WindowManager::SetTitle(const flutter::EncodableMap& args);
        void WindowManager::SetTitleBarStyle(const flutter::EncodableMap& args);
        int WindowManager::GetTitleBarHeight();
        bool WindowManager::IsSkipTaskbar();
        void WindowManager::SetSkipTaskbar(const flutter::EncodableMap& args);
        void WindowManager::SetProgressBar(const flutter::EncodableMap& args);
        void WindowManager::SetIcon(const flutter::EncodableMap& args);
        bool WindowManager::HasShadow();
        void WindowManager::SetHasShadow(const flutter::EncodableMap& args);
        double WindowManager::GetOpacity();
        void WindowManager::SetOpacity(const flutter::EncodableMap& args);
        void WindowManager::SetBrightness(const flutter::EncodableMap& args);
        void WindowManager::SetIgnoreMouseEvents(const flutter::EncodableMap& args);
        void WindowManager::PopUpWindowMenu(const flutter::EncodableMap& args);
        void WindowManager::StartDragging();
        void WindowManager::StartResizing(const flutter::EncodableMap& args);

    private:
        bool g_is_window_fullscreen = false;
        std::string g_title_bar_style_before_fullscreen;
        bool g_is_frameless_before_fullscreen;
        RECT g_frame_before_fullscreen;
        bool g_maximized_before_fullscreen;
        LONG g_style_before_fullscreen;
        LONG g_ex_style_before_fullscreen;
        ITaskbarList3* taskbar_ = nullptr;
    };

    WindowManager::WindowManager() {}

    WindowManager::~WindowManager() {}

    HWND WindowManager::GetMainWindow() {
        return native_window;
    }

    void WindowManager::ForceRefresh() {
        HWND hWnd = GetMainWindow();

        RECT rect;

        GetWindowRect(hWnd, &rect);
        SetWindowPos(
            hWnd, nullptr, rect.left, rect.top, rect.right - rect.left + 1,
            rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
        SetWindowPos(
            hWnd, nullptr, rect.left, rect.top, rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
    }

    void WindowManager::ForceChildRefresh() {
        HWND hWnd = GetWindow(GetMainWindow(), GW_CHILD);

        RECT rect;

        GetWindowRect(hWnd, &rect);
        SetWindowPos(
            hWnd, nullptr, rect.left, rect.top, rect.right - rect.left + 1,
            rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
        SetWindowPos(
            hWnd, nullptr, rect.left, rect.top, rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
    }

    void WindowManager::SetAsFrameless() {
        is_frameless_ = true;
        HWND hWnd = GetMainWindow();

        RECT rect;

        GetWindowRect(hWnd, &rect);
        SetWindowPos(hWnd, nullptr, rect.left, rect.top, rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE |
            SWP_FRAMECHANGED);
    }

    void WindowManager::WaitUntilReadyToShow() {
        ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&taskbar_));
    }

    void WindowManager::Destroy() {
        PostQuitMessage(0);
    }

    void WindowManager::Close() {
        HWND hWnd = GetMainWindow();
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    void WindowManager::SetPreventClose(const flutter::EncodableMap& args) {
        is_prevent_close_ =
            std::get<bool>(args.at(flutter::EncodableValue("isPreventClose")));
    }

    bool WindowManager::IsPreventClose() {
        return is_prevent_close_;
    }

    void WindowManager::Focus() {
        HWND hWnd = GetMainWindow();
        if (IsMinimized()) {
            Restore();
        }

        ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        SetForegroundWindow(hWnd);
    }

    void WindowManager::Blur() {
        HWND hWnd = GetMainWindow();
        HWND next_hwnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);
        while (next_hwnd) {
            if (::IsWindowVisible(next_hwnd)) {
                ::SetForegroundWindow(next_hwnd);
                return;
            }
            next_hwnd = ::GetNextWindow(next_hwnd, GW_HWNDNEXT);
        }
    }

    bool WindowManager::IsFocused() {
        return GetMainWindow() == GetActiveWindow();
    }

    void WindowManager::Show() {
        HWND hWnd = GetMainWindow();
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        gwlStyle = gwlStyle | WS_VISIBLE;
        if ((gwlStyle & WS_VISIBLE) == 0) {
            SetWindowLong(hWnd, GWL_STYLE, gwlStyle);
            ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        }

        ShowWindowAsync(GetMainWindow(), SW_SHOW);
        SetForegroundWindow(GetMainWindow());
    }

    void WindowManager::Hide() {
        ShowWindow(GetMainWindow(), SW_HIDE);
    }

    bool WindowManager::IsVisible() {
        bool isVisible = IsWindowVisible(GetMainWindow());
        return isVisible;
    }

    bool WindowManager::IsMaximized() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        return windowPlacement.showCmd == SW_MAXIMIZE;
    }

    void WindowManager::Maximize(const flutter::EncodableMap& args) {
        bool vertically =
            std::get<bool>(args.at(flutter::EncodableValue("vertically")));

        HWND hwnd = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(hwnd, &windowPlacement);

        if (vertically) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            PostMessage(hwnd, WM_NCLBUTTONDBLCLK, HTTOP,
                MAKELPARAM(cursorPos.x, cursorPos.y));
        }
        else {
            if (windowPlacement.showCmd != SW_MAXIMIZE) {
                PostMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
        }
    }

    void WindowManager::Unmaximize() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            PostMessage(mainWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
    }

    bool WindowManager::IsMinimized() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        return windowPlacement.showCmd == SW_SHOWMINIMIZED;
    }

    void WindowManager::Minimize() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_SHOWMINIMIZED) {
            PostMessage(mainWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
    }

    void WindowManager::Restore() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            PostMessage(mainWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
    }

    bool WindowManager::IsFullScreen() {
        return g_is_window_fullscreen;
    }

    void WindowManager::SetFullScreen(const flutter::EncodableMap& args) {
        bool isFullScreen =
            std::get<bool>(args.at(flutter::EncodableValue("isFullScreen")));

        HWND mainWindow = GetMainWindow();

        // Inspired by how Chromium does this
        // https://src.chromium.org/viewvc/chrome/trunk/src/ui/views/win/fullscreen_handler.cc?revision=247204&view=markup

        // Save current window state if not already fullscreen.
        if (!g_is_window_fullscreen) {
            // Save current window information.
            g_maximized_before_fullscreen = !!::IsZoomed(mainWindow);
            g_style_before_fullscreen = GetWindowLong(mainWindow, GWL_STYLE);
            g_ex_style_before_fullscreen = GetWindowLong(mainWindow, GWL_EXSTYLE);
            if (g_maximized_before_fullscreen) {
                SendMessage(mainWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            ::GetWindowRect(mainWindow, &g_frame_before_fullscreen);
            g_title_bar_style_before_fullscreen = title_bar_style_;
            g_is_frameless_before_fullscreen = is_frameless_;
        }

        if (isFullScreen) {
            flutter::EncodableMap args2 = flutter::EncodableMap();
            args2[flutter::EncodableValue("titleBarStyle")] =
                flutter::EncodableValue("normal");
            SetTitleBarStyle(args2);

            // Set new window style and size.
            ::SetWindowLong(mainWindow, GWL_STYLE,
                g_style_before_fullscreen & ~(WS_CAPTION | WS_THICKFRAME));
            ::SetWindowLong(mainWindow, GWL_EXSTYLE,
                g_ex_style_before_fullscreen &
                ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                    WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

            MONITORINFO monitor_info;
            monitor_info.cbSize = sizeof(monitor_info);
            ::GetMonitorInfo(::MonitorFromWindow(mainWindow, MONITOR_DEFAULTTONEAREST),
                &monitor_info);
            ::SetWindowPos(mainWindow, NULL, monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.top,
                monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            ::SendMessage(mainWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
        else {
            ::SetWindowLong(mainWindow, GWL_STYLE, g_style_before_fullscreen);
            ::SetWindowLong(mainWindow, GWL_EXSTYLE, g_ex_style_before_fullscreen);

            SendMessage(mainWindow, WM_SYSCOMMAND, SC_RESTORE, 0);

            if (title_bar_style_ != g_title_bar_style_before_fullscreen) {
                flutter::EncodableMap args2 = flutter::EncodableMap();
                args2[flutter::EncodableValue("titleBarStyle")] =
                    flutter::EncodableValue(g_title_bar_style_before_fullscreen);
                SetTitleBarStyle(args2);
            }

            if (g_is_frameless_before_fullscreen)
                SetAsFrameless();

            if (g_maximized_before_fullscreen) {
                flutter::EncodableMap args2 = flutter::EncodableMap();
                args2[flutter::EncodableValue("vertically")] =
                    flutter::EncodableValue(false);
                Maximize(args2);
            }
            else {
                ::SetWindowPos(
                    mainWindow, NULL, g_frame_before_fullscreen.left,
                    g_frame_before_fullscreen.top,
                    g_frame_before_fullscreen.right - g_frame_before_fullscreen.left,
                    g_frame_before_fullscreen.bottom - g_frame_before_fullscreen.top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
        }

        g_is_window_fullscreen = isFullScreen;
    }

    void WindowManager::SetAspectRatio(const flutter::EncodableMap& args) {
        aspect_ratio_ =
            std::get<double>(args.at(flutter::EncodableValue("aspectRatio")));
    }

    void WindowManager::SetBackgroundColor(const flutter::EncodableMap& args) {
        int backgroundColorA =
            std::get<int>(args.at(flutter::EncodableValue("backgroundColorA")));
        int backgroundColorR =
            std::get<int>(args.at(flutter::EncodableValue("backgroundColorR")));
        int backgroundColorG =
            std::get<int>(args.at(flutter::EncodableValue("backgroundColorG")));
        int backgroundColorB =
            std::get<int>(args.at(flutter::EncodableValue("backgroundColorB")));

        bool isTransparent = backgroundColorA == 0 && backgroundColorR == 0 &&
            backgroundColorG == 0 && backgroundColorB == 0;

        HWND hWnd = GetMainWindow();
        const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
        if (hModule) {
            typedef enum _ACCENT_STATE {
                ACCENT_DISABLED = 0,
                ACCENT_ENABLE_GRADIENT = 1,
                ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
                ACCENT_ENABLE_BLURBEHIND = 3,
                ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
                ACCENT_ENABLE_HOSTBACKDROP = 5,
                ACCENT_INVALID_STATE = 6
            } ACCENT_STATE;
            struct ACCENTPOLICY {
                int nAccentState;
                int nFlags;
                int nColor;
                int nAnimationId;
            };
            struct WINCOMPATTRDATA {
                int nAttribute;
                PVOID pData;
                ULONG ulDataSize;
            };
            typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND,
                WINCOMPATTRDATA*);
            const pSetWindowCompositionAttribute SetWindowCompositionAttribute =
                (pSetWindowCompositionAttribute)GetProcAddress(
                    hModule, "SetWindowCompositionAttribute");
            if (SetWindowCompositionAttribute) {
                int32_t accent_state = isTransparent ? ACCENT_ENABLE_TRANSPARENTGRADIENT
                    : ACCENT_ENABLE_GRADIENT;
                ACCENTPOLICY policy = {
                    accent_state, 2,
                    ((backgroundColorA << 24) + (backgroundColorB << 16) +
                     (backgroundColorG << 8) + (backgroundColorR)),
                    0 };
                WINCOMPATTRDATA data = { 19, &policy, sizeof(policy) };
                SetWindowCompositionAttribute(hWnd, &data);
            }
            FreeLibrary(hModule);
        }
    }

    flutter::EncodableMap WindowManager::GetBounds(
        const flutter::EncodableMap& args) {
        HWND hwnd = GetMainWindow();
        double devicePixelRatio =
            std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));

        flutter::EncodableMap resultMap = flutter::EncodableMap();
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            double x = rect.left / devicePixelRatio * 1.0f;
            double y = rect.top / devicePixelRatio * 1.0f;
            double width = (rect.right - rect.left) / devicePixelRatio * 1.0f;
            double height = (rect.bottom - rect.top) / devicePixelRatio * 1.0f;

            resultMap[flutter::EncodableValue("x")] = flutter::EncodableValue(x);
            resultMap[flutter::EncodableValue("y")] = flutter::EncodableValue(y);
            resultMap[flutter::EncodableValue("width")] =
                flutter::EncodableValue(width);
            resultMap[flutter::EncodableValue("height")] =
                flutter::EncodableValue(height);
        }
        return resultMap;
    }

    void WindowManager::SetBounds(const flutter::EncodableMap& args) {
        HWND hwnd = GetMainWindow();

        double devicePixelRatio =
            std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));

        auto* null_or_x = std::get_if<double>(ValueOrNull(args, "x"));
        auto* null_or_y = std::get_if<double>(ValueOrNull(args, "y"));
        auto* null_or_width = std::get_if<double>(ValueOrNull(args, "width"));
        auto* null_or_height = std::get_if<double>(ValueOrNull(args, "height"));

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        UINT uFlags = NULL;

        if (null_or_x != nullptr && null_or_y != nullptr) {
            x = static_cast<int>(*null_or_x * devicePixelRatio);
            y = static_cast<int>(*null_or_y * devicePixelRatio);
        }
        if (null_or_width != nullptr && null_or_height != nullptr) {
            width = static_cast<int>(*null_or_width * devicePixelRatio);
            height = static_cast<int>(*null_or_height * devicePixelRatio);
        }

        if (null_or_x == nullptr || null_or_y == nullptr) {
            uFlags = SWP_NOMOVE;
        }
        if (null_or_width == nullptr || null_or_height == nullptr) {
            uFlags = SWP_NOSIZE;
        }

        SetWindowPos(hwnd, HWND_TOP, x, y, width, height, uFlags);
    }

    void WindowManager::SetMinimumSize(const flutter::EncodableMap& args) {
        double devicePixelRatio =
            std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            minimum_size_ = point;
        }
    }

    void WindowManager::SetMaximumSize(const flutter::EncodableMap& args) {
        double devicePixelRatio =
            std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            maximum_size_ = point;
        }
    }

    bool WindowManager::IsResizable() {
        return is_resizable_;
    }

    void WindowManager::SetResizable(const flutter::EncodableMap& args) {
        HWND hWnd = GetMainWindow();
        is_resizable_ =
            std::get<bool>(args.at(flutter::EncodableValue("isResizable")));
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        if (is_resizable_) {
            gwlStyle |= WS_THICKFRAME;
        }
        else {
            gwlStyle &= ~WS_THICKFRAME;
        }
        ::SetWindowLong(hWnd, GWL_STYLE, gwlStyle);
    }

    bool WindowManager::IsMinimizable() {
        HWND hWnd = GetMainWindow();
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        return (gwlStyle & WS_MINIMIZEBOX) != 0;
    }

    void WindowManager::SetMinimizable(const flutter::EncodableMap& args) {
        HWND hWnd = GetMainWindow();
        bool isMinimizable =
            std::get<bool>(args.at(flutter::EncodableValue("isMinimizable")));
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        gwlStyle =
            isMinimizable ? gwlStyle | WS_MINIMIZEBOX : gwlStyle & ~WS_MINIMIZEBOX;
        SetWindowLong(hWnd, GWL_STYLE, gwlStyle);
    }

    bool WindowManager::IsMaximizable() {
        HWND hWnd = GetMainWindow();
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        return (gwlStyle & WS_MAXIMIZEBOX) != 0;
    }

    void WindowManager::SetMaximizable(const flutter::EncodableMap& args) {
        HWND hWnd = GetMainWindow();
        bool isMaximizable =
            std::get<bool>(args.at(flutter::EncodableValue("isMaximizable")));
        DWORD gwlStyle = GetWindowLong(hWnd, GWL_STYLE);
        gwlStyle =
            isMaximizable ? gwlStyle | WS_MAXIMIZEBOX : gwlStyle & ~WS_MAXIMIZEBOX;
        SetWindowLong(hWnd, GWL_STYLE, gwlStyle);
    }

    bool WindowManager::IsClosable() {
        HWND hWnd = GetMainWindow();
        DWORD gclStyle = GetClassLong(hWnd, GCL_STYLE);
        return !((gclStyle & CS_NOCLOSE) != 0);
    }

    void WindowManager::SetClosable(const flutter::EncodableMap& args) {
        HWND hWnd = GetMainWindow();
        bool isClosable =
            std::get<bool>(args.at(flutter::EncodableValue("isClosable")));
        DWORD gclStyle = GetClassLong(hWnd, GCL_STYLE);
        gclStyle = isClosable ? gclStyle & ~CS_NOCLOSE : gclStyle | CS_NOCLOSE;
        SetClassLong(hWnd, GCL_STYLE, gclStyle);
    }

    bool WindowManager::IsAlwaysOnTop() {
        DWORD dwExStyle = GetWindowLong(GetMainWindow(), GWL_EXSTYLE);
        return (dwExStyle & WS_EX_TOPMOST) != 0;
    }

    void WindowManager::SetAlwaysOnTop(const flutter::EncodableMap& args) {
        bool isAlwaysOnTop =
            std::get<bool>(args.at(flutter::EncodableValue("isAlwaysOnTop")));
        SetWindowPos(GetMainWindow(), isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    std::string WindowManager::GetTitle() {
        int const bufferSize = 1 + GetWindowTextLength(GetMainWindow());
        std::wstring title(bufferSize, L'\0');
        GetWindowText(GetMainWindow(), &title[0], bufferSize);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return (converter.to_bytes(title)).c_str();
    }

    void WindowManager::SetTitle(const flutter::EncodableMap& args) {
        std::string title =
            std::get<std::string>(args.at(flutter::EncodableValue("title")));

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        SetWindowText(GetMainWindow(), converter.from_bytes(title).c_str());
    }

    void WindowManager::SetTitleBarStyle(const flutter::EncodableMap& args) {
        title_bar_style_ =
            std::get<std::string>(args.at(flutter::EncodableValue("titleBarStyle")));
        // Enables the ability to go from setAsFrameless() to
        // TitleBarStyle.normal/hidden
        is_frameless_ = false;

        MARGINS margins = { 0, 0, 0, 0 };
        HWND hWnd = GetMainWindow();
        RECT rect;
        GetWindowRect(hWnd, &rect);
        DwmExtendFrameIntoClientArea(hWnd, &margins);
        SetWindowPos(hWnd, nullptr, rect.left, rect.top, 0, 0,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE |
            SWP_FRAMECHANGED);
    }

    int WindowManager::GetTitleBarHeight() {
        HWND hWnd = GetMainWindow();

        TITLEBARINFOEX* ptinfo = (TITLEBARINFOEX*)malloc(sizeof(TITLEBARINFOEX));
        ptinfo->cbSize = sizeof(TITLEBARINFOEX);
        SendMessage(hWnd, WM_GETTITLEBARINFOEX, 0, (LPARAM)ptinfo);
        int height = ptinfo->rcTitleBar.bottom == 0
            ? 0
            : ptinfo->rcTitleBar.bottom - ptinfo->rcTitleBar.top;
        free(ptinfo);

        return height;
    }

    bool WindowManager::IsSkipTaskbar() {
        return is_skip_taskbar_;
    }

    void WindowManager::SetSkipTaskbar(const flutter::EncodableMap& args) {
        is_skip_taskbar_ =
            std::get<bool>(args.at(flutter::EncodableValue("isSkipTaskbar")));

        HWND hWnd = GetMainWindow();

        LPVOID lp = NULL;
        CoInitialize(lp);

        taskbar_->HrInit();
        if (!is_skip_taskbar_)
            taskbar_->AddTab(hWnd);
        else
            taskbar_->DeleteTab(hWnd);
    }

    void WindowManager::SetProgressBar(const flutter::EncodableMap& args) {
        double progress =
            std::get<double>(args.at(flutter::EncodableValue("progress")));

        HWND hWnd = GetMainWindow();
        taskbar_->SetProgressState(hWnd, TBPF_INDETERMINATE);
        taskbar_->SetProgressValue(hWnd, static_cast<int32_t>(progress * 100),
            static_cast<int32_t>(100));

        if (progress < 0) {
            taskbar_->SetProgressState(hWnd, TBPF_NOPROGRESS);
            taskbar_->SetProgressValue(hWnd, static_cast<int32_t>(0),
                static_cast<int32_t>(0));
        }
        else if (progress > 1) {
            taskbar_->SetProgressState(hWnd, TBPF_INDETERMINATE);
            taskbar_->SetProgressValue(hWnd, static_cast<int32_t>(100),
                static_cast<int32_t>(100));
        }
        else {
            taskbar_->SetProgressState(hWnd, TBPF_INDETERMINATE);
            taskbar_->SetProgressValue(hWnd, static_cast<int32_t>(progress * 100),
                static_cast<int32_t>(100));
        }
    }

    void WindowManager::SetIcon(const flutter::EncodableMap& args) {
        std::string iconPath =
            std::get<std::string>(args.at(flutter::EncodableValue("iconPath")));

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        HICON hIconSmall =
            (HICON)(LoadImage(NULL, (LPCWSTR)(converter.from_bytes(iconPath).c_str()),
                IMAGE_ICON, 16, 16, LR_LOADFROMFILE));

        HICON hIconLarge =
            (HICON)(LoadImage(NULL, (LPCWSTR)(converter.from_bytes(iconPath).c_str()),
                IMAGE_ICON, 32, 32, LR_LOADFROMFILE));

        HWND hWnd = GetMainWindow();

        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconLarge);
    }

    bool WindowManager::HasShadow() {
        if (is_frameless_)
            return has_shadow_;
        return true;
    }

    void WindowManager::SetHasShadow(const flutter::EncodableMap& args) {
        if (is_frameless_) {
            has_shadow_ = std::get<bool>(args.at(flutter::EncodableValue("hasShadow")));

            HWND hWnd = GetMainWindow();

            MARGINS margins[2]{ {0, 0, 0, 0}, {0, 0, 1, 0} };

            DwmExtendFrameIntoClientArea(hWnd, &margins[has_shadow_]);
        }
    }

    double WindowManager::GetOpacity() {
        return opacity_;
    }

    void WindowManager::SetOpacity(const flutter::EncodableMap& args) {
        opacity_ = std::get<double>(args.at(flutter::EncodableValue("opacity")));
        HWND hWnd = GetMainWindow();
        long gwlExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        SetWindowLong(hWnd, GWL_EXSTYLE, gwlExStyle | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, 0, static_cast<int8_t>(255 * opacity_),
            0x02);
    }

    void WindowManager::SetBrightness(const flutter::EncodableMap& args) {
        std::string brightness =
            std::get<std::string>(args.at(flutter::EncodableValue("brightness")));

        const BOOL is_dark_mode = brightness == "dark";

        HWND hWnd = GetMainWindow();
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &is_dark_mode,
            sizeof(is_dark_mode));
    }

    void WindowManager::SetIgnoreMouseEvents(const flutter::EncodableMap& args) {
        bool ignore = std::get<bool>(args.at(flutter::EncodableValue("ignore")));

        HWND hwnd = GetMainWindow();
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        if (ignore)
            ex_style |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
        else
            ex_style &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);

        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    void WindowManager::PopUpWindowMenu(const flutter::EncodableMap& args) {
        HWND hWnd = GetMainWindow();
        HMENU hMenu = GetSystemMenu(hWnd, false);

        double x, y;

        POINT cursorPos;
        GetCursorPos(&cursorPos);
        x = cursorPos.x;
        y = cursorPos.y;

        int cmd =
            TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                static_cast<int>(x), static_cast<int>(y), 0, hWnd, NULL);

        if (cmd) {
            PostMessage(hWnd, WM_SYSCOMMAND, cmd, 0);
        }
    }

    void WindowManager::StartDragging() {
        ReleaseCapture();
        SendMessage(GetMainWindow(), WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
    }

    void WindowManager::StartResizing(const flutter::EncodableMap& args) {
        bool top = std::get<bool>(args.at(flutter::EncodableValue("top")));
        bool bottom = std::get<bool>(args.at(flutter::EncodableValue("bottom")));
        bool left = std::get<bool>(args.at(flutter::EncodableValue("left")));
        bool right = std::get<bool>(args.at(flutter::EncodableValue("right")));

        HWND hWnd = GetMainWindow();
        ReleaseCapture();
        LONG command;
        if (top && !bottom && !right && !left) {
            command = HTTOP;
        }
        else if (top && left && !bottom && !right) {
            command = HTTOPLEFT;
        }
        else if (left && !top && !bottom && !right) {
            command = HTLEFT;
        }
        else if (right && !top && !left && !bottom) {
            command = HTRIGHT;
        }
        else if (top && right && !left && !bottom) {
            command = HTTOPRIGHT;
        }
        else if (bottom && !top && !right && !left) {
            command = HTBOTTOM;
        }
        else if (bottom && left && !top && !right) {
            command = HTBOTTOMLEFT;
        }
        else
            command = HTBOTTOMRIGHT;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        PostMessage(hWnd, WM_NCLBUTTONDOWN, command,
            MAKELPARAM(cursorPos.x, cursorPos.y));
    }

    bool IsWindows11OrGreater() {
        DWORD dwVersion = 0;
        DWORD dwBuild = 0;

#pragma warning(push)
#pragma warning(disable : 4996)
        dwVersion = GetVersion();
        // Get the build number.
        if (dwVersion < 0x80000000)
            dwBuild = (DWORD)(HIWORD(dwVersion));
#pragma warning(pop)

        return dwBuild < 22000;
    }

    std::unique_ptr<
        flutter::MethodChannel<flutter::EncodableValue>,
        std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>>
        channel = nullptr;

    class WindowManagerPlugin : public flutter::Plugin {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

        WindowManagerPlugin(flutter::PluginRegistrarWindows* registrar);

        virtual ~WindowManagerPlugin();

    private:
        WindowManager* window_manager;
        flutter::PluginRegistrarWindows* registrar;

        // The ID of the WindowProc delegate registration.
        int window_proc_id = -1;

        void WindowManagerPlugin::_EmitEvent(std::string eventName);
        // Called for top-level WindowProc delegation.
        std::optional<LRESULT> WindowManagerPlugin::HandleWindowProc(HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);
        // Called when a method is called on this plugin's channel from Dart.
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    };

    // static
    void WindowManagerPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarWindows* registrar) {
        channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "window_manager",
            &flutter::StandardMethodCodec::GetInstance());

        auto plugin = std::make_unique<WindowManagerPlugin>(registrar);

        channel->SetMethodCallHandler(
            [plugin_pointer = plugin.get()](const auto& call, auto result) {
                plugin_pointer->HandleMethodCall(call, std::move(result));
            });

        registrar->AddPlugin(std::move(plugin));
    }

    WindowManagerPlugin::WindowManagerPlugin(
        flutter::PluginRegistrarWindows* registrar)
        : registrar(registrar) {
        window_manager = new WindowManager();
        window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
            [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
                return HandleWindowProc(hWnd, message, wParam, lParam);
            });
    }

    WindowManagerPlugin::~WindowManagerPlugin() {
        registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
    }

    void WindowManagerPlugin::_EmitEvent(std::string eventName) {
        flutter::EncodableMap args = flutter::EncodableMap();
        args[flutter::EncodableValue("eventName")] =
            flutter::EncodableValue(eventName);
        channel->InvokeMethod("onEvent",
            std::make_unique<flutter::EncodableValue>(args));
    }

    std::optional<LRESULT> WindowManagerPlugin::HandleWindowProc(HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam) {
        std::optional<LRESULT> result = std::nullopt;

        if (message == WM_NCCALCSIZE) {
            // This must always be first or else the one of other two ifs will execute
            //  when window is in full screen and we don't want that
            if (wParam && window_manager->IsFullScreen()) {
                NCCALCSIZE_PARAMS* sz = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                sz->rgrc[0].bottom -= 3;
                return 0;
            }

            // This must always be before handling title_bar_style_ == "hidden" so
            //  the if TitleBarStyle.hidden doesn't get executed.
            if (wParam && window_manager->is_frameless_) {
                NCCALCSIZE_PARAMS* sz = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                // Add borders when maximized so app doesn't get cut off.
                if (window_manager->IsMaximized()) {
                    sz->rgrc[0].left += 8;
                    sz->rgrc[0].top += 8;
                    sz->rgrc[0].right -= 8;
                    sz->rgrc[0].bottom -= 9;
                }
                // This cuts the app at the bottom by one pixel but that's necessary to
                // prevent jitter when resizing the app
                sz->rgrc[0].bottom += 1;
                return 0;
            }

            // This must always be last.
            if (wParam && window_manager->title_bar_style_ == "hidden") {
                NCCALCSIZE_PARAMS* sz = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

                // Add 8 pixel to the top border when maximized so the app isn't cut off
                if (window_manager->IsMaximized()) {
                    sz->rgrc[0].top += 8;
                }
                else {
                    // on windows 10, if set to 0, there's a white line at the top
                    // of the app and I've yet to find a way to remove that.
                    sz->rgrc[0].top += IsWindows11OrGreater() ? 0 : 1;
                }
                sz->rgrc[0].right -= 8;
                sz->rgrc[0].bottom -= 8;
                sz->rgrc[0].left -= -8;

                // Previously (WVR_HREDRAW | WVR_VREDRAW), but returning 0 or 1 doesn't
                // actually break anything so I've set it to 0. Unless someone pointed a
                // problem in the future.
                return 0;
            }
        }
        else if (message == WM_NCHITTEST) {
            if (!window_manager->is_resizable_) {
                return HTNOWHERE;
            }
        }
        else if (message == WM_GETMINMAXINFO) {
            MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
            // For the special "unconstrained" values, leave the defaults.
            if (window_manager->minimum_size_.x != 0)
                info->ptMinTrackSize.x = window_manager->minimum_size_.x;
            if (window_manager->minimum_size_.y != 0)
                info->ptMinTrackSize.y = window_manager->minimum_size_.y;
            if (window_manager->maximum_size_.x != -1)
                info->ptMaxTrackSize.x = window_manager->maximum_size_.x;
            if (window_manager->maximum_size_.y != -1)
                info->ptMaxTrackSize.y = window_manager->maximum_size_.y;
            result = 0;
        }
        else if (message == WM_NCACTIVATE) {
            if (wParam == TRUE) {
                _EmitEvent("focus");
            }
            else {
                _EmitEvent("blur");
            }

            if (window_manager->title_bar_style_ == "hidden" ||
                window_manager->is_frameless_)
                return 1;
        }
        else if (message == WM_EXITSIZEMOVE) {
            if (window_manager->is_resizing_) {
                _EmitEvent("resized");
                window_manager->is_resizing_ = false;
            }
            if (window_manager->is_moving_) {
                _EmitEvent("moved");
                window_manager->is_moving_ = false;
            }
            return false;
        }
        else if (message == WM_MOVING) {
            window_manager->is_moving_ = true;
            _EmitEvent("move");
            return false;
        }
        else if (message == WM_SIZING) {
            window_manager->is_resizing_ = true;
            _EmitEvent("resize");

            if (window_manager->aspect_ratio_ > 0) {
                RECT* rect = (LPRECT)lParam;

                double aspect_ratio = window_manager->aspect_ratio_;

                int new_width = static_cast<int>(rect->right - rect->left);
                int new_height = static_cast<int>(rect->bottom - rect->top);

                bool is_resizing_horizontally =
                    wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT ||
                    wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT;

                if (is_resizing_horizontally) {
                    new_height = static_cast<int>(new_width / aspect_ratio);
                }
                else {
                    new_width = static_cast<int>(new_height * aspect_ratio);
                }

                int left = rect->left;
                int top = rect->top;
                int right = rect->right;
                int bottom = rect->bottom;

                switch (wParam) {
                case WMSZ_RIGHT:
                case WMSZ_BOTTOM:
                    right = new_width + left;
                    bottom = top + new_height;
                    break;
                case WMSZ_TOP:
                    right = new_width + left;
                    top = bottom - new_height;
                    break;
                case WMSZ_LEFT:
                case WMSZ_TOPLEFT:
                    left = right - new_width;
                    top = bottom - new_height;
                    break;
                case WMSZ_TOPRIGHT:
                    right = left + new_width;
                    top = bottom - new_height;
                    break;
                case WMSZ_BOTTOMLEFT:
                    left = right - new_width;
                    bottom = top + new_height;
                    break;
                case WMSZ_BOTTOMRIGHT:
                    right = left + new_width;
                    bottom = top + new_height;
                    break;
                }

                rect->left = left;
                rect->top = top;
                rect->right = right;
                rect->bottom = bottom;
            }
        }
        else if (message == WM_SIZE) {
            LONG_PTR gwlStyle =
                GetWindowLongPtr(window_manager->GetMainWindow(), GWL_STYLE);
            if ((gwlStyle & (WS_CAPTION | WS_THICKFRAME)) == 0 &&
                wParam == SIZE_MAXIMIZED) {
                _EmitEvent("enter-full-screen");
                window_manager->last_state = STATE_FULLSCREEN_ENTERED;
            }
            else if (window_manager->last_state == STATE_FULLSCREEN_ENTERED &&
                wParam == SIZE_RESTORED) {
                window_manager->ForceChildRefresh();
                _EmitEvent("leave-full-screen");
                window_manager->last_state = STATE_NORMAL;
            }
            else if (wParam == SIZE_MAXIMIZED) {
                _EmitEvent("maximize");
                window_manager->last_state = STATE_MAXIMIZED;
            }
            else if (wParam == SIZE_MINIMIZED) {
                _EmitEvent("minimize");
                window_manager->last_state = STATE_MINIMIZED;
                return 0;
            }
            else if (wParam == SIZE_RESTORED) {
                if (window_manager->last_state == STATE_MAXIMIZED) {
                    _EmitEvent("unmaximize");
                    window_manager->last_state = STATE_NORMAL;
                }
                else if (window_manager->last_state == STATE_MINIMIZED) {
                    _EmitEvent("restore");
                    window_manager->last_state = STATE_NORMAL;
                }
            }
        }
        else if (message == WM_CLOSE) {
            _EmitEvent("close");
            if (window_manager->IsPreventClose()) {
                return -1;
            }
        }
        return result;
    }

    void WindowManagerPlugin::HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        std::string method_name = method_call.method_name();

        if (method_name.compare("ensureInitialized") == 0) {
            window_manager->native_window =
                ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("waitUntilReadyToShow") == 0) {
            window_manager->WaitUntilReadyToShow();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setAsFrameless") == 0) {
            window_manager->SetAsFrameless();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("destroy") == 0) {
            window_manager->Destroy();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("close") == 0) {
            window_manager->Close();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isPreventClose") == 0) {
            auto value = window_manager->IsPreventClose();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setPreventClose") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetPreventClose(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("focus") == 0) {
            window_manager->Focus();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("blur") == 0) {
            window_manager->Blur();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isFocused") == 0) {
            bool value = window_manager->IsFocused();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("show") == 0) {
            window_manager->Show();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("hide") == 0) {
            window_manager->Hide();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isVisible") == 0) {
            bool value = window_manager->IsVisible();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("isMaximized") == 0) {
            bool value = window_manager->IsMaximized();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("maximize") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->Maximize(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("unmaximize") == 0) {
            window_manager->Unmaximize();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isMinimized") == 0) {
            bool value = window_manager->IsMinimized();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("minimize") == 0) {
            window_manager->Minimize();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("restore") == 0) {
            window_manager->Restore();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isFullScreen") == 0) {
            bool value = window_manager->IsFullScreen();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setFullScreen") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetFullScreen(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setAspectRatio") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetAspectRatio(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setBackgroundColor") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetBackgroundColor(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("getBounds") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            flutter::EncodableMap value = window_manager->GetBounds(args);
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setBounds") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetBounds(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setMinimumSize") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetMinimumSize(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setMaximumSize") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetMaximumSize(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isResizable") == 0) {
            bool value = window_manager->IsResizable();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setResizable") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetResizable(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isMinimizable") == 0) {
            bool value = window_manager->IsMinimizable();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setMinimizable") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetMinimizable(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isMaximizable") == 0) {
            bool value = window_manager->IsMaximizable();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setMaximizable") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetMaximizable(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isClosable") == 0) {
            bool value = window_manager->IsClosable();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setClosable") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetClosable(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("isAlwaysOnTop") == 0) {
            bool value = window_manager->IsAlwaysOnTop();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setAlwaysOnTop") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetAlwaysOnTop(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("getTitle") == 0) {
            std::string value = window_manager->GetTitle();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setTitle") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetTitle(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setTitleBarStyle") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetTitleBarStyle(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("getTitleBarHeight") == 0) {
            int value = window_manager->GetTitleBarHeight();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("isSkipTaskbar") == 0) {
            bool value = window_manager->IsSkipTaskbar();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setSkipTaskbar") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetSkipTaskbar(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setProgressBar") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetProgressBar(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setIcon") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetIcon(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("hasShadow") == 0) {
            bool value = window_manager->HasShadow();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setHasShadow") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetHasShadow(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("getOpacity") == 0) {
            double value = window_manager->GetOpacity();
            result->Success(flutter::EncodableValue(value));
        }
        else if (method_name.compare("setOpacity") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetOpacity(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setBrightness") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetBrightness(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("setIgnoreMouseEvents") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->SetIgnoreMouseEvents(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("popUpWindowMenu") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->PopUpWindowMenu(args);
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("startDragging") == 0) {
            window_manager->StartDragging();
            result->Success(flutter::EncodableValue(true));
        }
        else if (method_name.compare("startResizing") == 0) {
            const flutter::EncodableMap& args =
                std::get<flutter::EncodableMap>(*method_call.arguments());
            window_manager->StartResizing(args);
            result->Success(flutter::EncodableValue(true));
        }
        else {
            result->NotImplemented();
        }
    }

void WindowManagerPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
    WindowManagerPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarManager::GetInstance()
        ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

}

namespace ScreenOriginPlugin {

    ///////////////////   Screen Plugin
    const double kBaseDpi = 96.0;

    std::unique_ptr<
        flutter::MethodChannel<flutter::EncodableValue>,
        std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>>
        channel = nullptr;

    class ScreenRetrieverPlugin : public flutter::Plugin {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

        ScreenRetrieverPlugin();

        virtual ~ScreenRetrieverPlugin();

    private:
        flutter::PluginRegistrarWindows* registrar;

        void ScreenRetrieverPlugin::_EmitEvent(std::string eventName);

        HWND ScreenRetrieverPlugin::GetMainWindow();
        void ScreenRetrieverPlugin::GetCursorScreenPoint(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void ScreenRetrieverPlugin::GetPrimaryDisplay(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void ScreenRetrieverPlugin::GetAllDisplays(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        // Called when a method is called on this plugin's channel from Dart.
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    };

    // static
    void ScreenRetrieverPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarWindows* registrar) {
        channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "screen_retriever",
            &flutter::StandardMethodCodec::GetInstance());

        auto plugin = std::make_unique<ScreenRetrieverPlugin>();

        channel->SetMethodCallHandler(
            [plugin_pointer = plugin.get()](const auto& call, auto result) {
                plugin_pointer->HandleMethodCall(call, std::move(result));
            });

        registrar->AddPlugin(std::move(plugin));
    }

    ScreenRetrieverPlugin::ScreenRetrieverPlugin() {}

    ScreenRetrieverPlugin::~ScreenRetrieverPlugin() {}

    void ScreenRetrieverPlugin::_EmitEvent(std::string eventName) {
        flutter::EncodableMap args = flutter::EncodableMap();
        args[flutter::EncodableValue("eventName")] =
            flutter::EncodableValue(eventName);
        channel->InvokeMethod("onEvent",
            std::make_unique<flutter::EncodableValue>(args));
    }

    HWND ScreenRetrieverPlugin::GetMainWindow() {
        return ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
    }

    flutter::EncodableMap MonitorToEncodableMap(HMONITOR monitor) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        MONITORINFOEX info;
        info.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(monitor, &info);
        UINT dpi = FlutterDesktopGetDpiForMonitor(monitor);

        wchar_t display_name[sizeof(info.szDevice) / sizeof(*info.szDevice) + 1];
        memset(display_name, 0, sizeof(display_name));
        memcpy(display_name, info.szDevice, sizeof(info.szDevice));

        double scale_factor = dpi / kBaseDpi;

        double visibleWidth =
            round((info.rcWork.right - info.rcWork.left) / scale_factor);
        double visibleHeight =
            round((info.rcWork.bottom - info.rcWork.top) / scale_factor);

        double visibleX = round((info.rcWork.left) / scale_factor);
        double visibleY = round((info.rcWork.top) / scale_factor);

        flutter::EncodableMap size = flutter::EncodableMap();
        flutter::EncodableMap visibleSize = flutter::EncodableMap();
        flutter::EncodableMap visiblePosition = flutter::EncodableMap();

        size[flutter::EncodableValue("width")] = flutter::EncodableValue(
            static_cast<double>(round(info.rcMonitor.right / scale_factor)));
        size[flutter::EncodableValue("height")] = flutter::EncodableValue(
            static_cast<double>(round(info.rcMonitor.bottom / scale_factor)));

        visibleSize[flutter::EncodableValue("width")] =
            flutter::EncodableValue(visibleWidth);
        visibleSize[flutter::EncodableValue("height")] =
            flutter::EncodableValue(visibleHeight);

        visiblePosition[flutter::EncodableValue("x")] =
            flutter::EncodableValue(visibleX);
        visiblePosition[flutter::EncodableValue("y")] =
            flutter::EncodableValue(visibleY);

        flutter::EncodableMap display = flutter::EncodableMap();

        display[flutter::EncodableValue("id")] = flutter::EncodableValue(0);
        display[flutter::EncodableValue("name")] =
            flutter::EncodableValue(converter.to_bytes(display_name).c_str());
        display[flutter::EncodableValue("size")] = flutter::EncodableValue(size);
        display[flutter::EncodableValue("visibleSize")] =
            flutter::EncodableValue(visibleSize);
        display[flutter::EncodableValue("visiblePosition")] =
            flutter::EncodableValue(visiblePosition);
        display[flutter::EncodableValue("scaleFactor")] =
            flutter::EncodableValue(scale_factor);

        return display;
    }

    BOOL CALLBACK MonitorRepresentationEnumProc(HMONITOR monitor,
        HDC hdc,
        LPRECT clip,
        LPARAM list_ref) {
        flutter::EncodableValue* monitors =
            reinterpret_cast<flutter::EncodableValue*>(list_ref);
        flutter::EncodableMap display = MonitorToEncodableMap(monitor);
        std::get<flutter::EncodableList>(*monitors).push_back(display);
        return TRUE;
    }

    void ScreenRetrieverPlugin::GetCursorScreenPoint(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args =
            std::get<flutter::EncodableMap>(*method_call.arguments());

        double device_pixel_ratio =
            std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));

        double x, y;
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        x = cursorPos.x / device_pixel_ratio * 1.0f;
        y = cursorPos.y / device_pixel_ratio * 1.0f;

        flutter::EncodableMap result_data = flutter::EncodableMap();
        result_data[flutter::EncodableValue("x")] = flutter::EncodableValue(x);
        result_data[flutter::EncodableValue("y")] = flutter::EncodableValue(y);

        result->Success(flutter::EncodableValue(result_data));
    }

    void ScreenRetrieverPlugin::GetPrimaryDisplay(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        POINT ptZero = { 0, 0 };
        HMONITOR monitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
        flutter::EncodableMap display = MonitorToEncodableMap(monitor);
        result->Success(flutter::EncodableValue(display));
    }

    void ScreenRetrieverPlugin::GetAllDisplays(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        flutter::EncodableValue displays(std::in_place_type<flutter::EncodableList>);

        ::EnumDisplayMonitors(nullptr, nullptr, MonitorRepresentationEnumProc,
            reinterpret_cast<LPARAM>(&displays));

        flutter::EncodableMap result_data = flutter::EncodableMap();
        result_data[flutter::EncodableValue("displays")] = displays;

        result->Success(result_data);
    }

    void ScreenRetrieverPlugin::HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        if (method_call.method_name().compare("getCursorScreenPoint") == 0) {
            GetCursorScreenPoint(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("getPrimaryDisplay") == 0) {
            GetPrimaryDisplay(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("getAllDisplays") == 0) {
            GetAllDisplays(method_call, std::move(result));
        }
        else {
            result->NotImplemented();
        }
    }
    void ScreenRetrieverPluginRegisterWithRegistrar(
        FlutterDesktopPluginRegistrarRef registrar) {
        ScreenRetrieverPlugin::RegisterWithRegistrar(
            flutter::PluginRegistrarManager::GetInstance()
            ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
    }
}