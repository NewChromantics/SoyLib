#pragma once

#include "SoyEvent.h"
#include "SoyMath.h"
#include "Array.hpp"

//	gr: this might want expanding later for multiple screens, mouse button number etc
typedef vec2x<int32_t> TMousePos;
namespace SoyMouseButton
{
	enum Type
	{
		None,
		Left,
		Right,
		Middle,
	};
}

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
	//	todo: change these coordinates to pixels and client can normalise with GetScreenRect
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseDown;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseMove;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseUp;
	std::function<bool(ArrayBridge<std::string>&)>	mOnTryDragDrop;
	std::function<void(ArrayBridge<std::string>&)>	mOnDragDrop;
	
	virtual Soy::Rectx<int32_t>		GetScreenRect()=0;		//	get pixel size on screen

};

