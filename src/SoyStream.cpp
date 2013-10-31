#include "ofxSoylent.h"
#include "SoyStream.h"
#include "ofxXmlSettings.h"


bool SoyStream::TFile::Write(const ofxXmlSettings& xml)
{
	BufferString<MAX_PATH> Filename = GetFilename();
	auto& xmlmutable = const_cast<ofxXmlSettings&>( xml );
	return xmlmutable.saveFile( Filename.c_str() );
}

bool SoyStream::TFile::Read(ofxXmlSettings& xml)
{
	BufferString<MAX_PATH> Filename = GetFilename();
	if ( !xml.loadFile( Filename.c_str() ) )
		return false;
	//	mark last-modified
	return true;
}


