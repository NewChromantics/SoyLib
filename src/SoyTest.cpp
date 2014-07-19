//	include this .cpp where we have unit tests!
#if defined(UNITTESTCPP_H)

TEST(SoyTimeStreamio)
{
	SoyTime One(1ull);
	
	std::stringstream String;
	String << One;
	std::Debug << String.str() << std::endl;
	std::Debug << One.ToString() << std::endl;
	CHECK( String.str() == "T000000001" );
	CHECK( One.ToString() == "T000000001" );

	CHECK( One.FromString( String.str() ) );
	CHECK( One.GetTime() == 1 );
}

#endif
