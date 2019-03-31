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
		None = -1,
		Left = 0,
		Right = 1,
		Middle = 2,
	};
}

//	we'll make this more sophisticated later
namespace SoyKeyButton
{
	typedef char Type;
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

namespace Platform
{
	void	PushCursor(SoyCursor::Type Cursor);
	void	PopCursor();
	
	class TScreenMeta;
	void	EnumScreens(std::function<void(TScreenMeta&)> EnumScreen);
}


class Platform::TScreenMeta
{
public:
	std::string			mName;	//	unique identifier
	Soy::Rectx<int32_t>	mRect;	//	position relative to "main screen"
};


class SoyWindow
{
public:
	//	todo: change these coordinates to pixels and client can normalise with GetScreenRect
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseDown;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseMove;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseUp;
	std::function<void(SoyKeyButton::Type)>	mOnKeyDown;
	std::function<void(SoyKeyButton::Type)>	mOnKeyUp;
	std::function<bool(ArrayBridge<std::string>&)>	mOnTryDragDrop;
	std::function<void(ArrayBridge<std::string>&)>	mOnDragDrop;
	
	virtual Soy::Rectx<int32_t>		GetScreenRect()=0;		//	get pixel size on screen

};


