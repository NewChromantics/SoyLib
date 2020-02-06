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
