#include "SoyWindow.h"

void SoyWindow::OnClosed()
{
	if ( mOnClosed )
	{
		mOnClosed();
	}
}

