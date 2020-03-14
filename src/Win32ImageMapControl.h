#pragma once

#include <tchar.h>
#include <windows.h>
#include <stdint.h>


 /* Window class */
#define IMAGEMAP_CLASSNAME	_T("ImageMap")

void Win32_Register_ImageMap(void);
void Win32_Unregister_ImageMap(void);
void ImageMap_SetImage(HWND ImageMapHandle, const uint8_t* PixelsRgb,uint32_t Width,uint32_t Height);
void ImageMap_SetCursorMap(HWND ImageMapHandle, const HCURSOR* PixelsCursors, uint32_t Width, uint32_t Height);
