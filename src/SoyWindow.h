#pragma once

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
	//	position relative to "main screen" 
	//	gr^^ verify this for windows
	Soy::Rectx<int32_t>	mFullRect;	//	whole resolution
	Soy::Rectx<int32_t>	mWorkRect;	//	window-space-resolution (ie. no toolbars, menu bars etc)
};


class SoyWindow
{
public:
	virtual Soy::Rectx<int32_t>		GetScreenRect()=0;		//	get pixel size on screen
	//virtual void					SetRect(const Soy::Rectx<int32_t>& Rect)=0;		//	set position on screen

	virtual void					SetFullscreen(bool Fullscreen)=0;
	virtual bool					IsFullscreen()=0;
	virtual void					OnClosed();

public:
	//	todo: change these coordinates to pixels and client can normalise with GetScreenRect
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseDown;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseMove;
	std::function<void(const TMousePos&,SoyMouseButton::Type)>	mOnMouseUp;
	std::function<void(SoyKeyButton::Type)>	mOnKeyDown;
	std::function<void(SoyKeyButton::Type)>	mOnKeyUp;
	std::function<bool(ArrayBridge<std::string>&)>	mOnTryDragDrop;
	std::function<void(ArrayBridge<std::string>&)>	mOnDragDrop;
	std::function<void()>			mOnClosed;
};

class SoySlider
{
public:
	virtual void					SetRect(const Soy::Rectx<int32_t>& Rect)=0;		//	set position on screen
	
	virtual void					SetMinMax(uint16_t Min,uint16_t Max)=0;
	virtual void					SetValue(uint16_t Value)=0;
	virtual uint16_t				GetValue()=0;
	
	virtual void					OnChanged();	//	helper to do GetValue and call the callback

	//	gr: windows has a limit of DWORDs so we'll limit to 16bit for now
	//		OSX uses doubles
	std::function<void(uint16_t&)>	mOnValueChanged;	//	reference so caller can change value in the callback

};


class SoyTextBox
{
public:
	virtual void					SetRect(const Soy::Rectx<int32_t>& Rect)=0;		//	set position on screen
	
	virtual void					SetValue(const std::string& Value)=0;
	virtual std::string				GetValue()=0;
	
	virtual void					OnChanged();	//	helper to do GetValue and call the callback
	
	std::function<void(const std::string&)>	mOnValueChanged;
};


class SoyLabel
{
public:
	virtual void					SetRect(const Soy::Rectx<int32_t>& Rect)=0;		//	set position on screen
	
	virtual void					SetValue(const std::string& Value)=0;
	virtual std::string				GetValue()=0;
};


class SoyTickBox
{
public:
	virtual void					SetRect(const Soy::Rectx<int32_t>& Rect)=0;		//	set position on screen
	
	//	todo: all platforms have half-checked, so turn this into a state
	virtual void					SetValue(bool Value)=0;
	virtual bool					GetValue()=0;

	//	all platforms have a label!
	virtual void					SetLabel(const std::string& Label)=0;

	virtual void					OnChanged();	//	helper to do GetValue and call the callback
	
	std::function<void(bool&)>		mOnValueChanged;	//	reference so caller can change value in the callback
};

