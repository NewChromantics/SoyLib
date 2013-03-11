#pragma once


#include <ofxSoylent.h>

class ofTrueTypeFont;




namespace TRender
{
	extern ofTrueTypeFont	Font;
	bool					InitFont();
};

//	push/pop style for immediate rendering
class TRenderSceneScope
{
public:
	TRenderSceneScope(const char* SceneName);
	~TRenderSceneScope();
};


inline void ofClearDepth()
{
	glClear( GL_DEPTH_BUFFER_BIT );
}


inline void ofClearViewport(const ofRectangle& Viewport,const ofColor& ClearColour)
{
	ofClearDepth();
	ofPushStyle();
	ofSetColor( ClearColour );
	ofRect( Viewport );
	ofPopStyle();
}

