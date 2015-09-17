#pragma once

#include <SoyEvent.h>
#include <SoyMath.h>

//	gr: this might want expanding later for multiple screens, mouse button number etc
typedef vec2f TMousePos;


namespace SoyCursor
{
	enum Type
	{
		Arrow,
		Hand,
		Busy,
	};
}

namespace Soy
{
	namespace Platform
	{
		void	PushCursor(SoyCursor::Type Cursor);
		void	PopCursor();
	}
}



class SoyWindow
{
public:
	SoyEvent<const TMousePos>		mOnMouseDown;
	SoyEvent<const TMousePos>		mOnMouseMove;
	SoyEvent<const TMousePos>		mOnMouseUp;
};

