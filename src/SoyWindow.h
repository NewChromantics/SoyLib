#pragma once

#include "SoyMath.h"
#include "Array.hpp"

//	should we remove the pixels dependency? (for ImageMap)
class SoyPixelsImpl;

//	todo: move all types into this namespace
namespace Gui
{
	class TControl;	
	class TColourPicker;
	class TImageMap;
	class TMouseEvent;
	class TRenderView;
    class TStringArray;
}

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
		Back = 3,
		Forward = 4,
	};
}

namespace SoyMouseEvent
{
	enum Type
	{
		Invalid = 0,
		Down,
		Move,
		Up
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
		ArrowAndWait,
		Cross,
		Hand,
		Help,
		TextCursor,
		NotAllowed,
		ResizeAll,
		ResizeVert,
		ResizeHorz,
		ResizeNorthEast,
		ResizeNorthWest,
		UpArrow,
		Wait,
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
	virtual bool					IsMinimised() = 0;
	virtual bool					IsForeground() = 0;
	virtual void					EnableScrollBars(bool Horz,bool Vert)=0;
	virtual void					OnClosed();
	virtual void					StartRender(std::function<void()> Frame, std::string ViewName) {};

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


//	strictly only functions that are going to be cross platform
class Gui::TControl
{
public:
	virtual void			SetRect(const Soy::Rectx<int32_t>& Rect) = 0;		//	set position on screen

	//	gr: styling functions, this is to basically emulate CSS
	//		maybe this is too specific here (not always useful)
	//		and instead we should have 
	//			Platform::SetControlVisible
	//			Platform::SetControlColour
	//		and implement styles in a more platform specific way
	virtual void			SetVisible(bool Visible)=0;
	virtual void			SetColour(const vec3x<uint8_t>& Rgb)=0;	//	alpha seperate, win32 doesn't support it. This is tint on ios. 
};

//	gr: lets avoid a hacky virtual void* GetPlatformSpecificViewOrContext
//		instead, overload this with Platform::TRenderView and put a hard typed version there
//		that class should prepare pre-draw, call OnDraw and then end a frame 
class Gui::TRenderView
{
public:
	virtual ~TRenderView()	{};
	
	std::function<void()>	mOnDraw;
};

class SoySlider : public Gui::TControl
{
public:
	virtual void					SetMinMax(uint16_t Min,uint16_t Max,uint16_t NotchCount)=0;
	virtual void					SetValue(uint16_t Value)=0;
	virtual uint16_t				GetValue()=0;
	
	virtual void					OnChanged(bool FinalValue=true);	//	helper to do GetValue and call the callback

	//	gr: windows has a limit of DWORDs so we'll limit to 16bit for now
	//		OSX uses doubles
	//		2nd param is whether value is final (ie. mouseup)
	std::function<void(uint16_t&,bool)>	mOnValueChanged;	//	reference so caller can change value in the callback

};


class SoyTextBox : public Gui::TControl
{
public:
	virtual void					SetValue(const std::string& Value)=0;
	virtual std::string				GetValue()=0;
	
	virtual void					OnChanged();	//	helper to do GetValue and call the callback
	
	std::function<void(const std::string&)>	mOnValueChanged;
};


class SoyLabel : public Gui::TControl
{
public:
	virtual void					SetValue(const std::string& Value)=0;
	virtual std::string				GetValue()=0;
};


class SoyTickBox : public Gui::TControl
{
public:
	//	todo: all platforms have half-checked, so turn this into a state
	virtual void					SetValue(bool Value)=0;
	virtual bool					GetValue()=0;

	//	all platforms have a label!
	virtual void					SetLabel(const std::string& Label)=0;

	virtual void					OnChanged();	//	helper to do GetValue and call the callback
	
	std::function<void(bool&)>		mOnValueChanged;	//	reference so caller can change value in the callback
};


//	native control, but a custom one in SoyLib
class SoyColourButton : public Gui::TControl
{
public:
	virtual void					SetValue(vec3x<uint8_t> Value) = 0;
	virtual vec3x<uint8_t>			GetValue() = 0;

	virtual void					OnChanged(bool FinalValue);	//	helper to do GetValue and call the callback

	std::function<void(vec3x<uint8_t>&,bool)>	mOnValueChanged;	//	reference so caller can change value in the callback
};

class SoyButton : public Gui::TControl
{
public:
	virtual void			SetLabel(const std::string& Label) = 0;
	void					OnClicked();	//	helper function

	std::function<void()>	mOnClicked;
};


class Gui::TMouseEvent
{
public:
	vec2x<int32_t>			mPosition;
	SoyMouseButton::Type	mButton = SoyMouseButton::None;
	SoyMouseEvent::Type		mEvent = SoyMouseEvent::Invalid;
};


//	colour pickers are temporary dialogs on windows & osx, but can have consistent callbacks
//	windows' default colour picker has no alpha, so we're not supporting it atm
class Gui::TColourPicker
{
public:
	std::function<void(vec3x<uint8_t>&)>	mOnValueChanged;
	std::function<void()>					mOnDialogClosed;
};


//	an Image map control is an image which auto sets the OS cursor for certain areas
//	and relays back mouse movement. Used for high-level custom gui controls which just update pixels
class Gui::TImageMap : public Gui::TControl
{
public:
	virtual void			SetImage(const SoyPixelsImpl& Pixels) = 0;

	//	we accept strings for cursor names, because windows allows loading from exe resources. Otherwise they map to SoyCursor::Type enums
	virtual void			SetCursorMap(const SoyPixelsImpl& CursorMap,const ArrayBridge<std::string>&& CursorIndexes) = 0;

public:
	std::function<void(Gui::TMouseEvent&)>	mOnMouseEvent;
};

class Gui::TStringArray
{
public:
    virtual void                        SetValue(const ArrayBridge<std::string>&& Values)=0;
    virtual ArrayBridge<std::string>&&  GetValue()=0;
};

namespace Platform
{
	class TWindow;
	class TSlider;
	class TTextBox;
	class TLabel;
	class TTickBox;
	class TColourButton;
	class TColourPicker;
	class TImageMap;
	class TMetalView;
	class TButton;
    class TStringArray;

	class TOpenglView;		//	on osx it's a view control
	class TOpenglContext;	//	on windows, its a context that binds to any control
	class TWin32Thread;		//	windows needs to make calls on a specific thread (just as OSX needs it to be on the main dispatcher)

	//	platforms should implement these
	std::shared_ptr<SoyWindow>			CreateWindow(const std::string& Name, Soy::Rectx<int32_t>& Rect, bool Resizable);
	std::shared_ptr<SoySlider>			CreateSlider(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<SoyTextBox>			GetTextBox(SoyWindow& Parent,const std::string& Name);
	std::shared_ptr<SoyTextBox>			CreateTextBox(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<SoyLabel>			GetLabel(SoyWindow& Parent,const std::string& Name);
	std::shared_ptr<SoyLabel>			CreateLabel(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<SoyButton>			GetButton(SoyWindow& Parent,const std::string& Name);
	std::shared_ptr<SoyButton>			CreateButton(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<SoyTickBox>			GetTickBox(SoyWindow& Parent,const std::string& Name);
	std::shared_ptr<SoyTickBox>			CreateTickBox(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<Gui::TColourPicker>	CreateColourPicker(vec3x<uint8_t> InitialColour);
	std::shared_ptr<SoyColourButton>	CreateColourButton(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<Gui::TImageMap>		GetImageMap(SoyWindow& Parent, const std::string& Name);
	std::shared_ptr<Gui::TImageMap>		CreateImageMap(SoyWindow& Parent, Soy::Rectx<int32_t>& Rect);
	std::shared_ptr<Gui::TRenderView>	GetRenderView(SoyWindow& Parent, const std::string& Name);
    std::shared_ptr<Gui::TStringArray>  GetStringArray(SoyWindow& Parent, const std::string& Name);
    std::shared_ptr<Gui::TStringArray>  CreateStringArray(SoyWindow& Parent, const ArrayBridge<std::string>&& Values);
}
