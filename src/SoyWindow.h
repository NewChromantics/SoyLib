#pragma once

#include <SoyEvent.h>
#include <SoyMath.h>
#include <array.hpp>

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
	std::function<void(const TMousePos&)>			mOnMouseDown;
	std::function<void(const TMousePos&)>			mOnMouseMove;
	std::function<void(const TMousePos&)>			mOnMouseUp;
	std::function<bool(ArrayBridge<std::string>&)>	mOnTryDragDrop;
	
};

