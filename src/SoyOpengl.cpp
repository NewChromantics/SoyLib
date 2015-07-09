#include "SoyOpengl.h"
#include "RemoteArray.h"

namespace Opengl
{
	const std::map<SoyPixelsFormat::Type,GLint>&	GetPixelFormatMap();
	std::string		GetTypeName(GLenum Type);
	
	template<typename TYPE>
	GLenum			GetTypeEnum()
	{
		throw new Soy::AssertException( std::string("Unhandled Get Gl enum from type case ")+__func__ );
		return GL_INVALID_ENUM;
	}
	
	template<typename TYPE>
	void			SetUniform(const TUniform& Uniform,const TYPE& Value)
	{
		std::stringstream Error;
		Error << __func__ << " not overloaded for " << Soy::GetTypeName<TYPE>();
		throw new Soy::AssertException( Error.str() );
	}

}

template<> GLenum Opengl::GetTypeEnum<uint16>()		{	return GL_UNSIGNED_SHORT;	}
template<> GLenum Opengl::GetTypeEnum<GLshort>()	{	return GL_UNSIGNED_SHORT;	}

std::string Opengl::GetTypeName(GLenum Type)
{
	switch ( Type )
	{
		case GL_BYTE:			return Soy::GetTypeName<int8>();
		case GL_UNSIGNED_BYTE:	return Soy::GetTypeName<uint8>();
		case GL_SHORT:			return Soy::GetTypeName<int16>();
		case GL_UNSIGNED_SHORT:	return Soy::GetTypeName<uint16>();
		case GL_INT:			return Soy::GetTypeName<int>();
		case GL_UNSIGNED_INT:	return Soy::GetTypeName<unsigned int>();
		case GL_FLOAT:			return Soy::GetTypeName<float>();
		case GL_FLOAT_VEC2:		return Soy::GetTypeName<vec2f>();
		case GL_FLOAT_VEC3:		return Soy::GetTypeName<vec3f>();
		case GL_FLOAT_VEC4:		return Soy::GetTypeName<vec4f>();
		case GL_INT_VEC2:		return Soy::GetTypeName<vec2x<int>>();
		case GL_INT_VEC3:		return Soy::GetTypeName<vec3x<int>>();
		case GL_INT_VEC4:		return Soy::GetTypeName<vec4x<int>>();
		case GL_BOOL:			return Soy::GetTypeName<bool>();
		case GL_SAMPLER_2D:		return "sampler2d";
		case GL_SAMPLER_CUBE:	return "samplercube";
		case GL_FLOAT_MAT2:		return "float2x2";
		case GL_FLOAT_MAT3:		return "float3x3";
		case GL_FLOAT_MAT4:		return "float4x4";

#if defined(OPENGL_CORE_3)
		case GL_DOUBLE:			return Soy::GetTypeName<double>();
		case GL_SAMPLER_1D:		return "sampler1d";
		case GL_SAMPLER_3D:		return "sampler3d";
#endif
	};
	
	std::stringstream Unknown;
	Unknown << "Unknown GL type 0x" << std::hex << Type;
	return Unknown.str();
}

std::ostream& Opengl::operator<<(std::ostream &out,const Opengl::TUniform& in)
{
	out << "#" << in.mIndex << " ";
	out << Opengl::GetTypeName(in.mType);
	if ( in.mArraySize != 1 )
		out << "[" << in.mArraySize << "]";
	out << " " << in.mName;
	return out;
}



template<>
void Opengl::SetUniform(const TUniform& Uniform,const vec4f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform4fv( Uniform.mIndex, ArraySize, &Value.x );
	Opengl_IsOkay();
}


// Returns false and logs the ShaderInfoLog on failure.
bool CompileShader( const GLuint shader, const char * src,const std::string& ErrorPrefix,std::ostream& Error)
{
	glShaderSource( shader, 1, &src, 0 );
	glCompileShader( shader );
	
	GLint r;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
	if ( r == GL_FALSE )
	{
		Error << ErrorPrefix << " Compiling shader error: ";
		GLchar msg[4096] = {0};
		glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
		Error << msg;
		return false;
	}
	return true;
}


const char* Opengl::ErrorToString(GLenum Error)
{
	switch ( Error )
	{
		case GL_NO_ERROR:			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:		return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:		return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:	return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:		return "GL_OUT_OF_MEMORY";

			//	opengl < 3 errors
#if defined(GL_STACK_UNDERFLOW)
		case GL_STACK_UNDERFLOW:	return "GL_STACK_UNDERFLOW";
#endif
#if defined(GL_STACK_OVERFLOW)
		case GL_STACK_OVERFLOW:		return "GL_STACK_OVERFLOW";
#endif
		default:
			return "Unknown opengl error value";
	};
}

bool Opengl::IsInitialised(const std::string &Context, bool ThrowException)
{
	return true;
}

bool Opengl::IsOkay(const std::string& Context)
{
	if ( !IsInitialised(std::string("CheckIsOkay") + Context, false ) )
		return false;
	
	auto Error = glGetError();
	if ( Error == GL_NONE )
		return true;
	
	std::stringstream ErrorStr;
	ErrorStr << "Opengl error in " << Context << ": " << Opengl::ErrorToString(Error) << std::endl;
	
	return Soy::Assert( Error == GL_NONE , ErrorStr.str() );
}

bool Opengl::IsSupported(const char *ExtensionName)
{
	return false;
}


Opengl::TFbo::TFbo(TTexture Texture) :
	mFbo		( GL_ASSET_INVALID ),
	mTarget		( Texture )
{
	Soy::Assert( Texture.IsValid(), "invalid texture" );
	
	auto& mFboTextureName = mTarget.mTexture.mName;
	auto& mFboMeta = mTarget.mMeta;
	
	std::Debug << "Creating FBO " << mFboMeta << ", texture name: " << mFboTextureName << std::endl;
	
	//	initialise new texture
	
	//	gr: get format from meta
	auto FboTexureFormat = GL_RGBA;
	//glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, mFboTextureName );
	Opengl::IsOkay("FBO glBindTexture");
	
	
	//GL_INVALID_OPERATION is generated if format does not match internalformat.
	auto DataFormat = FboTexureFormat;
	auto DataType = GL_UNSIGNED_BYTE;
	//GL_INVALID_OPERATION is generated if type is GL_UNSIGNED_SHORT_5_6_5 and format is not GL_RGB.
	//GL_INVALID_OPERATION is generated if type is GL_UNSIGNED_SHORT_4_4_4_4 or GL_UNSIGNED_SHORT_5_5_5_1 and format is not GL_RGBA.
	
	glTexImage2D( GL_TEXTURE_2D, 0, FboTexureFormat, mFboMeta.GetWidth(), mFboMeta.GetHeight(), 0, DataFormat, DataType, NULL );
	Opengl::IsOkay("FBO glTexImage2D");
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glBindTexture( GL_TEXTURE_2D, 0 );
	Opengl::IsOkay("FBO UN glBindTexture");
	//glActiveTexture( GL_TEXTURE0 );
	
	glGenFramebuffers( 1, &mFbo.mName );
	Opengl::IsOkay("FBO glGenFramebuffers");
	glBindFramebuffer( GL_FRAMEBUFFER, mFbo.mName );
	Opengl::IsOkay("FBO glBindFramebuffer2");
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboTextureName, 0 );
	Opengl::IsOkay("FBO glFramebufferTexture2D");
	
	//	gr: init? or render?
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);
	
	//	caps check
	auto FrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	Soy::Assert( FrameBufferStatus == GL_FRAMEBUFFER_COMPLETE, "DIdn't complete framebuffer setup" );
	
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	Opengl::IsOkay("FBO UN glBindTexture2");
}

Opengl::TFbo::~TFbo()
{
	if ( mFbo.mName != GL_ASSET_INVALID )
	{
		glDeleteFramebuffers( 1, &mFbo.mName );
		mFbo.mName = GL_ASSET_INVALID;
	}
}

bool Opengl::TFbo::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo.mName );
	Opengl_IsOkay();
	glViewport( 0, 0, mTarget.mMeta.GetWidth(), mTarget.mMeta.GetHeight() );
	Opengl_IsOkay();
	
	/* debugging
	glClearColor( 0.5f, 0.5f, 0, 1 );
	glClear( GL_COLOR_BUFFER_BIT );
	Opengl_IsOkay();
	
	 {
		glColor3f(1.0f, 1.f, 0.f);
		glBegin(GL_TRIANGLES);
		{
	 glVertex3f(  0.0,  0.6, 0.0);
	 glVertex3f( -0.2, -0.3, 0.0);
	 glVertex3f(  0.2, -0.3 ,0.0);
		}
		glEnd();
	 }
	Opengl_IsOkay();
*/
	return true;
}

void Opengl::TFbo::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	Opengl_IsOkay();
}

/*
Opengl::TGeometry Opengl::BuildTesselatedQuad( const int horizontal, const int vertical )
{
	const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );
	
	VertexAttribs attribs;
	attribs.position.resize( vertexCount );
	attribs.uv0.resize( vertexCount );
	attribs.color.resize( vertexCount );
	
	for ( int y = 0; y <= vertical; y++ )
	{
		const float yf = (float) y / (float) vertical;
		for ( int x = 0; x <= horizontal; x++ )
		{
			const float xf = (float) x / (float) horizontal;
			const int index = y * ( horizontal + 1 ) + x;
			attribs.position[index].x = -1 + xf * 2;
			attribs.position[index].z = 0;
			attribs.position[index].y = -1 + yf * 2;
			attribs.uv0[index].x = xf;
			attribs.uv0[index].y = 1.0 - yf;
			for ( int i = 0; i < 4; i++ )
			{
				attribs.color[index][i] = 1.0f;
			}
			// fade to transparent on the outside
			if ( x == 0 || x == horizontal || y == 0 || y == vertical )
			{
				attribs.color[index][3] = 0.0f;
			}
		}
	}
	
	std::vector< TriangleIndex > indices;
	indices.resize( horizontal * vertical * 6 );
	
	// If this is to be used to draw a linear format texture, like
	// a surface texture, it is better for cache performance that
	// the triangles be drawn to follow the side to side linear order.
	int index = 0;
	for ( int y = 0; y < vertical; y++ )
	{
		for ( int x = 0; x < horizontal; x++ )
		{
			indices[index + 0] = y * (horizontal + 1) + x;
			indices[index + 1] = y * (horizontal + 1) + x + 1;
			indices[index + 2] = (y + 1) * (horizontal + 1) + x;
			indices[index + 3] = (y + 1) * (horizontal + 1) + x;
			indices[index + 4] = y * (horizontal + 1) + x + 1;
			indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
			index += 6;
		}
	}
	
	return TGeometry( attribs, indices );
}
 */
/*
Opengl::TGeoQuad::TGeoQuad()
{
	mGeometry = BuildTesselatedQuad(1,1);
}

Opengl::TGeoQuad::~TGeoQuad()
{
	mGeometry.Free();
}


bool Opengl::TGeoQuad::Render()
{
	mGeometry.Draw();
	return true;
}
 */
/*

Opengl::TShaderEosBlit::TShaderEosBlit()
{
	mProgram = Opengl::BuildProgram(
									"uniform highp mat4 Mvpm;\n"
									"attribute vec4 Position;\n"
									//"attribute vec2 TexCoord;\n"
									//"varying  highp vec2 oTexCoord;\n"
									"void main()\n"
									"{\n"
									"   gl_Position = Position;\n"
									//"   oTexCoord = TexCoord;\n"
									"}\n"
									,
									//"#extension GL_OES_EGL_image_external : require\n"
									//"uniform samplerExternalOES Texture0;\n"
									//"varying highp vec2 oTexCoord;\n"
									"void main()\n"
									"{\n"
									"	gl_FragColor = vec4(1,0,0,1);\n"
									//"	gl_FragColor = texture2D( Texture0, oTexCoord );\n"
									"}\n"
									);
}

Opengl::TShaderEosBlit::~TShaderEosBlit()
{
	DeleteProgram( mProgram );
}


bool Opengl::BlitOES(TTexture& Texture,TFbo& Fbo,TGeoQuad& Geo,TShaderEosBlit& Shader)
{
	Opengl::IsOkay("start of BlitOES");
	if ( !Opengl_IsInitialised() )
		return false;
	if ( !Texture.IsValid() )
	{
		std::Debug << __func__ << " texture invalid" << std::endl;
		Soy::Assert(false,"texture invalid");
		return false;
	}
	std::Debug << __func__ << " texture ok" << std::endl;
	
	if ( !Fbo.IsValid() )
	{
		std::Debug << __func__ << " Fbo invalid" << std::endl;
		Soy::Assert(false,"Fbo invalid");
		return false;
	}
	std::Debug << __func__ << " fbo ok" << std::endl;
	
	if ( !Geo.IsValid() )
	{
		std::Debug << __func__ << " Geo invalid" << std::endl;
		Soy::Assert(false,"Geo invalid");
		return false;
	}
	std::Debug << __func__ << " Geo ok" << std::endl;
	
	if ( !Shader.IsValid() )
	{
		std::Debug << __func__ << " Shader invalid" << std::endl;
		Soy::Assert(false,"Shader invalid");
		return false;
	}
	std::Debug << __func__ << " shader ok" << std::endl;
	
	std::Debug << __func__ << " bind texture" << std::endl;
	//	copy surface texture to FBO which is bound to target texture
	glActiveTexture( GL_TEXTURE0 );
	Opengl::IsOkay("glActiveTexture");
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, Texture.mTexture.mName );
	Opengl::IsOkay("glBindTexture");
	
	std::Debug << __func__ << " bind fbo" << std::endl;
	glBindFramebuffer( GL_FRAMEBUFFER, Fbo.mFbo.mName );
	Opengl::IsOkay("glBindFramebuffer");
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	
	std::Debug << __func__ << " glInvalidateFramebuffer" << std::endl;
	const GLenum fboAttachments[1] = { GL_COLOR_ATTACHMENT0 };
	glInvalidateFramebuffer_( GL_FRAMEBUFFER, 1, fboAttachments );
	Opengl::IsOkay("glInvalidateFramebuffer_");
	
	glViewport( 0, 0, Texture.mMeta.GetWidth(), Texture.mMeta.GetHeight() );
	std::Debug << __func__ << " glUseProgram" << std::endl;
	glUseProgram( Shader.mProgram.program.mName );
	Opengl::IsOkay("glUseProgram");
	
	std::Debug << __func__ << " Geo.Render" << std::endl;
	Geo.Render();
	Opengl::IsOkay("Geo.Render");
	std::Debug << __func__ << " finished render" << std::endl;
	glUseProgram( 0 );
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	
	glBindTexture( GL_TEXTURE_2D, Fbo.mTarget.mTexture.mName );
	glGenerateMipmap( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
	
	std::Debug << __func__ << " finished" << std::endl;
	
	return true;
	
}
 */

Opengl::TTexture::TTexture(SoyPixelsMetaFull Meta) :
	mAutoRelease	( true )
{
	Opengl_IsInitialised();
	Soy::Assert( Meta.IsValid(), "Cannot setup texture with invalid meta" );
	
	GLuint Format = Opengl::GetPixelFormat( Meta.GetFormat() );
	if ( Format == GL_INVALID_VALUE )
	{
		std::stringstream Error;
		Error << "Failed to create texture, unsupported format " << Meta.GetFormat();
		throw new Soy::AssertException( Error.str() );
	}

	//	allocate texture
	glGenTextures( 1, &mTexture.mName );
	Opengl::IsOkay("glGenTextures");
	
	if ( !Bind() )
		throw  new Soy::AssertException("failed to bind after texture allocation");
	Opengl::IsOkay("glGenTextures");
	
	//	set mip-map levels to 0..0
	//	gr: change this to glGenerateMipMaps etc
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	//	initialise to set dimensions
	SoyPixels InitFramePixels;
	InitFramePixels.Init( Meta.GetWidth(), Meta.GetHeight(), Meta.GetFormat() );
	auto& PixelsArray = InitFramePixels.GetPixelsArray();
	glTexImage2D( GL_TEXTURE_2D, 0, Format, Meta.GetWidth(), Meta.GetHeight(), 0, Format, GL_UNSIGNED_BYTE, PixelsArray.GetArray() );
	//GLint MipLevel = 0;
	//glTexImage2D( GL_TEXTURE_2D, MipLevel, Format, Meta.GetWidth(), Meta.GetHeight(), 0, Format, GL_UNSIGNED_BYTE, nullptr );
	Opengl::IsOkay("glTexImage2D");
	
	//	built, save meta
	mMeta = Meta;
}

bool Opengl::TTexture::IsValid() const
{
	if ( !Opengl::IsInitialised(__func__,false) )
		return false;
	
	auto IsTexture = glIsTexture( mTexture.mName );

	//	gr: on IOS this is nice and reliable and NEEDED to distinguish from metal textures!
#if defined(TARGET_IOS)
	return IsTexture;
#else
	
	//	gr: this is returning false [on OSX] from other threads :/ even though they have a context
	//	other funcs are working though
	if ( IsTexture )
		return true;
	
	if ( mTexture.mName != GL_ASSET_INVALID )
		return true;
	
	return false;
#endif
}

void Opengl::TTexture::Delete()
{
	if ( mTexture.mName != GL_ASSET_INVALID )
	{
		glDeleteTextures( 1, &mTexture.mName );
		mTexture.mName = GL_ASSET_INVALID;
	}
}

bool Opengl::TTexture::Bind()
{
	glBindTexture( GL_TEXTURE_2D, mTexture.mName );
	return Opengl_IsOkay();
}

void Opengl::TTexture::Unbind()
{
	glBindTexture( GL_TEXTURE_2D, GL_ASSET_INVALID );
}

void Opengl::TTexture::Copy(const SoyPixelsImpl& SourcePixels,bool Stretch)
{
	Bind();
	Opengl::IsOkay( std::string(__func__) + " Bind()" );

	int MipLevel = 0;

	
	//	grab the texture's width & height so we can clip, if we try and copy pixels bigger than the texture we'll get an error
	//	gr: wrap this into a "get meta"
	GLint TextureWidth = 0;
	GLint TextureHeight = 0;
	GLint TextureInternalFormat = 0;
	
	//	opengl es doesn't have access!
#if defined(OPENGL_ES_3)
	TextureWidth = SourcePixels.GetWidth();
	TextureHeight = SourcePixels.GetHeight();
	TextureInternalFormat = GL_RGBA;
#else
	glGetTexLevelParameteriv (GL_TEXTURE_2D, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	glGetTexLevelParameteriv (GL_TEXTURE_2D, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
#endif
	
	//	pixel data format
	auto GlPixelsFormat = Opengl::GetPixelFormat( SourcePixels.GetFormat() );

	
	//	convert to upload-compatible type
	Array<SoyPixelsFormat::Type> TryFormats;
	TryFormats.PushBack( SoyPixelsFormat::RGB );
	TryFormats.PushBack( SoyPixelsFormat::RGBA );
	TryFormats.PushBack( SoyPixelsFormat::Greyscale );
	SoyPixels TempPixels;
	const SoyPixelsImpl* UsePixels = &SourcePixels;
	while ( GlPixelsFormat == GL_INVALID_VALUE && !TryFormats.IsEmpty() )
	{
		auto TryFormat = TryFormats.PopAt(0);
		auto TryGlFormat = Opengl::GetPixelFormat( TryFormat );
		if ( TryGlFormat == GL_INVALID_VALUE )
			continue;
		
		if ( !TempPixels.Copy(SourcePixels ) )
			continue;
		if ( !TempPixels.SetFormat( TryFormat ) )
			continue;
		
		GlPixelsFormat = TryGlFormat;
		UsePixels = &TempPixels;
	}
	
	auto& FinalPixels = *UsePixels;
	
	
	//	only for "new" textures
	GLuint TargetFormat = GL_RGBA;
	
	
	//	todo: find when we NEED to make a new texture (uninitialised)
	bool SubImage = !Stretch;

#if defined(TARGET_OSX)
	if ( !SubImage && Opengl::IsSupported("GL_APPLE_client_storage") )
	{
		//	https://developer.apple.com/library/mac/documentation/graphicsimaging/conceptual/opengl-macprogguide/opengl_texturedata/opengl_texturedata.html
		glTexParameteri(GL_TEXTURE_2D,
						GL_TEXTURE_STORAGE_HINT_APPLE,
						GL_STORAGE_CACHED_APPLE);
			
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
		
		//	gr: need a buffering system to save a buffer per texture then just update the pixels
		static SoyPixels PixelsBuffer;
		PixelsBuffer.Copy( FinalPixels );
			
		TargetFormat = GL_BGRA;
		GLenum TargetStorage = GL_UNSIGNED_INT_8_8_8_8_REV;
		glTexImage2D(GL_TEXTURE_2D, MipLevel, GlPixelsFormat, PixelsBuffer.GetWidth(), PixelsBuffer.GetHeight(), 0, TargetFormat, TargetStorage, PixelsBuffer.GetPixelsArray().GetArray() );
		Opengl_IsOkay();
	}
	else
#endif
	if ( !SubImage )
	{
		int Border = 0;
		
		//	if texture doesnt fit we'll get GL_INVALID_VALUE
		//	if frame is bigger than texture, it will mangle (bad stride)
		//	if pixels is smaller, we'll just get the sub-image drawn
		auto Width = std::min<GLsizei>( TextureWidth, FinalPixels.GetWidth() );
		auto Height = std::min<GLsizei>( TextureHeight, FinalPixels.GetHeight() );
		
		const ArrayInterface<char>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();

		glTexImage2D( GL_TEXTURE_2D, MipLevel, TargetFormat,  Width, Height, Border, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
		Opengl_IsOkay();
	}
	else
	{
		int XOffset = 0;
		int YOffset = 0;
		
		auto Width = std::min<GLsizei>( TextureWidth, FinalPixels.GetWidth() );
		auto Height = std::min<GLsizei>( TextureHeight, FinalPixels.GetHeight() );

		const ArrayInterface<char>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();
		
		//	invalid operation here means the unity pixel format is probably different to the pixel data we're trying to push now
		glTexSubImage2D( GL_TEXTURE_2D, MipLevel, XOffset, YOffset, Width, Height, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
		Opengl_IsOkay();
	}

	glGenerateMipmap( GL_TEXTURE_2D );
	Opengl_IsOkay();
	
	Unbind();
	Opengl_IsOkay();
}




// Returns false and logs the ShaderInfoLog on failure.
bool CompileShader( const Opengl::TAsset& shader, const char * src )
{
	if ( !Opengl_IsInitialised() )
		return false;
	
	glShaderSource( shader.mName, 1, &src, nullptr );
	glCompileShader( shader.mName );
	
	GLint r;
	glGetShaderiv( shader.mName, GL_COMPILE_STATUS, &r );
	if ( r == GL_FALSE )
	{
		std::Debug << "Compiling shader:\n" << src << "\n****** failed ******\n";
		GLchar msg[4096];
		glGetShaderInfoLog( shader.mName, sizeof( msg ), 0, msg );
		std::Debug << msg << std::endl;;
		return false;
	}
	return true;
}


Opengl::TShaderState::TShaderState(const Opengl::GlProgram& Shader) :
	mTextureBindCount	( 0 ),
	mShader				( Shader )
{
	glUseProgram( Shader.program.mName );
	Opengl_IsOkay();
}

Opengl::TShaderState::~TShaderState()
{
	//	unbind textures
	TTexture NullTexture;
	while ( mTextureBindCount > 0 )
	{
		BindTexture( mTextureBindCount-1, NullTexture );
		mTextureBindCount--;
	}
	
	//	unbind shader
	glUseProgram(0);
}



void Opengl::TShaderState::SetUniform(const char* Name,const vec4f& v)
{
	auto Uniform = mShader.GetUniform( Name );
	Soy::Assert( Uniform.IsValid(), std::stringstream() << "Invalid uniform " << Name );
	Opengl::SetUniform( Uniform, v );
}

void Opengl::TShaderState::SetUniform(const char* Name,const TTexture& Texture)
{
	auto Uniform = mShader.GetUniform( Name );
	Soy::Assert( Uniform.IsValid(), std::stringstream() << "Invalid uniform " << Name );

	auto BindTextureIndex = size_cast<GLint>( mTextureBindCount );
	BindTexture( BindTextureIndex, Texture );
	glUniform1i( Uniform.mIndex, BindTextureIndex );
	Opengl_IsOkay();
	mTextureBindCount++;
}

void Opengl::TShaderState::BindTexture(size_t TextureIndex,TTexture Texture)
{
	const GLenum _TexturesBindings[] =
	{
		GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7, GL_TEXTURE8, GL_TEXTURE9,
		GL_TEXTURE10, GL_TEXTURE11, GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14, GL_TEXTURE15, GL_TEXTURE16, GL_TEXTURE17, GL_TEXTURE18, GL_TEXTURE19,
		GL_TEXTURE20, GL_TEXTURE21, GL_TEXTURE22, GL_TEXTURE23, GL_TEXTURE24, GL_TEXTURE25, GL_TEXTURE26, GL_TEXTURE27, GL_TEXTURE28, GL_TEXTURE29,
	};
	auto TextureBindings = GetRemoteArray( _TexturesBindings );
	
	Opengl_IsOkay();
	glActiveTexture( TextureBindings[TextureIndex] );
	Opengl_IsOkay();
	glBindTexture( GL_TEXTURE_2D, Texture.mTexture.mName );
	Opengl_IsOkay();
}

namespace Soy
{
	//	returns if changed
	bool	StringReplace(std::string& str,const std::string& from,const std::string& to)
	{
		if ( from.empty() )
			return false;
		size_t Changes = 0;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		
			Changes++;
		}
		return (Changes!=0);
	}
}

Opengl::GlProgram Opengl::BuildProgram(std::string vertexSrc,std::string fragmentSrc,const TGeometryVertex& Vertex)
{
	if ( !Opengl_IsInitialised() )
		return GlProgram();
	
	GlProgram prog;
	
	bool ShaderVersion30 = true;
	
	if ( ShaderVersion30 )
	{
#if defined(TARGET_ANDROID)
		const char* Version_3_2 = "#version 300 es";
#else
		const char* Version_3_2 = "#version 150";
#endif
		//	auto-replace/insert the new fragment output
		//	https://www.opengl.org/wiki/Fragment_Shader#Outputs
		const char* AutoFragColorName = "FragColor";
		
		//	in 3.2, attribute/varying is now in/out
		//			varying is Vert OUT, and INPUT for a frag (it becomes an attribute of the pixel)
		Soy::StringReplace( vertexSrc, "attribute", "in" );
		Soy::StringReplace( fragmentSrc, "attribute", "in" );
		Soy::StringReplace( vertexSrc, "varying", "out" );
		Soy::StringReplace( fragmentSrc, "varying", "in" );
		Soy::StringReplace( fragmentSrc, "gl_FragColor", AutoFragColorName );
		Soy::StringReplace( fragmentSrc, "texture2D(", "texture(" );

		//	need a version
		std::stringstream InsertVert;
		InsertVert << Version_3_2 << std::endl;
		vertexSrc.insert( 0, InsertVert.str() );
	
		//	insert some stuff into frag
		std::stringstream InsertFrag;
		InsertFrag << Version_3_2 << std::endl;
		
		//	auto declare the new gl_FragColor variable
		InsertFrag << "out vec4 " << AutoFragColorName << ";" << std::endl;
		fragmentSrc.insert( 0, InsertFrag.str() );
	}
	
	prog.vertexShader.mName = glCreateShader( GL_VERTEX_SHADER );
	if ( !CompileShader( prog.vertexShader, vertexSrc.c_str() ) )
	{
		Soy::Assert( false, "Failed to compile vertex shader" );
		return GlProgram();
	}
	prog.fragmentShader.mName = glCreateShader( GL_FRAGMENT_SHADER );
	if ( !CompileShader( prog.fragmentShader, fragmentSrc.c_str() ) )
	{
		Soy::Assert( false, "Failed to compile fragment shader" );
		return GlProgram();
	}
	
	prog.program.mName = glCreateProgram();
	auto& ProgramName = prog.program.mName;
	glAttachShader( ProgramName, prog.vertexShader.mName );
	glAttachShader( ProgramName, prog.fragmentShader.mName );
	
	//	bind attributes before linking to match geometry
	for ( int i=0;	i<Vertex.mElements.GetSize();	i++ )
	{
		auto& Attrib = Vertex.mElements[i];
	//	glBindAttribLocation( ProgramName, Attrib.mIndex, Attrib.mName.c_str() );
	}
	
	// link and error check
	glLinkProgram( ProgramName );
	GLint r = GL_FALSE;
	glGetProgramiv( ProgramName, GL_LINK_STATUS, &r );
	if ( r == GL_FALSE )
	{
		GLchar msg[1024];
		glGetProgramInfoLog( ProgramName, sizeof( msg ), 0, msg );
		std::stringstream Error;
		Error << "Failed to link vertex and fragment shader: " << msg;
		Soy::Assert( false, Error.str() );
		return GlProgram();
	}
	
	glGetProgramiv( ProgramName, GL_VALIDATE_STATUS, &r );
	if ( r == GL_FALSE )
	{
		GLchar msg[1024];
		glGetProgramInfoLog( ProgramName, sizeof( msg ), 0, msg );
		std::stringstream Error;
		Error << "Failed to validate vertex and fragment shader: " << msg;
		std::Debug << Error.str() << std::endl;
		//Soy::Assert( false, Error.str() );
		//return GlProgram();
	}
	
	//	enum attribs and uniforms
	//	lookout for new version;	http://stackoverflow.com/a/12611619/355753
	GLint numActiveAttribs = 0;
	GLint numActiveUniforms = 0;
	glGetProgramiv( ProgramName, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
	glGetProgramiv( ProgramName, GL_ACTIVE_UNIFORMS, &numActiveUniforms);
	//	gr: look out for driver bugs: http://stackoverflow.com/a/12611619/355753
	numActiveUniforms = std::min( numActiveUniforms, 256 );

	GLint MaxNameLength = 0;
	glGetProgramiv( ProgramName, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &MaxNameLength );

	for( GLint attrib=0;	attrib<numActiveAttribs;	++attrib )
	{
		std::vector<GLchar> nameData( MaxNameLength );
		
		TUniform Uniform;
		Uniform.mIndex = attrib;
		
		GLsizei actualLength = 0;
		glGetActiveAttrib( ProgramName, attrib, size_cast<GLsizei>(nameData.size()), &actualLength, &Uniform.mArraySize, &Uniform.mType, &nameData[0]);
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	check is valid type etc
		
		prog.mAttributes.PushBack( Uniform );
	}
	
	for( GLint attrib=0;	attrib<numActiveUniforms;	++attrib )
	{
		std::vector<GLchar> nameData( MaxNameLength );
		
		TUniform Uniform;
		Uniform.mIndex = attrib;
		
		GLsizei actualLength = 0;
		glGetActiveUniform( ProgramName, attrib, size_cast<GLsizei>(nameData.size()), &actualLength, &Uniform.mArraySize, &Uniform.mType, &nameData[0]);
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	check is valid type etc
		
		prog.mUniforms.PushBack( Uniform );
	}

	std::Debug << "Shader has " << prog.mAttributes.GetSize() << " attributes; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(prog.mAttributes), "\n" );
	std::Debug << std::endl;

	std::Debug << "Shader has " << prog.mUniforms.GetSize() << " uniforms; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(prog.mUniforms), "\n" );
	std::Debug << std::endl;
	
	Opengl_IsOkay();
	glValidateProgram( ProgramName );
	Opengl_IsOkay();
	
	//	gr: runtime binding!
	/*
	glUseProgram( ProgramName );
	
	// texture and image_external bindings
	for ( int i = 0; i < 8; i++ )
	{
		char name[32];
		sprintf( name, "Texture%i", i );
		const GLint uTex = glGetUniformLocation( ProgramName, name );
		if ( uTex != -1 )
		{
			glUniform1i( uTex, i );
		}
	}
	
	glUseProgram( 0 );
	 */
	
	return prog;
}


bool Opengl::GlProgram::IsValid() const
{
	return program.IsValid() && vertexShader.IsValid() && fragmentShader.IsValid();
}

Opengl::TShaderState Opengl::GlProgram::Bind()
{
	Opengl_IsOkay();
	glUseProgram( program.mName );
	Opengl_IsOkay();
	
	TShaderState ShaderState( *this );
	return ShaderState;
}

void Opengl::GlProgram::Destroy()
{
	if ( program.mName != GL_ASSET_INVALID )
	{
		glDeleteProgram( program.mName );
		program.mName = GL_ASSET_INVALID;
	}
	if ( vertexShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( vertexShader.mName );
		vertexShader.mName = GL_ASSET_INVALID;
	}
	if ( fragmentShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( fragmentShader.mName );
		fragmentShader.mName = GL_ASSET_INVALID;
	}
}


#if defined(TARGET_ANDROID)
#define glBindVertexArray	glBindVertexArrayOES_
#define glGenVertexArrays	glGenVertexArraysOES_
#endif


size_t Opengl::TGeometryVertex::GetDataSize() const
{
	size_t Size = 0;
	for ( int e=0;	e<mElements.GetSize();	e++ )
		Size += mElements[e].mElementDataSize;
	return Size;
}

size_t Opengl::TGeometryVertex::GetOffset(size_t ElementIndex) const
{
	size_t Size = 0;
	for ( int e=0;	e<ElementIndex;	e++ )
		Size += mElements[e].mElementDataSize;
	return Size;
}

size_t Opengl::TGeometryVertex::GetStride(size_t ElementIndex) const
{
	//	gr: this is for interleaved vertexes
	//	todo: handle serial elements AND mixed interleaved & serial
	//	serial elements would be 0
	size_t Stride = GetDataSize();
	Stride -= mElements[ElementIndex].mElementDataSize;
	return Stride;
}


/*
 //	calc stuff
	Array<size_t> VertexOffsets;
	Array<size_t> VertexStrides;
	size_t VertexSize = 0;
	{
 size_t VertexOffset = 0;
 for ( int v=0;	v<Attribs.GetSize();	v++ )
 {
 //	this changes if we have interleaved attribs
 VertexOffsets.PushBack( VertexOffset );
 VertexStrides.PushBack( Attribs[v].mElementDataSize );
 
 VertexOffset += Attribs[v].mElementDataSize;
 VertexSize += Attribs[v].mElementDataSize;
 }
	}
 */

void Opengl::TGeometryVertex::EnableAttribs(bool Enable) const
{
	for ( int at=0;	at<mElements.GetSize();	at++ )
	{
		auto& Element = mElements[at];
		auto AttribIndex = Element.mIndex;
		
		if ( Enable )
			glEnableVertexAttribArray( AttribIndex );
		else
			glDisableVertexAttribArray( AttribIndex );
		Opengl_IsOkay();
	}
}

Opengl::TGeometry Opengl::CreateGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<GLshort>&& Indexes,const Opengl::TGeometryVertex& Vertex)
{
	TGeometry Geo;
	Geo.mVertexDescription = Vertex;
	
	glGenBuffers( 1, &Geo.vertexBuffer );
	glGenBuffers( 1, &Geo.indexBuffer );

	//	fill vertex array
	glGenVertexArrays( 1, &Geo.vertexArrayObject );
	glBindVertexArray( Geo.vertexArrayObject );
	glBindBuffer( GL_ARRAY_BUFFER, Geo.vertexBuffer );
	Opengl_IsOkay();

	
	for ( int at=0;	at<Vertex.mElements.GetSize();	at++ )
	{
		auto& Element = Vertex.mElements[at];
		auto ElementOffset = Vertex.GetOffset(at);
		GLsizei Stride = Vertex.GetStride(at);
		GLboolean Normalised = Element.mNormalised;
		
		void* ElementPointer = (void*)ElementOffset;
		auto AttribIndex = Element.mIndex;
		
		glEnableVertexAttribArray( AttribIndex );
		glVertexAttribPointer( AttribIndex, Element.mArraySize, Element.mType, Normalised, Stride, ElementPointer );
		Opengl_IsOkay();
	}
	
	//	push data
	glBufferData( GL_ARRAY_BUFFER, Data.GetDataSize(), Data.GetArray(), GL_STATIC_DRAW );
	Geo.vertexCount = size_cast<GLsizei>( Data.GetDataSize() / Vertex.GetDataSize() );
	Opengl_IsOkay();
	
	//	gr: don't need to do this?
	Vertex.DisableAttribs();

	//	push indexes
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, Geo.indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, Indexes.GetDataSize(), Indexes.GetArray(), GL_STATIC_DRAW );
	Geo.indexCount = size_cast<GLsizei>( Indexes.GetSize() );
	Geo.mIndexType = Opengl::GetTypeEnum<GLshort>();
	Opengl_IsOkay();

	
	//	unbind
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
	Opengl_IsOkay();
	

	
	return Geo;
}

template< typename _attrib_type_ >
void PackVertexAttribute( std::vector< uint8_t > & packed, const std::vector< _attrib_type_ > & attrib,
						 const int glLocation, const int glType, const int glComponents )
{
	if ( attrib.size() > 0 )
	{
		const size_t offset = packed.size();
		const size_t size = attrib.size() * sizeof( attrib[0] );
		
		packed.resize( offset + size );
		memcpy( &packed[offset], attrib.data(), size );
		
		glEnableVertexAttribArray( glLocation );
		glVertexAttribPointer( glLocation, glComponents, glType, false, sizeof( attrib[0] ), (void *)( offset ) );
	}
	else
	{
		glDisableVertexAttribArray( glLocation );
	}
}

/*
void Opengl::TGeometry::Create( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices )
{
	if ( !Opengl_IsInitialised() )
		return;
	
	vertexCount = size_cast<GLsizei>(attribs.position.size());
	indexCount = size_cast<GLsizei>(indices.size());
	
	std::Debug << "glGenBuffers" << std::endl;
	glGenBuffers( 1, &vertexBuffer );
	Opengl::IsOkay("glGenBuffers");
	glGenBuffers( 1, &indexBuffer );
	Opengl::IsOkay("glGenBuffers");
	
	glGenVertexArrays( 1, &vertexArrayObject );
	Opengl::IsOkay("glGenVertexArrays");
	
	glBindVertexArray( vertexArrayObject );
	Opengl::IsOkay("glBindVertexArray");
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	Opengl::IsOkay("glBindBuffer vertex");
	
	std::Debug << "PackVertexAttribute..." << std::endl;
	std::vector< uint8_t > packed;
	PackVertexAttribute( packed, attribs.position,		VERTEX_ATTRIBUTE_LOCATION_POSITION,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.normal,		VERTEX_ATTRIBUTE_LOCATION_NORMAL,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.tangent,		VERTEX_ATTRIBUTE_LOCATION_TANGENT,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.binormal,		VERTEX_ATTRIBUTE_LOCATION_BINORMAL,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.color,			VERTEX_ATTRIBUTE_LOCATION_COLOR,			GL_FLOAT,	4 );
	PackVertexAttribute( packed, attribs.uv0,			VERTEX_ATTRIBUTE_LOCATION_UV0,				GL_FLOAT,	2 );
	PackVertexAttribute( packed, attribs.uv1,			VERTEX_ATTRIBUTE_LOCATION_UV1,				GL_FLOAT,	2 );
	//PackVertexAttribute( packed, attribs.jointIndices,	VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES,	GL_INT,		4 );
	//PackVertexAttribute( packed, attribs.jointWeights,	VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	GL_FLOAT,	4 );
	
	std::Debug << "glBufferData" << std::endl;
	glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );
	Opengl::IsOkay("glBufferData vertex");
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	Opengl::IsOkay("glBindBuffer initidies");
	
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( indices[0] ), indices.data(), GL_STATIC_DRAW );
	Opengl::IsOkay("glBufferData indicies");
	
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
	
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_POSITION );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_NORMAL );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_TANGENT );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_BINORMAL );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_COLOR );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_UV0 );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_UV1 );
	//glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES );
	//glDisableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS );
	std::Debug << "TGeometry::Create finished" << std::endl;
	Opengl_IsOkay();
}

void Opengl::TGeometry::Update( const VertexAttribs & attribs )
{
	vertexCount = attribs.position.size();
	
	glBindVertexArray( vertexArrayObject );
	
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	
	std::vector< uint8_t > packed;
	PackVertexAttribute( packed, attribs.position,		VERTEX_ATTRIBUTE_LOCATION_POSITION,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.normal,		VERTEX_ATTRIBUTE_LOCATION_NORMAL,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.tangent,		VERTEX_ATTRIBUTE_LOCATION_TANGENT,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.binormal,		VERTEX_ATTRIBUTE_LOCATION_BINORMAL,			GL_FLOAT,	3 );
	PackVertexAttribute( packed, attribs.color,			VERTEX_ATTRIBUTE_LOCATION_COLOR,			GL_FLOAT,	4 );
	PackVertexAttribute( packed, attribs.uv0,			VERTEX_ATTRIBUTE_LOCATION_UV0,				GL_FLOAT,	2 );
	PackVertexAttribute( packed, attribs.uv1,			VERTEX_ATTRIBUTE_LOCATION_UV1,				GL_FLOAT,	2 );
	//PackVertexAttribute( packed, attribs.jointIndices,	VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES,	GL_INT,		4 );
	//PackVertexAttribute( packed, attribs.jointWeights,	VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	GL_FLOAT,	4 );
	
	glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );
}
 */

void Opengl::TGeometry::Draw() const
{
	if ( !Opengl_IsInitialised() )
		return;
	
	//	null to draw from indexes in vertex array
	glBindVertexArray( vertexArrayObject );
	Opengl_IsOkay();
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	
	mVertexDescription.EnableAttribs();
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	//glBindBuffer(GL_ARRAY_BUFFER,0);
	Opengl_IsOkay();

	glDrawArrays( GL_TRIANGLES, 0, 2 );
	Opengl_IsOkay();
	
	
	glDrawElements( GL_TRIANGLES, indexCount, mIndexType, nullptr );
	Opengl::IsOkay("glDrawElements");
}

void Opengl::TGeometry::Free()
{
	glDeleteVertexArrays( 1, &vertexArrayObject );
	glDeleteBuffers( 1, &indexBuffer );
	glDeleteBuffers( 1, &vertexBuffer );
	
	indexBuffer = 0;
	vertexBuffer = 0;
	vertexArrayObject = 0;
	vertexCount = 0;
	indexCount = 0;
}


bool Opengl::TGeometry::IsValid() const
{
	return	vertexBuffer != GL_ASSET_INVALID &&
	indexBuffer != GL_ASSET_INVALID &&
	vertexArrayObject != GL_ASSET_INVALID &&
	vertexCount != GL_ASSET_INVALID &&
	indexCount != GL_ASSET_INVALID;
}



const std::map<SoyPixelsFormat::Type,GLint>& Opengl::GetPixelFormatMap()
{
	static std::map<SoyPixelsFormat::Type,GLint> PixelFormatMap =
	{
#if defined(TARGET_IOS)
		std::make_pair( SoyPixelsFormat::RGBA, GL_RGBA ),
#endif
#if defined(TARGET_WINDOWS)
		std::make_pair( SoyPixelsFormat::RGB, GL_RGB ),
		std::make_pair( SoyPixelsFormat::RGBA, GL_RGBA ),
		std::make_pair( SoyPixelsFormat::Greyscale, GL_LUMINANCE ),
#endif
#if defined(GL_VERSION_3_0)
		std::make_pair( SoyPixelsFormat::RGB, GL_RGB ),
		std::make_pair( SoyPixelsFormat::RGBA, GL_RGBA ),
		std::make_pair( SoyPixelsFormat::BGR, GL_BGR ),
		std::make_pair( SoyPixelsFormat::Greyscale, GL_RED ),
		std::make_pair( SoyPixelsFormat::GreyscaleAlpha, GL_RG ),
#endif
	};
	return PixelFormatMap;
}

GLuint Opengl::GetPixelFormat(SoyPixelsFormat::Type Format)
{
	auto& Map = GetPixelFormatMap();
	auto it = Map.find( Format );
	if ( it == Map.end() )
		return GL_INVALID_VALUE;
	return it->second;
}

SoyPixelsFormat::Type Opengl::GetPixelFormat(GLuint glFormat)
{
	//	gr: there was special code here, check it before removing
	switch ( glFormat )
	{
		//	gr: osx returns these values hmmmm
		case GL_RGBA8:	return SoyPixelsFormat::RGBA;
		case GL_RGB8:	return SoyPixelsFormat::RGB;
		case GL_RGB:	return SoyPixelsFormat::RGB;
		case GL_RGBA:	return SoyPixelsFormat::RGBA;
		case GL_BGRA:	return SoyPixelsFormat::BGRA;
		default:
			break;
	}

	//	check the map
	auto& Map = GetPixelFormatMap();
	for ( auto it=Map.begin();	it!=Map.end();	it++ )
	{
		if ( it->second == glFormat )
			return it->first;
	}
	return SoyPixelsFormat::Invalid;
}


void Opengl::SetViewport(Soy::Rectf Viewport)
{
	glViewport( Viewport.x, Viewport.y, Viewport.w, Viewport.h );
}

void Opengl::ClearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Opengl::ClearColour(Soy::TRgb Colour)
{
	float Alpha = 1;
	glClearColor( Colour.r(), Colour.g(), Colour.b(), Alpha );
	glClear(GL_COLOR_BUFFER_BIT);
}
