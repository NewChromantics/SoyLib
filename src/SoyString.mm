#include "SoyString.h"
#import <Foundation/Foundation.h>
#include "HeapArray.hpp"
#include "SoyCFString.h"


std::string Soy::GetString(CFStringRef CfString)
{
	//	https://stackoverflow.com/questions/28860033/convert-from-cfurlref-or-cfstringref-to-stdstring
	//	https://stackoverflow.com/questions/17227348/nsstring-to-cfstringref-and-cfstringref-to-nsstring-in-arc
	//auto* NsString = const_cast<NSString*>(reinterpret_cast<const NSString*>(CfString));
	//	gr: have to use this in arc mode only?
	auto* NsString = (__bridge NSString*)(CfString);
	
	return NSStringToString(NsString);
}



std::string Soy::NSStringToString(NSString* String)
{
	if ( !String )
		return "<null>";
	return std::string([String UTF8String]);
}

NSString* Soy::StringToNSString(const std::string& String)
{
	NSString* MacString = [NSString stringWithCString:String.c_str() encoding:[NSString defaultCStringEncoding]];
	return MacString;
}


std::string Soy::NSErrorToString(NSError* Error)
{
	if ( !Error )
		return "Error(null)";
	
	//	in case description is missing
	try
	{
		auto* ErrorNs = [Error description];
		return NSStringToString( ErrorNs );
	}
	catch ( ... )
	{
		return "Error(exception getting description)";
	}
}

std::string Soy::NSErrorToString(NSException* Exception)
{
	if ( !Exception )
		return "<No exception>";
	
	std::stringstream String;
	String << Soy::NSStringToString( Exception.name ) << ": " << Soy::NSStringToString( Exception.reason );
	return String.str();
}


void Soy::NSDictionaryToStrings(ArrayBridge<std::pair<std::string,std::string>>&& Elements,NSDictionary* Dictionary)
{
	for ( NSString* KeyNs in Dictionary )
	{
		std::string Key = Soy::NSStringToString( KeyNs );
		std::stringstream Value;
		@try
		{
			NSString* ValueNs = [[Dictionary objectForKey:KeyNs] description];
			Value << Soy::NSStringToString( ValueNs );
		}
		@catch (NSException* e)
		{
			Value << "<unkown value " << Soy::NSErrorToString( e ) << ">";
		}
	
		Elements.PushBack( std::make_pair(Key,Value.str() ) );
	}
}


std::string Soy::NSDictionaryToString(NSDictionary* Dictionary)
{
	Array<std::pair<std::string,std::string>> Strings;
	NSDictionaryToStrings( GetArrayBridge(Strings), Dictionary );

	std::stringstream String;
	String << "Dictionary x" << Strings.GetSize() << "; ";
	for ( int i=0;	i<Strings.GetSize();	i++ )
	{
		auto& Key = Strings[i].first;
		auto& Value = Strings[i].second;
		
		String << Key << "=" << Value << "; ";
	}
	
	return String.str();
}

std::string	Soy::NSDictionaryToString(CFDictionaryRef Dictionary)
{
	NSDictionary* DictionaryNs = (__bridge NSDictionary*)Dictionary;
	return NSDictionaryToString( DictionaryNs );
}

