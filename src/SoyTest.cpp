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

#include <SoyPixels.h>
#include <SoyPng.h>

TEST(PngTypeTest)
{
	TPng::THeader Header;
	CHECK( !Header.IsValid() );
}

TEST(PngWriteRead)
{
	//	check we can read and write our own PNG's
	SoyPixels Pixels;
	CHECK( Pixels.Init( 32, 32, SoyPixelsFormat::RGB ) );
	Array<char> PngData;
	auto PngDataBridge = GetArrayBridge( PngData );
	CHECK( Pixels.GetPng( PngDataBridge ) );
	//	read back
	std::stringstream Error;
	CHECK( Pixels.SetPng( PngDataBridge, Error ) );
	CHECK( Error.str().empty() );
	if ( !Error.str().empty() )
	{
		std::cerr << "Expecting empty error: " << Error.str() << std::endl;
	}
}

#endif
