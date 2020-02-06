/* File custom.c
 * (custom control implementation)
 */

#include "Win32ColourControl.h"
#include <wingdi.h>


#define OUTLINE_WIDTH	1
#define OUTLINE_COLOR	RGB(0,0,0)
#define BRUSH_COLOR		RGB(241,179,0)


typedef struct ColourButtonData {
	HWND hwnd;
	DWORD style;
	HBRUSH mBrushColour;
	HPEN mPenOutline;
	//HANDLE	mColourDialog;
} ColourButtonData;


static void ColourButton_Paint(ColourButtonData* pData, HDC hDC, RECT* rcDirty, BOOL bErase)
{
	int x, y;
	//RECT r;
	HBRUSH hBrush;

	//RECT rect = *rcDirty;
	RECT Rect;
	GetClientRect(WindowFromDC(hDC), &Rect);
	
	//	select pen, background, and draw
	SelectObject(hDC, pData->mPenOutline);
	//	gr: can we just use bkcolor?
	//	gr: bkcolor is pen background for dashed
	//SetBkColor(hDC, RGB(255, 0, 0));
	SelectObject(hDC, pData->mBrushColour);
	Rectangle( hDC, Rect.left, Rect.top, Rect.right, Rect.bottom );
	//FillRect(hDC, &Rect, pData->mBrushColour);
}

static LRESULT CALLBACK ColourButtonCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ColourButtonData* pData = (ColourButtonData*)GetWindowLongPtr(hwnd, 0);

	switch (uMsg) {
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
		if (wParam == GWL_STYLE)
			pData->style = lParam;
		break;

	case WM_NCCREATE:
		pData = (ColourButtonData*)malloc(sizeof(ColourButtonData));
		if (pData == NULL)
			return FALSE;
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)pData);
		pData->hwnd = hwnd;
		pData->style = ((CREATESTRUCT*)lParam)->style;
		pData->mPenOutline = CreatePen(PS_SOLID, OUTLINE_WIDTH, OUTLINE_COLOR);
		pData->mBrushColour = CreateSolidBrush(BRUSH_COLOR);
		return TRUE;

	case WM_NCDESTROY:
		if (pData != NULL) {
			DeleteObject(pData->mPenOutline);
			DeleteObject(pData->mBrushColour);
			free(pData);
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

void ColourButton_SetColour(HWND ColourButtonHandle, uint8_t Red, uint8_t Green, uint8_t Blue)
{
	//	remake the brush
	//	gr: is it safe to do here? do it when we repaint?
	ColourButtonData* pData = (ColourButtonData*)GetWindowLongPtr(ColourButtonHandle, 0);
	
	if (pData->mBrushColour)
	{
		DeleteObject(pData->mBrushColour);
		pData->mBrushColour = NULL;
	}

	auto Colour = RGB(Red, Green, Blue);
	pData->mBrushColour = CreateSolidBrush(Colour);

	//	trigger repaint automtically?
	InvalidateRect(ColourButtonHandle, NULL, TRUE);
}
