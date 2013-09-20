#include "ofxSoylent.h"
#include "SoyStream.h"
#include "SoySocket.h"
#include "ofxXmlSettings.h"


bool SoyStream::TFile::Write(const ofxXmlSettings& xml)
{
	BufferString<MAX_PATH> Filename = GetFilename();
	auto& xmlmutable = const_cast<ofxXmlSettings&>( xml );
	return xmlmutable.saveFile( Filename.c_str() );
}


