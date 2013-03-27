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
	if ( ClearColour.a > 0 )
	{
		ofSetColor( ClearColour );
		ofRect( Viewport );
	}
	ofPopStyle();
}

template<class ARRAY>
void ofDrawPathClosed(const ARRAY& PathPoints)
{
	for ( int a=0;	a<PathPoints.GetSize();	a++ )
	{
		const auto& va = PathPoints[a];
		int b = (a+1) % PathPoints.GetSize();
		const auto& vb = PathPoints[b];
		ofLine( va, vb );
	}
}
