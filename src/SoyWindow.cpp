#include "SoyWindow.h"

void SoyWindow::OnClosed()
{
	if ( mOnClosed )
	{
		mOnClosed();
	}
}

void SoySlider::OnChanged()
{
	if ( !mOnValueChanged )
		return;
	
	auto Value = GetValue();
	mOnValueChanged( Value );
}


void SoyTextBox::OnChanged()
{
	if ( !mOnValueChanged )
		return;
	
	auto Value = GetValue();
	mOnValueChanged( Value );
}
