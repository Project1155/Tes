#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <string>

void LoadSettings();
void SaveSettings();
void SaveWindowPlacement(const WINDOWPLACEMENT &wp);
bool LoadWindowPlacement(WINDOWPLACEMENT &wp);
// WriteIntToIni is internal to config.cpp — not exposed in header

#endif // CONFIG_H
