#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <windows.h>
#include <string>

void InitWebView2(HWND hWnd);
void WaitAndRefreshIfNeeded();
void refreshWeb(bool refreshAll);

// These are defined as static in webview.cpp (internal linkage)
// Declared here only for documentation — not needed externally
// static void SetupWebMessageHandler();
// static void SetupExtensions();
// static void SetupWebMods();

#endif // WEBVIEW_H
