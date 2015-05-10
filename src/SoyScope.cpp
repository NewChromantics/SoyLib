#include "SoyScope.h"

#if defined(UNITTESTCPP_H)


class TStackCounter
{
public:
	TStackCounter(int& Counter) :
	mCounter	( Counter )
	{
	}
	
	void		Begin()	{	mCounter++;	}
	void		End()	{	mCounter--;	}
	
	int&		mCounter;
};


int Test_IncrementViaScope(int& StackVariable)
{
	auto Enter = [&StackVariable](){
		StackVariable++;
		std::cout << "enter" << std::endl;
	};
	auto Exit = [&StackVariable](){
		StackVariable--;
		std::cout << "exit" << std::endl;
	};
	
	TStackCounter Stack( StackVariable )
	
	auto ScopeA = SoyScope( [Enter]{Enter();}, [Exit]{Exit();} );
	auto ScopeB = SoyScopeSimple( Enter, Exit );
	auto ScopeC = SoyScope( [&Stack]{Stack.Begin();}, [&Stack]{Stack.End();} );
	auto ScopeD = SoyScopeMember( Stack, &TStackCounter::Begin, &TStackCounter::End );
	
	std::cout << "post enter" << std::endl;
	return StackVariable;
}

TEST(TestLambdaScope)
{
	//	start at 0
	int Counter = 0;
	
	//	pass in the counter, and capture it's value at the end of the func, before scopes finish
	int CounterEOF = Test_IncrementViaScope( Counter );
	
	EXPECT_GT( CounterEOF, 0 );
	EXPECT_EQ( Counter, 0 );
}

int SopeTest = DoScopeTest();


#endif