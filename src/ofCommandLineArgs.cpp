#include "ofCommandLineArgs.h"

/*


using namespace prgen;
using namespace prcore;

PRCommandLineArgs::PRCommandLineArgs(int argc,char* argv[])
{
	for ( int i=1;	i<argc;	i++ )
	{
		String Arg( argv[i] );

		//	check if it's an --option
		if ( Arg.StartsWith("--") )
		{
			Private::CommandLineOption& Option = mOptions.PushBack();

			//	split into option=value where applicable
			Array<String> Parts;
			Arg.Split( Parts, '=', false, 2 );
			Option.mOption = Parts[0];
			if ( Parts.GetSize() == 2 )
				Option.mValue = Parts[1];
		}
		else
		{
			//	just a parameter/argument
			mParameters.PushBack( Arg );
		}
	}

	mApplication = argv[0];
}


//	return the value (if any) for an --option=value. NULL returned if the option was not supplied. A non-null, but empty string will be returned if no value was supplied for the option. (eg. --option)
const String* PRCommandLineArgs::GetOption(const char* OptionName) const
{
	const Private::CommandLineOption* pOption = mOptions.Find( OptionName );
	return pOption ? &pOption->mValue : NULL;
}

//	return the value (if any) for an --option=value. NULL returned if the option was not supplied. A non-null, but empty string will be returned if no value was supplied for the option. (eg. --option)
const String* PRCommandLineArgs::GetOptionBool(const char* OptionName,bool& Value) const
{
	const String* pOption = GetOption( OptionName );
	if ( !pOption )
		return NULL;

	//	extract bool
	if ( !pOption->GetBool( Value ) )
		return NULL;

	//	successfully extracted bool
	return pOption;
}


//	return parameter (after options). If more than one supplied they'll be joined together into this one string
String PRCommandLineArgs::GetParameter() const
{
	String Parameter;
	for ( int i=0;	i<mParameters.GetSize();	i++ )
	{
		//	include spaces which were originally truncated by argv
		if ( i > 0 )
			Parameter << ' ';
		Parameter << mParameters[i];
	}

	return Parameter;
}

bool PRCommandLineArgs::HasInvalidOptions( const prcore::Array<prcore::String>& ValidOptions, prcore::Array<prcore::String>* InvalidOptions) const
{
	bool has_invalid = false;
	for (int o = 0; o < mOptions.GetSize(); ++o)
	{
		bool is_valid = false;
		for (int vo = 0; vo < ValidOptions.GetSize(); ++vo)
		{
			if (StringCompare(mOptions[o].mOption,ValidOptions[vo],false))
			{
				is_valid = true;
				break;
			}
		}
		if (is_valid == false)
		{
			if (InvalidOptions)
			{
				InvalidOptions->PushBack(mOptions[o].mOption);
			}
			has_invalid = true;
		}
	}
	return has_invalid;
}

	

*/