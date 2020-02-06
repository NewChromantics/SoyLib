#pragma once

#include <tchar.h>
#include <windows.h>

typedef unsigned char      uint8_t;

 /* Window class */
#define COLOURBUTTON_CLASSNAME	_T("ColourButton")
#define COLOURBUTTON_COLOURCHANGED		(WM_USER+1)
#define COLOURBUTTON_COLOURDIALOGCLOSED	(WM_USER+2)

/* Style to request using double buffering. */
//#define XXS_DOUBLEBUFFER         (0x0001)

/* Register/unregister the window class */
void Win32_Register_ColourButton(void);
void Win32_Unregister_ColourButton(void);
void ColourButton_SetColour(HWND ColourButtonHandle,uint8_t Red, uint8_t Green, uint8_t Blue);
