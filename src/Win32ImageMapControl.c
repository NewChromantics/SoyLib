#include "Win32ImageMapControl.h"
#include <wingdi.h>


#define NoBrushColourRef	RGB(0, 255, 255)
typedef BOOL bool;

typedef struct ImageMapData
{
	HWND		mHwnd;
	HBITMAP		mBitmap;
	HCURSOR*	mCursorMap;
	uint32_t	mCursorMapWidth;
	uint32_t	mCursorMapHeight;

} ImageMapData;


static void ImageMap_Paint(ImageMapData* pData, HDC hDC, RECT* rcDirty, BOOL bErase)
{
	RECT Rect;
	if (!GetClientRect(WindowFromDC(hDC), &Rect))
	{
		//	error!
		return;
	}

	static bool AlwaysClear = TRUE;
	
	auto NoBitmap = (!pData || !pData->mBitmap);

	//	paint a colour if no bitmap
	if ( AlwaysClear || NoBitmap)
	{
		auto OldBrushColour = SetDCBrushColor(hDC, NoBrushColourRef);
		HGDIOBJ NoImageBrush = GetStockObject(DC_BRUSH);
		HGDIOBJ NoImagePen = GetStockObject(BLACK_PEN);

		//	select pen, background, and draw
		SelectObject(hDC, NoImagePen);
		SelectObject(hDC, NoImageBrush);
		Rectangle(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom);
	}

	if (NoBitmap)
		return;

	HANDLE BitmapHandle = pData->mBitmap;
	HDC HdcMem = CreateCompatibleDC(hDC);
	HGDIOBJ OldBitmap = SelectObject(HdcMem, BitmapHandle);

	BITMAP Bitmap;
	GetObject(BitmapHandle, sizeof(Bitmap), &Bitmap);
	
	int SourceWidth = Bitmap.bmWidth;
	int SourceHeight= Bitmap.bmHeight;
	int DestWidth = Rect.right - Rect.left;
	int DestHeight = Rect.bottom - Rect.top;
	
	if (DestWidth != SourceWidth || DestHeight != SourceHeight)
	{
		SetStretchBltMode(hDC, HALFTONE);
		if (!StretchBlt(hDC, 0, 0, DestWidth, DestHeight, HdcMem, 0, 0, SourceWidth, SourceHeight, SRCCOPY))
		{
		}
	}
	else
	{
		if (!BitBlt(hDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, HdcMem, 0, 0, SRCCOPY))
		{
		}
	}

	//	restore	
	SelectObject(HdcMem, OldBitmap);
	DeleteDC(HdcMem);
}

static void FreeImageMapData(ImageMapData* Data)
{
	if (Data == NULL)
		return;

	if (Data->mBitmap)
		DeleteObject(Data->mBitmap);
	free(Data);
}

static ImageMapData* GetImageMapData(HANDLE Hwnd)
{
	ImageMapData* Data = (ImageMapData*)GetWindowLongPtr(Hwnd, 0);
	return Data;
}


static ImageMapData* CreateImageMapData(HWND Handle)
{
	ImageMapData* pData = (ImageMapData*)malloc(sizeof(ImageMapData));
	if (pData == NULL)
		return NULL;
	
	pData->mBitmap = NULL;
	pData->mHwnd = Handle;
	pData->mCursorMap = NULL;
	pData->mCursorMapWidth = 0;
	pData->mCursorMapHeight = 0;

	return pData;
}

HCURSOR ImageMap_GetCursor(ImageMapData* Data,POINT xy)
{
	//	get cursor from map
	if (!Data)
		return NULL;
	if (!Data->mCursorMap)
		return NULL;

	RECT ClientRect;
	if (!GetClientRect(Data->mHwnd, &ClientRect))
		return NULL;

	//	scale to our map
	float rectw = ClientRect.right - ClientRect.left;
	float recth = ClientRect.bottom - ClientRect.top;
	uint32_t w = Data->mCursorMapWidth;
	uint32_t h = Data->mCursorMapHeight;
	float xf = xy.x / rectw;
	float yf = xy.y / recth;
	int32_t x = xf * w;
	int32_t y = yf * h;
	x = max(0, min(x, w - 1));
	y = max(0, min(y, h - 1));
	uint32_t Index = x + (y*w);
	HCURSOR Cursor = Data->mCursorMap[Index];
	return Cursor;
}


static LRESULT CALLBACK ImageMapCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImageMapData* pData = GetImageMapData(hwnd);

	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return FALSE;  // Defer erasing into WM_PAINT

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		ImageMap_Paint(pData, ps.hdc, &ps.rcPaint, ps.fErase);
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_PRINTCLIENT:
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		ImageMap_Paint(pData, (HDC)wParam, &rc, TRUE);
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
			pData = CreateImageMapData(hwnd);
			SetWindowLongPtr(hwnd, 0, (LONG_PTR)pData);
			if (!pData)
				return FALSE;
		}
		return TRUE;

	case WM_NCDESTROY:
		FreeImageMapData(pData);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
		return 0;

	case WM_SETCURSOR:
		if (pData != NULL)
		{
			POINT xy;
			if (GetCursorPos(&xy))
			{
				//	not correct
				//xy.x = LOWORD(lParam);
				//xy.y = HIWORD(lParam);
				ScreenToClient(hwnd, &xy);

				HCURSOR Cursor = ImageMap_GetCursor(pData, xy);
				if (Cursor)
				{
					SetCursor(Cursor);
					return TRUE;
				}
			}
		}
		//	re: returns https://stackoverflow.com/a/19257787/355753
		return FALSE;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Win32_Register_ImageMap(void)
{
	WNDCLASS wc = { 0 };

	// Note we do not use CS_HREDRAW and CS_VREDRAW.
	// This means when the control is resized, WM_SIZE (as handled by DefWindowProc()) 
	// invalidates only the newly uncovered area.
	// With those class styles, it would invalidate complete client rectangle.
	wc.style = CS_GLOBALCLASS;
	wc.lpfnWndProc = ImageMapCallback;
	wc.cbWndExtra = sizeof(ImageMapData*);
	wc.hCursor = LoadCursor(NULL, IDC_HAND);
	wc.lpszClassName = IMAGEMAP_CLASSNAME;
	RegisterClass(&wc);
}

void Win32_Unregister_ImageMap(void)
{
	UnregisterClass(IMAGEMAP_CLASSNAME, NULL);
}

void ImageMap_Repaint(HWND ImageMap)
{
	//	trigger repaint
	InvalidateRect(ImageMap, NULL, TRUE);
}


void ResizeArray(HCURSOR** Data, uint32_t* Width, uint32_t* Height, uint32_t NewWidth, uint32_t NewHeight, uint32_t ElementSize)
{
	uint32_t OldSize = (*Width) * (*Height);
	uint32_t NewSize = NewWidth * NewHeight;
	if (*Data)
	{
		if (OldSize != NewSize)
		{
			free(*Data);
			*Data = NULL;
		}
	}

	if (!*Data)
	{
		*Data = malloc(ElementSize * NewSize);
		*Width = NewWidth;
		*Height = NewHeight;
	}
}

void ImageMap_SetImage(HWND ImageMapHandle,const uint8_t* PixelsBgr, uint32_t Width, uint32_t Height)
{
	ImageMapData* ImageMap = GetImageMapData(ImageMapHandle);
	if (!ImageMap)
		return;
	
	if (ImageMap->mBitmap)
	{
		DeleteObject(ImageMap->mBitmap);
		ImageMap->mBitmap = NULL;
	}

	bool Flip = TRUE;
	int Channels = 3;
	BITMAPINFO BitmapInfo;
	BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Flip ? -Height : Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = Channels * sizeof(uint8_t)*8;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;	//	this is BGR!
	BitmapInfo.bmiHeader.biSizeImage = 0;	//	only needed for !BI_RGB
	BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	BitmapInfo.bmiHeader.biClrUsed = 0;
	BitmapInfo.bmiHeader.biClrImportant = 0;
	
	HDC Hdc = GetDC(ImageMap->mHwnd);
	//void* BitmapPixels = (void*)&PixelsRgb;	//	const cast
	void* BitmapPixels = NULL;
	ImageMap->mBitmap = CreateDIBSection(Hdc, &BitmapInfo, DIB_RGB_COLORS, &BitmapPixels, 0, 0);
	
	memcpy(BitmapPixels, PixelsBgr, Width*Height * Channels);
	
	ImageMap_Repaint(ImageMapHandle);
}


void ImageMap_SetCursorMap(HWND ImageMapHandle,const HCURSOR* PixelsCursors, uint32_t Width, uint32_t Height)
{
	ImageMapData* Data = GetImageMapData(ImageMapHandle);
	if (!Data)
		return;

	//	resize map
	int OldSize = Data->mCursorMapWidth * Data->mCursorMapHeight;
	int NewSize = Width * Height;
	ResizeArray(&Data->mCursorMap, &Data->mCursorMapWidth, &Data->mCursorMapHeight, Width, Height, sizeof(HCURSOR) );

	//	copy data
	memcpy(Data->mCursorMap, PixelsCursors, sizeof(HCURSOR) * Width * Height);

	//	try and trigger cursor change too?
	ImageMap_Repaint(ImageMapHandle);
}
