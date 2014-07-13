#include "SoyEvent.h"

#if defined(UNITTESTCPP_H)


static int gValue = 0;
void StaticSetValue(int& NewValue)		
{	
	gValue = NewValue;	
}

class TEventTest
{
public:
	TEventTest() :
		mValue	( gValue )
	{
	}
	void		SetValue(int& NewValue)	
	{	
		mValue = NewValue;
	}

	int			mValue;
};


TEST(SoyEventTest)
{
	SoyEvent<int> mChangeValue;

	//	test static funcs
	mChangeValue.AddListener( StaticSetValue );

	//	test members
	TEventTest Test;
	mChangeValue.AddListener( Test, &TEventTest::SetValue );

	//	todo: test lambda
	int NewValue = 999;
	mChangeValue.OnTriggered(NewValue);
	CHECK( Test.mValue == NewValue );
	CHECK( gValue == NewValue );
}

#endif