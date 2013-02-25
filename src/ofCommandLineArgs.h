#pragma once


#include "..\..\..\addons\ofxSoylent\src\ofxSoylent.h"



/*

namespace Private
{
	class CommandLineOption
	{
	public:
		String	mOption;
		String	mValue;

		inline bool		operator==(const char* Name) const	{	return StringCompare( mOption, String(Name), false );	}
	};
};


class PRCommandLineArgs
{
public:
	PRCommandLineArgs(int argc,char* argv[]);

	bool									HasAnyArguments() const						{	return HasOptions() || HasParameters();	}
	bool									HasOptions() const							{	return !mOptions.IsEmpty();	}
	bool									HasParameters() const						{	return !mParameters.IsEmpty();	}
	const prcore::String*					GetOption(const char* OptionName) const;	//	return the value (if any) for an --option=value. NULL returned if the option was not supplied. A non-null, but empty string will be returned if no value was supplied for the option. (eg. --option)
	const prcore::String*					GetOptionBool(const char* OptionName,bool& Value) const;	//	match an option and convert to a bool. if it fails to convert to a bool (or doesnt exist) NULL is returned. On success the value string is returned
	prcore::String							GetParameter() const;						//	return parameter (after options). If more than one supplied they'll be joined together into this one string
	const prcore::Array<prcore::String>&	GetParameters() const						{	return mParameters;	}

	bool									HasInvalidOptions( const prcore::Array<prcore::String>& ValidOptions, 
																prcore::Array<prcore::String>* InvalidOptions = NULL ) const;	// given a list of ValidOptions, return true if there are any options that are not in that list (and optionally, their names)

	prcore::String							GetApplication() const { return mApplication; }
private:
	prcore::Array<Private::CommandLineOption>	mOptions;
	prcore::Array<prcore::String>				mParameters;
	prcore::String								mApplication;
};

*/
