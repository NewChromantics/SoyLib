#include "SoyOpengl.h"
#include "RemoteArray.h"

namespace Opengl
{
	const std::map<SoyPixelsFormat::Type,GLenum>&	GetPixelFormatMap();
	
	template<typename TYPE>
	GLenum			GetTypeEnum()
	{
		throw Soy::AssertException( std::string("Unhandled Get Gl enum from type case ")+__func__ );
		return GL_INVALID_ENUM;
	}
	
	template<typename TYPE>
	void			SetUniform(const TUniform& Uniform,const TYPE& Value)
	{
		std::stringstream Error;
		Error << __func__ << " not overloaded for " << Soy::GetTypeName<TYPE>();
		throw Soy::AssertException( Error.str() );
	}

}

template<> GLenum Opengl::GetTypeEnum<uint16>()		{	return GL_UNSIGNED_SHORT;	}
template<> GLenum Opengl::GetTypeEnum<GLshort>()	{	return GL_UNSIGNED_SHORT;	}

std::string Opengl::GetEnumString(GLenum Type)
{
#define CASE_ENUM_STRING(e)	case (e):	return #e;
	switch ( Type )
	{
			//	errors
			CASE_ENUM_STRING( GL_NO_ERROR );
			CASE_ENUM_STRING( GL_INVALID_ENUM );
			CASE_ENUM_STRING( GL_INVALID_VALUE );
			CASE_ENUM_STRING( GL_INVALID_OPERATION );
			CASE_ENUM_STRING( GL_INVALID_FRAMEBUFFER_OPERATION );
			CASE_ENUM_STRING( GL_OUT_OF_MEMORY );
#if defined(GL_STACK_UNDERFLOW)
			CASE_ENUM_STRING( GL_STACK_UNDERFLOW );
#endif
#if defined(GL_STACK_OVERFLOW)
			CASE_ENUM_STRING( GL_STACK_OVERFLOW );
#endif
			
			//	types
			CASE_ENUM_STRING( GL_BYTE );
			CASE_ENUM_STRING( GL_UNSIGNED_BYTE );
			CASE_ENUM_STRING( GL_SHORT );
			CASE_ENUM_STRING( GL_UNSIGNED_SHORT );
			CASE_ENUM_STRING( GL_INT );
			CASE_ENUM_STRING( GL_UNSIGNED_INT );
			CASE_ENUM_STRING( GL_FLOAT );
			CASE_ENUM_STRING( GL_FLOAT_VEC2 );
			CASE_ENUM_STRING( GL_FLOAT_VEC3 );
			CASE_ENUM_STRING( GL_FLOAT_VEC4 );
			CASE_ENUM_STRING( GL_INT_VEC2 );
			CASE_ENUM_STRING( GL_INT_VEC3 );
			CASE_ENUM_STRING( GL_INT_VEC4 );
			CASE_ENUM_STRING( GL_BOOL );
			CASE_ENUM_STRING( GL_SAMPLER_2D );
			CASE_ENUM_STRING( GL_SAMPLER_CUBE );
			CASE_ENUM_STRING( GL_FLOAT_MAT2 );
			CASE_ENUM_STRING( GL_FLOAT_MAT3 );
			CASE_ENUM_STRING( GL_FLOAT_MAT4 );
#if defined(OPENGL_CORE_3)
			CASE_ENUM_STRING( GL_DOUBLE );
			CASE_ENUM_STRING( GL_SAMPLER_1D );
			CASE_ENUM_STRING( GL_SAMPLER_3D );
#endif
			
			//	colours
#if !defined(TARGET_ANDROID)
			CASE_ENUM_STRING( GL_BGRA );
#endif
			CASE_ENUM_STRING( GL_RGBA );
			CASE_ENUM_STRING( GL_RGB );
			CASE_ENUM_STRING( GL_RGB8 );
			CASE_ENUM_STRING( GL_RED );
	};
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "Unknown GL enum 0x" << std::hex << Type;
	return Unknown.str();
}

std::ostream& Opengl::operator<<(std::ostream &out,const Opengl::TUniform& in)
{
	out << "#" << in.mIndex << " ";
	out << Opengl::GetEnumString(in.mType);
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

template<>
void Opengl::SetUniform(const TUniform& Uniform,const vec2f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform2fv( Uniform.mIndex, ArraySize, &Value.x );
	Opengl_IsOkay();
}



bool CompileShader(const Opengl::TAsset& Shader,ArrayBridge<std::string>&& SrcLines,const std::string& ErrorPrefix,std::ostream& Error)
{
	Array<const GLchar*> Lines;
	for ( int i=0;	i<SrcLines.GetSize();	i++ )
	{
		SrcLines[i] += "\n";
		Lines.PushBack( SrcLines[i].c_str() );
	}
	const GLint* LineLengths = nullptr;
	glShaderSource( Shader.mName, size_cast<GLsizei>(Lines.GetSize()), Lines.GetArray(), LineLengths );
	Opengl::IsOkay("glShaderSource");
	glCompileShader( Shader.mName );
	Opengl::IsOkay("glCompileShader");
	
	GLint r = GL_FALSE;
	glGetShaderiv( Shader.mName, GL_COMPILE_STATUS, &r );
	Opengl::IsOkay("glGetShaderiv(GL_COMPILE_STATUS)");
	if ( r == GL_FALSE )
	{
		Error << ErrorPrefix << " Compiling shader error: ";
		GLchar msg[4096] = {0};
		glGetShaderInfoLog( Shader.mName, sizeof( msg ), 0, msg );
		Error << msg << std::endl;
		return false;
	}
	return true;
}

bool Opengl::IsInitialised(const std::string &Context,bool ThrowException)
{
	return true;
}

bool Opengl::IsOkay(const std::string& Context,bool ThrowException)
{
	if ( !IsInitialised(std::string("CheckIsOkay") + Context, false ) )
		return false;
	
	auto Error = glGetError();
	if ( Error == GL_NONE )
		return true;
	
	std::stringstream ErrorStr;
	ErrorStr << "Opengl error in " << Context << ": " << Opengl::GetEnumString(Error) << std::endl;
	
	if ( !ThrowException )
	{
		std::Debug << ErrorStr.str() << std::endl;
		return false;
	}
	
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
	//	todo: if invalid texture, make one!
	Soy::Assert( Texture.IsValid(), "invalid texture" );
	
	auto& mFboTextureName = mTarget.mTexture.mName;
	auto& mType = mTarget.mType;
	auto& mFboMeta = mTarget.mMeta;

	//	gr: added to try and get IOS working
#if defined(TARGET_IOS)
	//	remove other mip levels
	glBindTexture( mType, mFboTextureName );
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, 0);
	glBindTexture( mType, 0 );
#endif
	
	std::Debug << "Creating FBO " << mFboMeta << ", texture name: " << mFboTextureName << std::endl;
	
	glGenFramebuffers( 1, &mFbo.mName );
	Opengl::IsOkay("FBO glGenFramebuffers");
	glBindFramebuffer( GL_FRAMEBUFFER, mFbo.mName );
	Opengl::IsOkay("FBO glBindFramebuffer2");
	
	GLint MipLevel = 0;
	if ( mType == GL_TEXTURE_2D )
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mType, mFboTextureName, MipLevel );
	else
		throw Soy::AssertException("Don't currently support frame buffer texture if not GL_TEXTURE_2D");
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

void Opengl::TFbo::InvalidateContent()
{
#if defined(OPENGL_ES_3)
	//	gr: not needed, but I think is a performance boost?
	//		maybe a proper performance boost would be AFTER using it to copy to texture
	const GLenum fboAttachments[1] = { GL_COLOR_ATTACHMENT0 };
	glInvalidateFramebuffer( GL_FRAMEBUFFER, 1, fboAttachments );
	Opengl_IsOkay();
#endif
	
	/*
	 const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
	 glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	 glDiscardFramebufferEXT(GL_FRAMEBUFFER,1,discards);
	 */
}

bool Opengl::TFbo::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo.mName );
	Opengl_IsOkay();
	
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	Opengl_IsOkay();

	Soy::Rectf FrameBufferRect( 0, 0, mTarget.mMeta.GetWidth(), mTarget.mMeta.GetHeight() );
	Opengl::SetViewport( FrameBufferRect );
	Opengl_IsOkay();
	
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

Opengl::TTexture::TTexture(SoyPixelsMetaFull Meta,GLenum Type) :
	mAutoRelease	( true ),
	mType			( Type )
{
	Opengl_IsInitialised();
	Soy::Assert( Meta.IsValid(), "Cannot setup texture with invalid meta" );
	
	GLuint Format = Opengl::GetPixelFormat( Meta.GetFormat() );
	if ( Format == GL_INVALID_VALUE )
	{
		std::stringstream Error;
		Error << "Failed to create texture, unsupported format " << Meta.GetFormat();
		throw Soy::AssertException( Error.str() );
	}

	//	allocate texture
	glGenTextures( 1, &mTexture.mName );
	Opengl::IsOkay("glGenTextures");
	
	if ( !Bind() )
		throw Soy::AssertException("failed to bind after texture allocation");
	Opengl::IsOkay("glGenTextures");
	
	//	set mip-map levels to 0..0
	//	gr: change this to glGenerateMipMaps etc
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, 0);

	//	initialise to set dimensions
	SoyPixels InitFramePixels;
	InitFramePixels.Init( Meta.GetWidth(), Meta.GetHeight(), Meta.GetFormat() );
	auto& PixelsArray = InitFramePixels.GetPixelsArray();
	glTexImage2D( mType, 0, Format, Meta.GetWidth(), Meta.GetHeight(), 0, Format, GL_UNSIGNED_BYTE, PixelsArray.GetArray() );
	//GLint MipLevel = 0;
	//glTexImage2D( mType, MipLevel, Format, Meta.GetWidth(), Meta.GetHeight(), 0, Format, GL_UNSIGNED_BYTE, nullptr );
	Opengl::IsOkay("glTexImage2D");
	
	Unbind();
	
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
	glBindTexture( mType, mTexture.mName );
	return Opengl_IsOkay();
}

void Opengl::TTexture::Unbind()
{
	glBindTexture( mType, GL_ASSET_INVALID );
}

void Opengl::TTexture::Copy(const SoyPixelsImpl& SourcePixels,bool Stretch,bool DoConversion)
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
	TextureWidth = this->GetWidth();
	TextureHeight = this->GetHeight();
	TextureInternalFormat = GetPixelFormat( this->GetFormat() );
#else
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()" );
#endif

	
	//	pixel data format
	auto GlPixelsFormat = Opengl::GetUploadPixelFormat( *this, SourcePixels.GetFormat() );

	//	gr: take IOS's target-must-match-source requirement into consideration here (replace GetUploadPixelFormat)
	SoyPixels ConvertedPixels;
	const SoyPixelsImpl* UsePixels = &SourcePixels;
	if ( DoConversion )
	{
		//	convert to upload-compatible type
		Array<SoyPixelsFormat::Type> TryFormats;
		TryFormats.PushBack( SoyPixelsFormat::RGB );
		TryFormats.PushBack( SoyPixelsFormat::RGBA );
		TryFormats.PushBack( SoyPixelsFormat::Greyscale );
		
		while ( GlPixelsFormat == GL_INVALID_VALUE && !TryFormats.IsEmpty() )
		{
			auto TryFormat = TryFormats.PopAt(0);
			auto TryGlFormat = Opengl::GetUploadPixelFormat( *this, TryFormat );
			if ( TryGlFormat == GL_INVALID_VALUE )
				continue;
			
			if ( !ConvertedPixels.Copy(SourcePixels ) )
				continue;
			if ( !ConvertedPixels.SetFormat( TryFormat ) )
				continue;
			
			GlPixelsFormat = TryGlFormat;
			UsePixels = &ConvertedPixels;
		}
		
	}
	
	//	we CANNOT go from big to small, only small to big, so to avoid corrupted textures, lets shrink
	SoyPixels Resized;
	static bool AllowResize = true;
	if ( AllowResize )
	{
		auto PixelsWidth = UsePixels->GetWidth();
		auto PixelsHeight = UsePixels->GetHeight();
		if ( TextureWidth < PixelsWidth || TextureHeight < PixelsHeight )
		{
			std::Debug << "Warning, pixels(" << PixelsWidth << "x" << PixelsHeight << ") too large for texture(" << TextureWidth << "x" << TextureHeight << "), resizing on CPU" << std::endl;
			Resized.Copy( *UsePixels );
			Resized.ResizeClip( std::min<uint16>(TextureWidth,PixelsWidth), std::min<uint16>(TextureHeight,PixelsHeight) );
			UsePixels = &Resized;
		}
	}
	
	auto& FinalPixels = *UsePixels;
	
	
	//	todo: find when we NEED to make a new texture (uninitialised)
	bool SubImage = !Stretch;

#if defined(TARGET_OSX)
	if ( !SubImage && Opengl::IsSupported("GL_APPLE_client_storage") )
	{
		//	https://developer.apple.com/library/mac/documentation/graphicsimaging/conceptual/opengl-macprogguide/opengl_texturedata/opengl_texturedata.html
		glTexParameteri(mType,
						GL_TEXTURE_STORAGE_HINT_APPLE,
						GL_STORAGE_CACHED_APPLE);
			
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
		
		//	gr: need a buffering system to save a buffer per texture then just update the pixels
		static SoyPixels PixelsBuffer;
		PixelsBuffer.Copy( FinalPixels );
			
		GLenum TargetFormat = GL_BGRA;
		GLenum TargetStorage = GL_UNSIGNED_INT_8_8_8_8_REV;
		glTexImage2D(mType, MipLevel, GlPixelsFormat, PixelsBuffer.GetWidth(), PixelsBuffer.GetHeight(), 0, TargetFormat, TargetStorage, PixelsBuffer.GetPixelsArray().GetArray() );
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
	
		//	only for "new" textures
		GLuint TargetFormat = GL_RGBA;
	
		glTexImage2D( mType, MipLevel, TargetFormat,  Width, Height, Border, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
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

		static bool ForceSize = 0;
		if ( ForceSize != 0 )
		{
			Width = ForceSize;
			Height = ForceSize;
		}
		
		//	invalid operation here means the unity pixel format is probably different to the pixel data we're trying to push now
		glTexSubImage2D( mType, MipLevel, XOffset, YOffset, Width, Height, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
		
		std::stringstream Context;
		Context << __func__ << "glTexSubImage2D(" << Opengl::GetEnumString(GlPixelsFormat) << ")";
		
		if ( !Opengl::IsOkay( Context.str(),false) )
		{
			//	on ios the internal format must match the pixel format. No conversion!
			GLenum TargetFormat = GlPixelsFormat;
			int Border = 0;
			
			//	gr: first copy needs to initialise the texture... before we can use subimage
			glTexImage2D( GL_TEXTURE_2D, MipLevel, TargetFormat, Width, Height, Border, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
			Opengl::IsOkay("glTexImage2D",false);
		}
	}

	glGenerateMipmap( mType );
	Opengl_IsOkay();
	
	Unbind();
	Opengl_IsOkay();
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



void Opengl::TShaderState::SetUniform(const std::string& Name,const vec4f& v)
{
	auto Uniform = mShader.GetUniform( Name );
	Soy::Assert( Uniform.IsValid(), std::stringstream() << "Invalid uniform " << Name );
	Opengl::SetUniform( Uniform, v );
}


void Opengl::TShaderState::SetUniform(const std::string& Name,const vec2f& v)
{
	auto Uniform = mShader.GetUniform( Name );
	Soy::Assert( Uniform.IsValid(), std::stringstream() << "Invalid uniform " << Name );
	Opengl::SetUniform( Uniform, v );
}

void Opengl::TShaderState::SetUniform(const std::string& Name,const TTexture& Texture)
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
	glBindTexture( Texture.mType, Texture.mTexture.mName );
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
	
	//	returns if changed
	bool	StringReplace(ArrayBridge<std::string>& str,const std::string& from,const std::string& to)
	{
		bool Changed = false;
		for ( int i=0;	i<str.GetSize();	i++ )
		{
			Changed |= StringReplace( str[i], from, to );
		}
		return Changed;
	}

	bool	StringReplace(ArrayBridge<std::string>&& str,const std::string& from,const std::string& to)
	{
		return StringReplace(str,from,to);
	}
}


size_t GetNonProcessorFirstLine(ArrayBridge<std::string>& Shader)
{
	size_t LastProcessorDirectiveLine = 0;
	for ( int i=0;	i<Shader.GetSize();	i++ )
	{
		auto& Line = Shader[i];
		if ( Line.empty() )
			continue;
		
		bool IsHeader = false;
		
		if ( Line[0] == '#' )
			IsHeader = true;

		if ( Soy::StringBeginsWith(Line,"precision ",true) )
			IsHeader = true;
		
		if ( !IsHeader )
			continue;

		LastProcessorDirectiveLine = i;
	}
	return LastProcessorDirectiveLine+1;
}

//	vert & frag changes
void UpgradeShader(ArrayBridge<std::string>& Shader,size_t Version)
{
	//	insert version if there isn't one there
	if ( !Soy::StringBeginsWith(Shader[0],"#version",true) )
	{
		std::stringstream VersionStr;
		VersionStr << "#version " << Version;
		
		//	append ES/Core suffix
		if ( Version == 300 )
			VersionStr << " es";
		else
			VersionStr << " core";
		
		VersionStr<<std::endl;
		Shader.InsertAt( 0, VersionStr.str() );
	}
	
#if defined(TARGET_IOS)
	if ( Version == 300 )
	{
		//	ios requires precision
		//	gr: add something to check if this is already declared
		Shader.InsertAt( GetNonProcessorFirstLine(Shader), "precision highp float;\n" );
	}
#endif

}

void Opengl::UpgradeVertShader(ArrayBridge<std::string>&& Shader,size_t Version)
{
	UpgradeShader( Shader, Version );
	
	//	in 3.2, attribute/varying is now in/out
	//			varying is Vert OUT, and INPUT for a frag (it becomes an attribute of the pixel)
	Soy::StringReplace( Shader, "attribute", "in" );
	Soy::StringReplace( Shader, "varying", "out" );
}


void Opengl::UpgradeFragShader(ArrayBridge<std::string>&& Shader,size_t Version)
{
	UpgradeShader( Shader, Version );

	//	auto-replace/insert the new fragment output
	//	https://www.opengl.org/wiki/Fragment_Shader#Outputs
	const char* AutoFragColorName = "FragColor";
	
	//	in 3.2, attribute/varying is now in/out
	//			varying is Vert OUT, and INPUT for a frag (it becomes an attribute of the pixel)
	Soy::StringReplace( Shader, "attribute", "in" );
	Soy::StringReplace( Shader, "varying", "in" );
	Soy::StringReplace( Shader, "gl_FragColor", AutoFragColorName );
	Soy::StringReplace( Shader, "texture2D(", "texture(" );
	

	//	auto declare the new gl_FragColor variable after all processor directives
	std::stringstream FragVariable;
	FragVariable << "out vec4 " << AutoFragColorName << ";" << std::endl;
	Shader.InsertAt( GetNonProcessorFirstLine(Shader), FragVariable.str() );
/*
	std::Debug << "upgraded frag shader:" << std::endl;
	for ( int i=0;	i<Shader.GetSize();	i++ )
	{
		std::Debug << "Frag[" << i << "]" << Shader[i] << std::endl;
	}
	std::Debug << "<<EOF" << std::endl;
	*/
}

Opengl::GlProgram Opengl::BuildProgram(const std::string& vertexSrc,const std::string& fragmentSrc,const TGeometryVertex& Vertex,const std::string& ShaderName)
{
	if ( !Opengl_IsInitialised() )
		return GlProgram();
	
	std::string ShaderNameVert = ShaderName + " (vert)";
	std::string ShaderNameFrag = ShaderName + " (frag)";
	
	GlProgram prog;
	
	Array<std::string> VertShader;
	Array<std::string> FragShader;
	Soy::SplitStringLines( GetArrayBridge(VertShader), vertexSrc );
	Soy::SplitStringLines( GetArrayBridge(FragShader), fragmentSrc );
	
	size_t UpgradeVersion = 0;
	
	//	not required for android
#if defined(OPENGL_ES_3)
	UpgradeVersion = 300;
#endif
	
#if defined(OPENGL_CORE_3)
	UpgradeVersion = 150;
#endif
	
	if ( UpgradeVersion != 0 )
	{
		UpgradeVertShader( GetArrayBridge(VertShader), UpgradeVersion );
		UpgradeFragShader( GetArrayBridge(FragShader), UpgradeVersion );
	}
	
	prog.vertexShader.mName = glCreateShader( GL_VERTEX_SHADER );
	if ( !CompileShader( prog.vertexShader, GetArrayBridge(VertShader), ShaderNameVert, std::Debug ) )
	{
		Soy::Assert( false, "Failed to compile vertex shader" );
		return GlProgram();
	}
	prog.fragmentShader.mName = glCreateShader( GL_FRAGMENT_SHADER );
	if ( !CompileShader( prog.fragmentShader, GetArrayBridge(FragShader), ShaderNameFrag, std::Debug ) )
	{
		Soy::Assert( false, "Failed to compile fragment shader" );
		return GlProgram();
	}
	
	prog.program.mName = glCreateProgram();
	auto& ProgramName = prog.program.mName;
	glAttachShader( ProgramName, prog.vertexShader.mName );
	glAttachShader( ProgramName, prog.fragmentShader.mName );
	
	//	gr: this is not required. We cache the links that the compiler generates AFTERwards
	//	bind attributes before linking to match geometry
	for ( int i=0;	i<Vertex.mElements.GetSize();	i++ )
	{
	//	auto& Attrib = Vertex.mElements[i];
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
		throw Soy::AssertException( Error.str() );
		return GlProgram();
	}
	
	glValidateProgram( ProgramName );
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

	GLint MaxAttribNameLength=0,MaxUniformNameLength=0;
	glGetProgramiv( ProgramName, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &MaxAttribNameLength );
	glGetProgramiv( ProgramName, GL_ACTIVE_UNIFORM_MAX_LENGTH, &MaxUniformNameLength );
	auto MaxNameLength = std::max( MaxAttribNameLength, MaxUniformNameLength );

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

	std::Debug << ShaderName << " has " << prog.mAttributes.GetSize() << " attributes; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(prog.mAttributes), "\n" );
	std::Debug << std::endl;

	std::Debug << ShaderName << " has " << prog.mUniforms.GetSize() << " uniforms; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(prog.mUniforms), "\n" );
	std::Debug << std::endl;

	
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

bool Opengl::TShaderState::IsValid() const
{
	return mShader.IsValid();
}


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

	//	gr: buffer data before setting attrib pointer (needed for ios)
	//	push data
	glBufferData( GL_ARRAY_BUFFER, Data.GetDataSize(), Data.GetArray(), GL_STATIC_DRAW );
	Geo.vertexCount = size_cast<GLsizei>( Data.GetDataSize() / Vertex.GetDataSize() );
	Opengl_IsOkay();
	
	for ( int at=0;	at<Vertex.mElements.GetSize();	at++ )
	{
		auto& Element = Vertex.mElements[at];
		auto ElementOffset = Vertex.GetOffset(at);
		GLsizei Stride = size_cast<GLsizei>(Vertex.GetStride(at));
		GLboolean Normalised = Element.mNormalised;
		
		void* ElementPointer = (void*)ElementOffset;
		auto AttribIndex = Element.mIndex;

		std::Debug << "Pushing attrib " << AttribIndex << ", arraysize " << Element.mArraySize << ", stride " << Stride << std::endl;
		glEnableVertexAttribArray( AttribIndex );
		glVertexAttribPointer( AttribIndex, Element.mArraySize, Element.mType, Normalised, Stride, ElementPointer );
		Opengl_IsOkay();
	}
	
	//	gr: disabling vertex attribs stops rendering on ios
	//Vertex.DisableAttribs();

	//	push indexes
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, Geo.indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, Indexes.GetDataSize(), Indexes.GetArray(), GL_STATIC_DRAW );
	Geo.indexCount = size_cast<GLsizei>( Indexes.GetSize() );
	Geo.mIndexType = Opengl::GetTypeEnum<GLshort>();
	Opengl_IsOkay();

	
	//	unbind
	//	gr: don't unbind, leave bound for life of VAO (maybe for GL 3 only)
	//glBindBuffer( GL_ARRAY_BUFFER, 0 );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
	Opengl_IsOkay();
	

	
	return Geo;
}


void Opengl::TGeometry::Draw() const
{
	if ( !Opengl_IsInitialised() )
		return;
	
	//	null to draw from indexes in vertex array
	glBindVertexArray( vertexArrayObject );
	Opengl_IsOkay();
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	
#if !defined(TARGET_IOS)
	//	should be left enabled
	mVertexDescription.EnableAttribs();
	Opengl_IsOkay();
#endif
	
	//	gr: shouldn't use this, only for debug
	//glDrawArrays( GL_TRIANGLES, 0, this->vertexCount );
	//Opengl_IsOkay();
	
	
	glDrawElements( GL_TRIANGLES, indexCount, mIndexType, nullptr );
	Opengl::IsOkay("glDrawElements");

	//	unbinding so nothing alters it
#if defined(TARGET_IOS)
	glBindVertexArray( 0 );
#endif
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



const std::map<SoyPixelsFormat::Type,GLenum>& Opengl::GetPixelFormatMap()
{
	static std::map<SoyPixelsFormat::Type,GLenum> PixelFormatMap =
	{
		std::make_pair( SoyPixelsFormat::RGB, GL_RGB ),
		std::make_pair( SoyPixelsFormat::RGBA, GL_RGBA ),

#if defined(TARGET_IOS)
		std::make_pair( SoyPixelsFormat::BGRA, GL_BGRA ),
#endif
#if defined(TARGET_WINDOWS)
		std::make_pair( SoyPixelsFormat::Greyscale, GL_LUMINANCE ),
#endif
#if defined(GL_VERSION_3_0)
		std::make_pair( SoyPixelsFormat::BGR, GL_BGR ),
		std::make_pair( SoyPixelsFormat::Greyscale, GL_RED ),
		std::make_pair( SoyPixelsFormat::GreyscaleAlpha, GL_RG ),
		std::make_pair( SoyPixelsFormat::BGRA, GL_BGRA ),
#endif
	};
	return PixelFormatMap;
}

GLenum Opengl::GetPixelFormat(SoyPixelsFormat::Type Format)
{
	auto& Map = GetPixelFormatMap();
	auto it = Map.find( Format );
	if ( it == Map.end() )
		return GL_INVALID_VALUE;
	return it->second;
}


//	gr: seperate function until I've figured all these out
GLenum Opengl::GetUploadPixelFormat(const Opengl::TTexture& Texture,SoyPixelsFormat::Type Format)
{
	//	ios requires formats to match, no internal conversion
#if defined(TARGET_IOS)
	Soy::Assert( Texture.GetFormat() == Format, "IOS requires texture upload format to match" );
	return GetPixelFormat( Texture.GetFormat() );
#endif
	
	//	let callee do conversion
	return GetPixelFormat( Format );
}


SoyPixelsFormat::Type Opengl::GetPixelFormat(GLenum glFormat)
{
	//	gr: there was special code here, check it before removing
	switch ( glFormat )
	{
		//	gr: osx returns these values hmmmm
		case GL_RGBA8:	return SoyPixelsFormat::RGBA;
		case GL_RGB8:	return SoyPixelsFormat::RGB;
		case GL_RGB:	return SoyPixelsFormat::RGB;
		case GL_RGBA:	return SoyPixelsFormat::RGBA;
#if !defined(TARGET_ANDROID)
		case GL_BGRA:	return SoyPixelsFormat::BGRA;
#endif
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
