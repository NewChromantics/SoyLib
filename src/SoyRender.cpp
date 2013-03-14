#include "SoyRender.h"




ofTrueTypeFont TRender::Font;


bool TRender::InitFont()
{
	int dpi = 150;
	int size = 10;
	bool AntiAliased = true;
	bool FullCharSet = false;
	bool makeContours = false;
	float simplifyAmt = 0.3;
	const char* Filename = "data/NewMedia Fett.ttf";

	return Font.loadFont( Filename, size, AntiAliased, FullCharSet, makeContours, simplifyAmt, dpi );
};


TRenderSceneScope::TRenderSceneScope(const char* SceneName)
{
	ofPushMatrix();
	ofPushStyle();
	ofSetupGraphicDefaults();
	ofDisableBlendMode(); 
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc( GL_LEQUAL );	//	handy!
	glEnable(GL_DEPTH_CLAMP);	//	never clip
}


TRenderSceneScope::~TRenderSceneScope()
{
	ofPopMatrix();
	ofPopStyle();
}


void ofCube(const vec3f& Center,float WidthHeightDepth)
{
	ofPushMatrix();
	ofTranslate( Center );
	ofScale( WidthHeightDepth/2.f, WidthHeightDepth/2.f, WidthHeightDepth/2.f );

	//	front/back top/bottom left/right
	vec3f FTL( -1, -1, -1 );
	vec3f FTR(  1, -1, -1 );
	vec3f FBR(  1,  1, -1 );
	vec3f FBL( -1,  1, -1 );
	vec3f BTL( -1, -1,  1 );
	vec3f BTR(  1, -1,  1 );
	vec3f BBR(  1,  1,  1 );
	vec3f BBL( -1,  1,  1 );
	
	//	front
	ofTriangle( FBL, FTL, FTR );
	ofTriangle( FTR, FBR, FBL );

	//	back
	ofTriangle( BBL, BTL, BTR );
	ofTriangle( BTR, BBR, BBL );

	//	left
	ofTriangle( BBL, BTL, FTL );
	ofTriangle( FTL, FBL, BBL );

	//	right
	ofTriangle( FBR, FTR, BTR );
	ofTriangle( BTR, BBR, FBR );

	//	top
	ofTriangle( FTR, FTL, BTL );
	ofTriangle( BTL, BTR, FTR );

	//	bottom
	ofTriangle( FBR, FBL, BBL );
	ofTriangle( BBL, BBR, FBR );

	ofPopMatrix();
}
