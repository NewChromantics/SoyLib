/* File custom.c
 * (custom control implementation)
 */

#include "Win32ColourControl.h"
#include <wingdi.h>


#define OUTLINE_WIDTH	1
#define OUTLINE_COLOR	RGB(0,0,0)
static const Default_Red = 0;
static const Default_Green = 255;
static const Default_Blue = 255;



typedef struct ColourButtonData 
{
	HWND		mHwnd;
	HBRUSH		mColourBrush;
	HPEN		mOutlinePen;
	COLORREF	mColour;
	COLORREF	mPalette[16];

} ColourButtonData;


static void ColourButton_SetColourRef(HWND ColourButtonHandle, COLORREF Colour,BOOL SendNotification)
{
	//	remake the brush
	//	gr: is it safe to do here? do it when we repaint?
	ColourButtonData* pData = (ColourButtonData*)GetWindowLongPtr(ColourButtonHandle, 0);
	if (!pData)
		return;// FALSE;

	pData->mColour = Colour;

	//	recreate brush
	if (pData->mColourBrush)
	{
		DeleteObject(pData->mColourBrush);
		pData->mColourBrush = NULL;
	}
	pData->mColourBrush = CreateSolidBrush(pData->mColour);

	//	trigger repaint
	InvalidateRect(ColourButtonHandle, NULL, TRUE);

	if (SendNotification)
	{
		//	https://docs.microsoft.com/en-us/windows/win32/controls/user-controls-intro#sending-notifications-from-a-control
		int ControlId = GetDlgCtrlID(ColourButtonHandle);
		HWND Parent = GetParent(ColourButtonHandle);
		WPARAM WParam = MAKEWPARAM(ControlId, COLOURBUTTON_COLOURCHANGED);
		LPARAM LParam = (LPARAM)ColourButtonHandle;
		SendMessage(Parent, WM_COMMAND, WParam, LParam);
	}
}


static void ColourButton_Paint(ColourButtonData* pData, HDC hDC, RECT* rcDirty, BOOL bErase)
{
	int x, y;
	//RECT r;
	HBRUSH hBrush;

	//RECT rect = *rcDirty;
	RECT Rect;
	GetClientRect(WindowFromDC(hDC), &Rect);
	
	//	select pen, background, and draw
	SelectObject(hDC, pData->mOutlinePen);
	//	gr: can we just use bkcolor?
	//	gr: bkcolor is pen background for dashed
	//SetBkColor(hDC, RGB(255, 0, 0));
	SelectObject(hDC, pData->mColourBrush);
	Rectangle( hDC, Rect.left, Rect.top, Rect.right, Rect.bottom );
	//FillRect(hDC, &Rect, pData->mBrushColour);
}

static void FreeColourButtonData(ColourButtonData* Data)
{
	if (Data == NULL)
		return;

	DeleteObject(Data->mOutlinePen);
	DeleteObject(Data->mColourBrush);
	free(Data);
}


static ColourButtonData* CreateColourButtonData()
{
	ColourButtonData* pData = (ColourButtonData*)malloc(sizeof(ColourButtonData));
	if (pData == NULL)
		return NULL;
	
	pData->mColour = RGB(Default_Red, Default_Green, Default_Blue);
	pData->mHwnd = NULL;
	pData->mOutlinePen = CreatePen(PS_SOLID, OUTLINE_WIDTH, OUTLINE_COLOR);
	pData->mColourBrush = CreateSolidBrush(pData->mColour);

	return pData;
}

UINT_PTR ColourDialogCallback(
	HWND Arg1,
	UINT Message,
	WPARAM Arg3,
	LPARAM Arg4
)
{
	switch (Message)
	{
	case WM_KILLFOCUS:
	case WM_SETFOCUS:
	case WM_COMMAND:
	case WM_SETFONT:
	case WM_ERASEBKGND:
	case WM_PAINT:
	case WM_WINDOWPOSCHANGING:
	case WM_GETICON:
	case WM_WINDOWPOSCHANGED:
	case WM_CHANGEUISTATE:
	case WM_UPDATEUISTATE:
	case WM_INITDIALOG:
	case WM_ACTIVATEAPP:
	case WM_NCACTIVATE:
		return 0;
	}
	return 0;
}

static LRESULT CALLBACK ColourButtonCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ColourButtonData* pData = (ColourButtonData*)GetWindowLongPtr(hwnd, 0);

	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return FALSE;  // Defer erasing into WM_PAINT

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		ColourButton_Paint(pData, ps.hdc, &ps.rcPaint, ps.fErase);
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_PRINTCLIENT:
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		ColourButton_Paint(pData, (HDC)wParam, &rc, TRUE);
		return 0;
	}

	case WM_STYLECHANGED:
		/*
		if (wParam == GWL_STYLE)
			pData->style = lParam;
			*/
		break;

	case WM_NCCREATE:

		if (!pData)
		{
			pData = CreateColourButtonData();
			SetWindowLongPtr(hwnd, 0, (LONG_PTR)pData);
			if (!pData)
				return FALSE;
		}

		pData->mHwnd = hwnd;
		return TRUE;

	case WM_NCDESTROY:
		FreeColourButtonData(pData);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
		return 0;

	case WM_LBUTTONDOWN:
		if (pData != NULL) 
		{
			CHOOSECOLOR Meta;
			ZeroMemory(&Meta, sizeof(CHOOSECOLOR));
			Meta.lStructSize = sizeof(CHOOSECOLOR);
			Meta.hwndOwner = hwnd;
			Meta.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
			Meta.lpfnHook = ColourDialogCallback;
			//	gr: crashes without palette
			Meta.lpCustColors = (LPDWORD)pData->mPalette;
			Meta.rgbResult = pData->mColour;

			if (ChooseColor(&Meta))
			{
				ColourButton_SetColourRef(hwnd,Meta.rgbResult,TRUE);
			}
			else
			{
				//	check  CommDlgExtendedError 
				//	https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms646912(v=vs.85)?redirectedfrom=MSDN
			}
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Win32_Register_ColourButton(void)
{
	WNDCLASS wc = { 0 };

	// Note we do not use CS_HREDRAW and CS_VREDRAW.
	// This means when the control is resized, WM_SIZE (as handled by DefWindowProc()) 
	// invalidates only the newly uncovered area.
	// With those class styles, it would invalidate complete client rectangle.
	wc.style = CS_GLOBALCLASS;
	wc.lpfnWndProc = ColourButtonCallback;
	wc.cbWndExtra = sizeof(ColourButtonData*);
	wc.hCursor = LoadCursor(NULL, IDC_HAND);
	wc.lpszClassName = COLOURBUTTON_CLASSNAME;
	RegisterClass(&wc);
}

void Win32_Unregister_ColourButton(void)
{
	UnregisterClass(COLOURBUTTON_CLASSNAME, NULL);
}



void ColourButton_SetColour(HWND ColourButtonHandle, uint8_t Red, uint8_t Green, uint8_t Blue,BOOL SendNotification)
{
	auto Colour = RGB(Red, Green, Blue);
	ColourButton_SetColourRef(ColourButtonHandle, Colour,SendNotification);
}


void ColourButton_GetColour(HWND ColourButtonHandle, uint8_t RedGreenBlue[3])
{
	ColourButtonData* pData = (ColourButtonData*)GetWindowLongPtr(ColourButtonHandle, 0);
	if (!pData)
	{
		RedGreenBlue[0] = 0;
		RedGreenBlue[1] = 0;
		RedGreenBlue[2] = 0;
		return;
	}

	RedGreenBlue[0] = GetRValue(pData->mColour);
	RedGreenBlue[1] = GetGValue(pData->mColour);
	RedGreenBlue[2] = GetBValue(pData->mColour);
}
