#include "SoyWindow.h"
#import <Cocoa/Cocoa.h>


NSCursor* CursorToNSCursor(SoyCursor::Type Cursor)
{
	switch ( Cursor )
	{
		default:
		case SoyCursor::Arrow:	return [NSCursor arrowCursor];
		case SoyCursor::Hand:	return [NSCursor openHandCursor];
		case SoyCursor::Busy:	return [NSCursor openHandCursor];
	}
}

void Soy::Platform::PushCursor(SoyCursor::Type Cursor)
{
	auto CursorNs = CursorToNSCursor( Cursor );
	[CursorNs push];
}

void Soy::Platform::PopCursor()
{
	[NSCursor pop];
}

namespace Soy
{
	namespace Platform
	{
		void	PushCursor(SoyCursor::Type Cursor);
		void	PopCursor();
	}
}

