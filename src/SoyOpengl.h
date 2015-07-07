#pragma once

#include <ostream>
#include <SoyEvent.h>
#include <SoyMath.h>
#include <SoyPixels.h>

//	opengl stuff
#if defined(TARGET_ANDROID)

#define OPENGL_ES_3	//	need 3 for FBO's
//#define GL_NONE				GL_NO_ERROR	//	declared in GLES3

#if defined(OPENGL_ES_3)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES/glext.h>	//	need for EOS
static const int GL_ES_VERSION = 3;	// This will be passed to EglSetup() by App.cpp
#endif


#if defined(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES/glext.h>	//	need for EOS
static const int GL_ES_VERSION = 2;	// This will be passed to EglSetup() by App.cpp
#endif

#endif

#if defined(TARGET_OSX)
#include <Opengl/gl.h>
#include <OpenGL/OpenGL.h>

#endif

#define GL_ASSET_INVALID	0




namespace Opengl
{
	class TFboMeta;
	class TUniform;
	class TAsset;
	class GlProgram;
	class TTexture;
	class TFbo;
	class TGeoQuad;
	class TShaderEosBlit;
	class TGeometry;
	
	
	
	enum VertexAttributeLocation
	{
		VERTEX_ATTRIBUTE_LOCATION_POSITION		= 0,
		VERTEX_ATTRIBUTE_LOCATION_NORMAL		= 1,
		VERTEX_ATTRIBUTE_LOCATION_TANGENT		= 2,
		VERTEX_ATTRIBUTE_LOCATION_BINORMAL		= 3,
		VERTEX_ATTRIBUTE_LOCATION_COLOR			= 4,
		VERTEX_ATTRIBUTE_LOCATION_UV0			= 5,
		VERTEX_ATTRIBUTE_LOCATION_UV1			= 6,
		//	VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES	= 7,
		//	VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS	= 8,
		//	VERTEX_ATTRIBUTE_LOCATION_FONT_PARMS	= 9
	};
	
	
	// It probably isn't worth keeping these shared here, each user
	// should just duplicate them.
	extern const char * externalFragmentShaderSource;
	extern const char * textureFragmentShaderSource;
	extern const char * identityVertexShaderSource;
	extern const char * untexturedFragmentShaderSource;
	
	extern const char * VertexColorVertexShaderSrc;
	extern const char * VertexColorSkinned1VertexShaderSrc;
	extern const char * VertexColorFragmentShaderSrc;
	
	extern const char * SingleTextureVertexShaderSrc;
	extern const char * SingleTextureSkinned1VertexShaderSrc;
	extern const char * SingleTextureFragmentShaderSrc;
	
	extern const char * LightMappedVertexShaderSrc;
	extern const char * LightMappedSkinned1VertexShaderSrc;
	extern const char * LightMappedFragmentShaderSrc;
	
	extern const char * ReflectionMappedVertexShaderSrc;
	extern const char * ReflectionMappedSkinned1VertexShaderSrc;
	extern const char * ReflectionMappedFragmentShaderSrc;
	
	// Will abort() after logging an error if either compiles or the link status
	// fails, but not if uniforms are missing.
	GlProgram	BuildProgram( const char * vertexSrc, const char * fragmentSrc );
	
	void		DeleteProgram( GlProgram & prog );
	
	
	
	struct VertexAttribs
	{
		std::vector<vec3f> position;
		std::vector<vec3f> normal;
		std::vector<vec3f> tangent;
		std::vector<vec3f> binormal;
		std::vector<vec4f> color;
		std::vector<vec2f> uv0;
		std::vector<vec2f> uv1;
		//Array< Vector4i > jointIndices;
		//Array< Vector4f > jointWeights;
	};
	
	typedef unsigned short TriangleIndex;
	//typedef unsigned int TriangleIndex;
	
	static const int MAX_GEOMETRY_VERTICES	= 1 << ( sizeof( TriangleIndex ) * 8 );
	static const int MAX_GEOMETRY_INDICES	= 1024 * 1024 * 3;
	
	//TGeometry BuildTesselatedQuad( const int horizontal, const int vertical );
	
#define Opengl_IsInitialised()	Opengl::IsInitialised(__func__,true)
#define Opengl_IsOkay()			Opengl::IsOkay(__func__)

	bool	IsInitialised(const std::string& Context,bool ThrowException);	//	throws exception
	bool	IsOkay(const std::string& Context);	//	throws exception
};





enum VertexAttributeLocation
{
	VERTEX_ATTRIBUTE_LOCATION_POSITION		= 0,
	VERTEX_ATTRIBUTE_LOCATION_NORMAL		= 1,
	VERTEX_ATTRIBUTE_LOCATION_TANGENT		= 2,
	VERTEX_ATTRIBUTE_LOCATION_BINORMAL		= 3,
	VERTEX_ATTRIBUTE_LOCATION_COLOR			= 4,
	VERTEX_ATTRIBUTE_LOCATION_UV0			= 5,
	VERTEX_ATTRIBUTE_LOCATION_UV1			= 6,
	VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES	= 7,
	VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS	= 8,
	VERTEX_ATTRIBUTE_LOCATION_FONT_PARMS	= 9
};

struct GlProgram
{
	GlProgram() :
	program( 0 ),
	vertexShader( 0 ),
	fragmentShader( 0 ),
	uMvp( 0 ),
	uModel( 0 ),
	uView( 0 ),
	uColor( 0 ),
	uFadeDirection( 0 ),
	uTexm( 0 ),
	uTexm2( 0 ),
	uJoints( 0 ),
	uColorTableOffset( 0 ) {};
	
	
	bool		IsValid()
	{
		return (program != 0);
	}
	
	
	// These will always be > 0 after a build, any errors will abort()
	unsigned	program;
	unsigned	vertexShader;
	unsigned	fragmentShader;
	
	// Uniforms that aren't found will have a -1 value
	int		uMvp;				// uniform Mvpm
	int		uModel;				// uniform Modelm
	int		uView;				// uniform Viewm
	int		uColor;				// uniform UniformColor
	int		uFadeDirection;		// uniform FadeDirection
	int		uTexm;				// uniform Texm
	int		uTexm2;				// uniform Texm2
	int		uJoints;			// uniform Joints
	int		uColorTableOffset;	// uniform offset to apply to color table index
};


GlProgram	BuildProgram( const char * vertexSrc, const char * fragmentSrc,std::ostream& Error  );

void		DeleteProgram( GlProgram & prog );




class Opengl::TFboMeta
{
public:
	TFboMeta()	{}
	TFboMeta(const std::string& Name,size_t Width,size_t Height) :
		mName	( Name ),
		mSize	( Width, Height )
	{
	}
	
	std::string		mName;
	vec2x<size_t>	mSize;
};




class Opengl::TUniform
{
public:
	TUniform() :
	mValue	( GL_ASSET_INVALID )
	{
	}
	TUniform(GLint Value) :
	mValue	( GL_ASSET_INVALID )
	{
	}
	
	GLint	mValue;
};


//	gr: for now, POD, no virtuals...
class Opengl::TAsset
{
public:
	TAsset() :
	mName	( GL_ASSET_INVALID )
	{
	}
	explicit TAsset(GLuint Name) :
	mName	( Name )
	{
	}
	
	bool	IsValid() const		{	return mName != GL_ASSET_INVALID;	}
	
	GLuint	mName;
};

class Opengl::GlProgram
{
public:
	bool		IsValid() const;
	
	TAsset		program;
	TAsset		vertexShader;
	TAsset		fragmentShader;
	
	// Uniforms that aren't found will have a -1 value
	TUniform	uMvp;				// uniform Mvpm
	TUniform	uModel;				// uniform Modelm
	TUniform	uView;				// uniform Viewm
	TUniform	uColor;				// uniform UniformColor
	TUniform	uFadeDirection;		// uniform FadeDirection
	TUniform	uTexm;				// uniform Texm
	TUniform	uTexm2;				// uniform Texm2
	TUniform	uJoints;			// uniform Joints
	TUniform	uColorTableOffset;	// uniform offset to apply to color table index
};

/*
class Opengl::TGeometry
{
public:
	TGeometry() :
	vertexBuffer( GL_ASSET_INVALID ),
	indexBuffer( GL_ASSET_INVALID ),
	vertexArrayObject( GL_ASSET_INVALID ),
	vertexCount( GL_ASSET_INVALID ),
	indexCount( GL_ASSET_INVALID )
	{
	}
	
	TGeometry( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices ) :
	vertexBuffer( GL_ASSET_INVALID ),
	indexBuffer( GL_ASSET_INVALID ),
	vertexArrayObject( GL_ASSET_INVALID ),
	vertexCount( GL_ASSET_INVALID ),
	indexCount( GL_ASSET_INVALID )
	{
		Create( attribs, indices );
	}
	
	// Create the VAO and vertex and index buffers from arrays of data.
	void	Create( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices );
	void	Update( const VertexAttribs & attribs );
	
	// Assumes the correct program, uniforms, textures, etc, are all bound.
	// Leaves the VAO bound for efficiency, be careful not to inadvertently
	// modify any of the state.
	// You need to manually bind and draw if you want to use GL_LINES / GL_POINTS / GL_TRISTRIPS,
	// or only draw a subset of the indices.
	void	Draw() const;
	
	// Free the buffers and VAO, assuming that they are strictly for this geometry.
	// We could save some overhead by packing an entire model into a single buffer, but
	// it would add more coupling to the structures.
	// This is not in the destructor to allow objects of this class to be passed by value.
	void	Free();
	
	bool	IsValid() const;
	
public:
	unsigned 	vertexBuffer;
	unsigned 	indexBuffer;
	unsigned 	vertexArrayObject;
	int			vertexCount;
	int 		indexCount;
};
*/

class Opengl::TTexture
{
public:
	explicit TTexture()
	{
	}
	TTexture(void* TexturePtr,const SoyPixelsMetaFull& Meta) :
	mMeta		( Meta ),
#if defined(TARGET_ANDROID)
	mTexture	( reinterpret_cast<GLuint>(TexturePtr) )
#elif defined(TARGET_OSX)
	mTexture	( static_cast<GLuint>(reinterpret_cast<GLuint64>(TexturePtr)) )
#endif
	{
	}
	
	bool				Bind();
	static void			Unbind();
	bool				IsValid() const;
	void				Delete();	//	no auto delete atm
	
public:
	TAsset				mTexture;
	SoyPixelsMetaFull	mMeta;
};

class Opengl::TFbo
{
public:
	TFbo(TTexture Texture);
	~TFbo();
	
	bool		IsValid() const	{	return mFbo.IsValid() && mTarget.IsValid();	}
	bool		Bind();
	void		Unbind();
	
	TAsset		mFbo;
	TTexture	mTarget;
};
/*
class Opengl::TGeoQuad
{
public:
	TGeoQuad();
	~TGeoQuad();
	
	bool		IsValid() const	{	return mGeometry.IsValid();	}
	
	TGeometry	mGeometry;
	
	bool		Render();
};


class Opengl::TShaderEosBlit
{
public:
	TShaderEosBlit();
	~TShaderEosBlit();
	
	bool		IsValid() const	{	return mProgram.IsValid();	}
	
	Opengl::GlProgram	mProgram;
};
*/

