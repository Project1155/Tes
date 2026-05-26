#include "globals.h"

// Window & instance
TCHAR  szWindowClass[]   = APP_NAME;
TCHAR  szTitle[]         = APP_TITLE;

HINSTANCE g_hInst   = nullptr;
HWND      g_hWnd    = nullptr;
HBRUSH    g_darkBrush = nullptr;
HANDLE    g_hMutex  = nullptr;
HHOOK     g_hMouseHook = nullptr;

std::vector<std::wstring> g_webuiUrls = {
    L"https://stremio.zarg.me/",
    L"https://zaarrg.github.io/stremio-web-shell-fixes/",
    L"https://web.stremio.com/"
};
std::vector<std::wstring> g_domainWhitelist;
std::string  g_updateUrl= "https://raw.githubusercontent.com/Project1155/DarkFlix/refs/heads/main/version/version.json";
std::wstring  g_extensionsDetailsUrl= L"https://raw.githubusercontent.com/Project1155/DarkFlix/refs/heads/main/extensions/extensions.json";
std::wstring  g_webuiUrl;

// Command-line args
bool g_streamingServer      = true;
bool g_autoupdaterForceFull = false;

// mpv
mpv_handle* g_mpv = nullptr;
std::set<std::string> g_observedProps;
bool g_initialSet = false;
std::string g_initialVO = "gpu-next";
int g_currentVolume = 50;
const std::vector<std::wstring> g_subtitleExtensions = {
    L".srt", L".ass", L".ssa", L".sub", L".vtt", L".ttml",
    L".dfxp", L".smi", L".sami", L".sup", L".scc",
    L".xml", L".lrc", L".pjs", L".mpl", L".usf",
    L".qtvr"
};
// Node
std::atomic_bool g_nodeRunning = false;
std::thread      g_nodeThread;
HANDLE           g_nodeProcess = nullptr;
HANDLE           g_nodeOutPipe = nullptr;
HANDLE           g_nodeInPipe  = nullptr;

// DarkFlix Auth server
std::atomic_bool g_authRunning = false;
HANDLE           g_authProcess = nullptr;

// WebView2
wil::com_ptr<ICoreWebView2Controller4> g_webviewController;
wil::com_ptr<ICoreWebView2Profile8>    g_webviewProfile;
wil::com_ptr<ICoreWebView2_21>         g_webview;

// Tray
std::vector<MenuItem> g_menuItems;
NOTIFYICONDATA  g_nid        = {0};
bool            g_showWindow = true;
bool            g_alwaysOnTop= false;
bool            g_isFullscreen = false;
bool            g_closeOnExit = false;
bool            g_useDarkTheme = false;
bool            g_isPipMode = false;
int             g_thumbFastHeight = 0;
int             g_hoverIndex = -1;
HFONT           g_hMenuFont = nullptr;
HANDLE          g_serverJob  = nullptr;
HWND            g_trayHwnd   = nullptr;

// Ini Settings
bool g_pauseOnMinimize   = true;
bool g_pauseOnLostFocus  = false;
bool g_allowZoom         = false;
bool g_isRpcOn = true;

// Tray sizes
int g_tray_itemH = 31;
int g_tray_sepH  = 8;
int g_tray_w     = 200;
int g_font_height    = 12;

// Splash
HWND       g_hSplash      = nullptr;
HBITMAP    g_hSplashImage = nullptr;
float      g_splashOpacity= 1.0f;
int        g_pulseDirection = -1;
ULONG_PTR  g_gdiplusToken = 0;

// Pending messages
std::vector<nlohmann::json> g_outboundMessages;
std::wstring g_launchProtocol;
std::atomic<bool>  g_isAppReady   = false;
std::atomic<bool>  g_waitStarted(false);

// Extensions
std::map<std::wstring, std::wstring> g_extensionMap;
std::vector<std::wstring> g_scriptQueue;

// Updater
std::atomic_bool       g_updaterRunning = false;
std::filesystem::path  g_installerPath;
std::thread            g_updaterThread;
const char* public_key_pem = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkDvxRT3uUlR1TQkUN0Ty
cuK1yYNujV6r5JJIKJsM7UsLjNL6Fy/wfK1JdVUSkYDiqHqN14/8kksWW4NdI+qc
HXRbuWrcD+SWFTb2shNVAgP9r6gB7HeSBhFVceGN/ncviWFDH0lSF+mGgRlaOyHN
NeToC1eYsRFn+LYlBd/OqMorOaQlqjHs+80h0oQFP0q9CrWFS2VnUvfxyL+mLEZk
S51E+kTFS4iGXaTV90uyXl2oF0GLT177LyN4/WC2PeToflVEXyNm32Dqb7xIhZW8
4Pb5BIlM2xNACdFdMLlhVhdAyAg99pZZ3V2mQZgSNp/38uyhY4CUAuTuac5LGRC3
+wIDAQAB
-----END PUBLIC KEY-----)";

// ThumbFast
std::atomic<bool> g_ignoreHover(false);
std::chrono::steady_clock::time_point g_ignoreUntil;
