#include "SoyWindow.h"

void SoyWindow::OnClosed()
{
	if ( mOnClosed )
	{
		mOnClosed();
	}
}

void SoySlider::OnChanged(bool FinalValue)
{
	if ( !mOnValueChanged )
		return;
	
	auto Value = GetValue();
	mOnValueChanged( Value, FinalValue );
}


void SoyTextBox::OnChanged()
{
	if ( !mOnValueChanged )
		return;
	
	auto Value = GetValue();
	mOnValueChanged( Value );
}


void SoyTickBox::OnChanged()
{
	if ( !mOnValueChanged )
		return;
	
	auto Value = GetValue();
	mOnValueChanged( Value );
}

void SoyColourButton::OnChanged(bool FinalValue)
{
	if (!mOnValueChanged)
		return;

	auto Value = GetValue();
	mOnValueChanged(Value, FinalValue);
}

void SoyButton::OnClicked()
{
	if (!mOnClicked)
	{
		//std::Debug << __PRETTY_FUNCTION__ << std::endl;
		return;
	}
	mOnClicked();
}

//	this should be in a linux gui file
#if defined(TARGET_LINUX) || defined(TARGET_ANDROID)
void Platform::EnumScreens(std::function<void(TScreenMeta&)> EnumScreen)
{
	Soy_AssertTodo();
}
#endif


