#pragma once

#include <tchar.h>
#include <windows.h>
#include <stdint.h>


 /* Window class */
#define COLOURBUTTON_CLASSNAME	_T("ColourButton")
#define COLOURBUTTON_COLOURCHANGED		(WM_APP+1)

void Win32_Register_ColourButton(void);
void Win32_Unregister_ColourButton(void);
void ColourButton_SetColour(HWND ColourButtonHandle, uint8_t Red, uint8_t Green, uint8_t Blue,BOOL SendNotification);
void ColourButton_GetColour(HWND ColourButtonHandle, uint8_t RedGreenBlue[3]);
