#include "SoyRender.h"
#include "ofShape.h"




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


void ofCapsule(const ofShapeCapsule2& Capsule,float z)
{
	if ( !Capsule.IsValid() )
		return;

	//	generate triangle points
	float mRadius = Capsule.mRadius;
	ofLine2 Line = Capsule.mLine;
	vec2f Normal = Line.GetNormal();
	vec2f Right = Normal.getPerpendicular();

	//	BL TL TR BR (bottom = start)
	vec3f Quad_BL( Line.mStart - (Right*mRadius), z );
	vec3f Quad_TL( Line.mEnd - (Right*mRadius), z );
	vec3f Quad_TR( Line.mEnd + (Right*mRadius), z );
	vec3f Quad_BR( Line.mStart + (Right*mRadius), z );

	if ( ofGetFill() == OF_FILLED )
	{
		ofTriangle( Quad_BL, Quad_TL, Quad_TR );
		ofTriangle( Quad_TR, Quad_BR, Quad_BL );
	}
	else if ( ofGetFill() == OF_OUTLINE )
	{
		ofLine( Quad_BL, Quad_TL );
		ofLine( Quad_BR, Quad_TR );
		//ofLine( Line.mStart, Line.mEnd );
		//ofRect( Line.mStart, mRadius/4.f, mRadius/4.f );
	}

	//	http://digerati-illuminatus.blogspot.co.uk/2008/05/approximating-semicircle-with-cubic.html
	vec3f BLControl = Quad_BL - (Normal * mRadius * 4.f/3.f) + (Right * mRadius * 0.10f );
	vec3f BRControl = Quad_BR - (Normal * mRadius * 4.f/3.f) - (Right * mRadius * 0.10f );
	vec3f TLControl = Quad_TL + (Normal * mRadius * 4.f/3.f) + (Right * mRadius * 0.10f );
	vec3f TRControl = Quad_TR + (Normal * mRadius * 4.f/3.f) - (Right * mRadius * 0.10f );
	BLControl.z = 
	BRControl.z =
	TLControl.z = 
	TRControl.z = z;
	ofBezier( Quad_BL, BLControl, BRControl, Quad_BR );
	ofBezier( Quad_TL, TLControl, TRControl, Quad_TR );
}



void ofDrawTriangles(const ArrayBridge<ofShapeTriangle2>& Triangles)
{
	for ( int t=0;	t<Triangles.GetSize();	t++ )
	{
		auto& Triangle = Triangles[t];
		auto& v0 = Triangle.mTriangle[0];
		auto& v1 = Triangle.mTriangle[1];
		auto& v2 = Triangle.mTriangle[2];
		ofTriangle( v0.x, v0.y, v1.x, v1.y, v2.x, v2.y );
	}
}

void ofDrawTriangles(const ArrayBridge<ofShapeTriangle2>& Triangles,ofTexture& Texture,const ArrayBridge<ofShapeTriangle2>& TexCoords)
{
	Texture.bind();

	for ( int t=0;	t<Triangles.GetSize();	t++ )
	{
		auto& Triangle = Triangles[t];
		auto& TexCoord = TexCoords[t];
		auto& v0 = Triangle.mTriangle[0];
		auto& v1 = Triangle.mTriangle[1];
		auto& v2 = Triangle.mTriangle[2];
		
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer( 2, GL_FLOAT, sizeof(ofVec2f), &TexCoord );
		ofTriangle( v0.x, v0.y, v1.x, v1.y, v2.x, v2.y );
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	Texture.unbind();
}
