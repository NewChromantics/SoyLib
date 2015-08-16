#include "SoyOpengl.h"
#include "RemoteArray.h"
#include "SoyOpenglContext.h"
#include "SoyShader.h"
#include <regex>

class TPixelFormatMapping
{
public:
	TPixelFormatMapping() :
	mPixelFormat		( SoyPixelsFormat::Invalid ),
		mDownloadFormat		( GL_INVALID_ENUM ),
		mUploadFormat		( GL_INVALID_ENUM ),
		mNewTextureFormat	( GL_INVALID_ENUM )
	{
	}
	TPixelFormatMapping(SoyPixelsFormat::Type PixelFormat,GLenum DownloadFormat,GLenum UploadFormat,GLenum NewTextureFormat) :
		mPixelFormat		( PixelFormat ),
		mDownloadFormat		( DownloadFormat ),
		mUploadFormat		( UploadFormat ),
		mNewTextureFormat	( NewTextureFormat )
	{
	}
	
	bool					operator==(const SoyPixelsFormat::Type Format) const	{	return mPixelFormat == Format;	}
	
	SoyPixelsFormat::Type	mPixelFormat;
	GLenum					mDownloadFormat;
	GLenum					mUploadFormat;
	GLenum					mNewTextureFormat;
};

namespace Opengl
{
	const Array<TPixelFormatMapping>&	GetPixelFormatMap();
	TPixelFormatMapping					GetPixelMapping(SoyPixelsFormat::Type Format);
	
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

	//	get some CPU-persistent-storage for a texture
	//	todo: some way to clean this up. We WOULD keep it on a texture... IF that texture was persistent (ie. not from unity)
	std::shared_ptr<SoyPixelsImpl>	GetClientStorage(TTexture& Texture);
}

std::shared_ptr<SoyPixelsImpl> Opengl::GetClientStorage(TTexture& Texture)
{
	if ( !Texture.IsValid() )
		return nullptr;
	
	static bool AllowOnTextureStorage = true;
	if ( AllowOnTextureStorage )
	{
		//	can store it on the texture, if we own the texture
		if ( Texture.mAutoRelease )
		{
			if ( !Texture.mClientBuffer )
				Texture.mClientBuffer.reset( new SoyPixels );
			return Texture.mClientBuffer;
		}
	}
	
	//	look in a map
	//	gr: assume texture ID is persistent... may need to change if we do multiple contexts, and caller can check if the data is not appropriate (if it'll re-alloc, maybe we can return a const storage?)
	//	gr: does map need locking?
	static std::map<GLuint,std::shared_ptr<SoyPixelsImpl>> gTextureClientStorage;
	
	std::shared_ptr<SoyPixelsImpl>& Storage = gTextureClientStorage[Texture.mTexture.mName];
	if ( !Storage )
		Storage.reset( new SoyPixels );
	
	return Storage;
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
			CASE_ENUM_STRING( GL_RGBA8 );
			CASE_ENUM_STRING( GL_RED );

			CASE_ENUM_STRING( GL_R8 );
			CASE_ENUM_STRING( GL_RG8 );

#if defined(OPENGL_CORE_3)
			CASE_ENUM_STRING( GL_TEXTURE_1D );
#endif
			CASE_ENUM_STRING( GL_TEXTURE_2D );
			CASE_ENUM_STRING( GL_TEXTURE_3D );
#if defined(OPENGL_CORE_3)
			CASE_ENUM_STRING( GL_TEXTURE_RECTANGLE );
#endif
#if defined(TARGET_ANDROID)
			CASE_ENUM_STRING( GL_TEXTURE_EXTERNAL_OES );
			CASE_ENUM_STRING( GL_SAMPLER_EXTERNAL_OES );
#endif
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

template<>
void Opengl::SetUniform(const TUniform& Uniform,const float& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform1fv( Uniform.mIndex, ArraySize, &Value );
	Opengl_IsOkay();
}

std::string ParseLog(const std::string& ErrorLog,const ArrayBridge<std::string>& SrcLines)
{
	static bool EnableParsing = false;
	if ( !EnableParsing )
		return ErrorLog;
	
	std::stringstream NewLog;

	std::string LogHaystack = ErrorLog;
	
	//	todo: extract everything up to the first line with a regex
	auto FirstMatchIndex = LogHaystack.find("ERROR");
	if ( FirstMatchIndex != std::string::npos && FirstMatchIndex > 0 )
	{
		std::string Intro = LogHaystack.substr( 0, FirstMatchIndex );
		NewLog << Intro << std::endl;
		LogHaystack.erase( 0, FirstMatchIndex );
	}
	
	std::smatch Match;
	while ( std::regex_search( LogHaystack, Match, std::regex("(ERROR): ([0-9]+):([0-9]+): (.*)[\n$]") ) )
	{
		//	(wholestring)(key)(=)
		std::string LogLevel = Match[1].str();
		std::string CharacterIndex = Match[2].str();
		std::string LineIndexStr = Match[3].str();
		std::string Message = Match[4].str();
		LogHaystack = Match.suffix().str();
		
		int LineIndex = 0;
		Soy::StringToType( LineIndex, LineIndexStr );
		
		NewLog << LogLevel << ": " << Message << std::endl;
		//	grab line
		NewLog << "Line " << LineIndex << "# " << SrcLines[LineIndex] << std::endl;
		NewLog << std::endl;
	}

	if ( !LogHaystack.empty() )
	{
		NewLog << LogHaystack << std::endl;
	}
	
	return NewLog.str();
}

void CompileShader(const Opengl::TAsset& Shader,ArrayBridge<std::string>&& SrcLines,const std::string& ErrorPrefix)
{
	//	turn into explicit lines to aid with parsing erros later
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
		std::stringstream Error;
		Error << ErrorPrefix << " Compiling shader error: ";
		GLchar msg[4096] = {0};
		glGetShaderInfoLog( Shader.mName, sizeof( msg ), 0, msg );
		std::string ErrorLog( msg );
		
		//	re-process log in case we can get some extended info out of it
		ErrorLog = ParseLog( ErrorLog, SrcLines );
		
		Error << ErrorLog;
		for ( int i=0;	i<SrcLines.GetSize();	i++ )
		{
			Error << SrcLines[i];
		}
		throw Soy::AssertException( Error.str() );
	}
}


bool Opengl::IsOkay(const char* Context,bool ThrowException)
{
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


Opengl::TFbo::TFbo(TTexture Texture) :
	mFbo		( GL_ASSET_INVALID ),
	mTarget		( Texture )
{
	//	todo: if invalid texture, make one!
	Soy::Assert( Texture.IsValid(), "invalid texture" );
	
	auto& mFboTextureName = mTarget.mTexture.mName;
	auto& mType = mTarget.mType;
	//auto& mFboMeta = mTarget.mMeta;

	//	gr: added to try and get IOS working
#if defined(TARGET_IOS)
	//	remove other mip levels
	glBindTexture( mType, mFboTextureName );
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, 0);
	glBindTexture( mType, 0 );
#endif
	
	//std::Debug << "Creating FBO " << mFboMeta << ", texture name: " << mFboTextureName << std::endl;
	
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
	
	CheckStatus();
	Unbind();
}

Opengl::TFbo::~TFbo()
{
	Delete();
}

void Opengl::TFbo::Delete(Opengl::TContext &Context)
{

	if ( !mFbo.IsValid() )
		return;

	GLuint FboName = mFbo.mName;
	auto DefferedDelete = [FboName]
	{
		glDeleteFramebuffers( 1, &FboName );
		Opengl::IsOkay("Deffered FBO delete");
		return true;
	};

	Context.PushJob( DefferedDelete );

	//	dont-auto delete later
	mFbo.mName = GL_ASSET_INVALID;
}

void Opengl::TFbo::Delete()
{
	if ( mFbo.mName != GL_ASSET_INVALID )
	{
		glDeleteFramebuffers( 1, &mFbo.mName );
		Opengl::IsOkay("glDeleteFramebuffers", false);
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
	Opengl_IsOkayFlush();

	bool IsFrameBuffer = glIsFramebuffer(mFbo.mName);
	Opengl_IsOkay();
	if ( !Soy::Assert( IsFrameBuffer, "Frame buffer no longer valid" ) )
		return false;

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

void Opengl::TFbo::CheckStatus()
{
	auto FrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	Opengl::IsOkay("glCheckFramebufferStatus");
	Soy::Assert(FrameBufferStatus == GL_FRAMEBUFFER_COMPLETE, "DIdn't complete framebuffer setup");

	//	check target is still valid
	Soy::Assert(mTarget.IsValid(), "FBO texture invalid");
}

void Opengl::TFbo::Unbind()
{
	CheckStatus();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	Opengl_IsOkay();
}

size_t Opengl::TFbo::GetAlphaBits() const
{
	GLint AlphaSize = -1;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &AlphaSize );
	Opengl::IsOkay("Get FBO GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE");
	std::Debug << "FBO has " << AlphaSize << " alpha bits" << std::endl;

	return AlphaSize < 0 ? 0 :AlphaSize;
}

Opengl::TTexture::TTexture(SoyPixelsMeta Meta,GLenum Type) :
	mAutoRelease	( true ),
	mType			( Type )
{
	Soy::Assert( Meta.IsValid(), "Cannot setup texture with invalid meta" );
	
	bool AllowOpenglConversion = true;
	GLuint PixelsFormat = Opengl::GetUploadPixelFormat(*this, Meta.GetFormat(), AllowOpenglConversion );
	GLuint InternalPixelFormat = Opengl::GetNewTexturePixelFormat( Meta.GetFormat() );
	if ( PixelsFormat == GL_INVALID_ENUM || InternalPixelFormat == GL_INVALID_ENUM )
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
	GLint MipLevel = 0;
	GLenum GlPixelsStorage = GL_UNSIGNED_BYTE;
	GLint Border = 0;
	
	//	gr: change this to glGenerateMipMaps etc
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, MipLevel );
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, MipLevel );
	Opengl::IsOkay("glTexParameteri set mip levels");
	
	//	debug construction
	//std::Debug << "glTexImage2D(" << Opengl::GetEnumString( mType ) << "," << Opengl::GetEnumString( InternalPixelFormat ) << "," << Opengl::GetEnumString( PixelsFormat ) << "," << Opengl::GetEnumString(GlPixelsStorage) << ")" << std::endl;
	
	//	initialise to set dimensions
	static bool InitialiseNull = true;
	if ( InitialiseNull )
	{
		glTexImage2D( mType, MipLevel, InternalPixelFormat, Meta.GetWidth(), Meta.GetHeight(), Border, PixelsFormat, GlPixelsStorage, nullptr );
	}
	else
	{
		SoyPixels InitFramePixels;
		InitFramePixels.Init( Meta.GetWidth(), Meta.GetHeight(), Meta.GetFormat() );
		auto& PixelsArray = InitFramePixels.GetPixelsArray();
		glTexImage2D( mType, MipLevel, InternalPixelFormat, Meta.GetWidth(), Meta.GetHeight(), Border, PixelsFormat, GlPixelsStorage, PixelsArray.GetArray() );
	}
	Opengl::IsOkay("glTexImage2D texture construction");
	
	//	verify params
#if !defined(OPENGL_ES_3)
	GLint TextureWidth = -1;
	GLint TextureHeight = -1;
	GLint TextureInternalFormat = -1;
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()" );
	Soy::Assert( TextureWidth==Meta.GetWidth(), "Texture width doesn't match initialisation");
	Soy::Assert( TextureHeight==Meta.GetHeight(), "Texture height doesn't match initialisation");

	//	gr: internal format comes out as RGBA8 even if we specify RGBA. Need to have some "is same format" func
	//Soy::Assert( TextureInternalFormat==InternalPixelFormat, "Texture format doesn't match initialisation");
	//std::Debug << "new texture format "  << Opengl::GetEnumString(TextureInternalFormat) << std::endl;
#endif
	
	Unbind();
	
	//	built, save meta
	mMeta = Meta;
}

bool Opengl::TTexture::IsValid() const
{
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

bool Opengl::TTexture::Bind() const
{
	Opengl::IsOkay("Texture bind flush",false);
	glBindTexture( mType, mTexture.mName );
	return Opengl_IsOkay();
}

void Opengl::TTexture::Unbind() const
{
	glBindTexture( mType, GL_ASSET_INVALID );
}


void Opengl::TTexture::Read(SoyPixelsImpl& Pixels) const
{
	Soy::Assert( IsValid(), "Trying to read from invalid texture" );

	//	not currently supported directly in opengl ES (need to make a pixel buffer, copy to it and read, I think)
#if defined(TARGET_ANDROID) || defined(TARGET_IOS)
	throw Soy::AssertException( std::string(__func__) + " not supported on opengl es yet");
	return;
#else
	
	Bind();
	
	//	resolve GL & soy pixel formats
	SoyPixelsFormat::Type PixelFormat = GetFormat();
	GLenum GlFormat = Opengl::GetDownloadPixelFormat( *this, PixelFormat );
	if ( GlFormat == GL_INVALID_ENUM || PixelFormat == SoyPixelsFormat::Invalid )
	{
		std::stringstream Error;
		Error << "Failed to resolve pixel(" << PixelFormat << ") and opengl(" << GetEnumString(GlFormat) << ") for downloading texture (" << mMeta << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	//	pre-alloc data
	if ( !Pixels.Init( GetWidth(), GetHeight(), PixelFormat ) )
		throw Soy::AssertException("Failed to allocate pixels for texture read");
	
	//
	GLint MipLevel = 0;
	GLenum PixelStorage = GL_UNSIGNED_BYTE;
	auto* PixelBytes = Pixels.GetPixelsArray().GetArray();
	glGetTexImage( mType, MipLevel, GlFormat, PixelStorage, PixelBytes );
	Opengl_IsOkay();
	
	static int DebugPixelCount_ = 0;
	if ( DebugPixelCount_ > 0 )
	{
		//auto DebugPixelCount = std::min<size_t>( DebugPixelCount_, Pixels.GetPixelsArray().GetDataSize() );
		Pixels.PrintPixels("Read pixels from texture: ", std::Debug, false, " " );
	}
	
	Unbind();
	Opengl_IsOkay();
#endif
}



void Opengl::TTexture::Copy(const SoyPixelsImpl& SourcePixels,Opengl::TTextureUploadParams Params)
{
	Soy::Assert( IsValid(), "Trying to upload to invalid texture ");
	
	Bind();
	Opengl::IsOkay( std::string(__func__) + " Bind()" );

	int MipLevel = 0;
	
	//	grab the texture's width & height so we can clip, if we try and copy pixels bigger than the texture we'll get an error
	GLint TextureWidth = 0;
	GLint TextureHeight = 0;
	GLint TextureInternalFormat = 0;
	
	//	opengl es doesn't have access!
#if defined(OPENGL_ES_3)
	TextureWidth = this->GetWidth();
	TextureHeight = this->GetHeight();
	//	gr: errr what should this be now?
	TextureInternalFormat = GetNewTexturePixelFormat( this->GetFormat() );
#else
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()" );
#endif
	
	const SoyPixelsImpl* UsePixels = &SourcePixels;

	
	//	we CANNOT go from big to small, only small to big, so to avoid corrupted textures, lets shrink
	SoyPixels Resized;
	static bool AllowClipResize = true;
	if ( AllowClipResize )
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

	bool IsSameDimensions = (UsePixels->GetWidth()==TextureWidth) && (UsePixels->GetHeight()==TextureHeight);
	bool SubImage = !(Params.mStretch);
	bool UsingAppleStorage = (!SubImage || IsSameDimensions) && Opengl::TContext::IsSupported( OpenglExtensions::AppleClientStorage, nullptr );

	if ( UsingAppleStorage )
	{
		//	https://developer.apple.com/library/mac/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_texturedata/opengl_texturedata.html
		//	. Note that a texture width must be a multiple of 32 bytes for OpenGL to bypass the copy operation from the application to the OpenGL framework.
		if ( !Params.mAllowClientStorage )
			UsingAppleStorage = false;
	}

	//	fetch our client storage
	std::shared_ptr<SoyPixelsImpl> ClientStorage;
	if ( UsingAppleStorage )
	{
		ClientStorage = Opengl::GetClientStorage( *this );
		if ( !ClientStorage )
			UsingAppleStorage = false;
	}
	
	if ( UsingAppleStorage )
		Params.mAllowOpenglConversion = false;
	
	//	pixel data format
	auto GlPixelsFormat = Opengl::GetUploadPixelFormat( *this, SourcePixels.GetFormat(), Params.mAllowOpenglConversion );


	//	gr: take IOS's target-must-match-source requirement into consideration here (replace GetUploadPixelFormat)
	SoyPixels ConvertedPixels;
	if ( Params.mAllowCpuConversion )
	{
		//	convert to upload-compatible type
		Array<SoyPixelsFormat::Type> TryFormats;
		TryFormats.PushBack( SoyPixelsFormat::RGB );
		TryFormats.PushBack( SoyPixelsFormat::RGBA );
		TryFormats.PushBack( SoyPixelsFormat::Greyscale );
		
		while ( GlPixelsFormat == GL_INVALID_VALUE && !TryFormats.IsEmpty() )
		{
			auto TryFormat = TryFormats.PopAt(0);
			auto TryGlFormat = Opengl::GetUploadPixelFormat( *this, TryFormat, Params.mAllowOpenglConversion );
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
	
	if (GlPixelsFormat == GL_INVALID_ENUM)
	{
		std::stringstream Error;
		Error << "Failed to write texture, unsupported upload format " << SourcePixels.GetFormat();
		throw Soy::AssertException(Error.str());
	}
	
	auto& FinalPixels = *UsePixels;
	GLenum GlPixelsStorage = GL_UNSIGNED_BYTE;


#if defined(TARGET_OSX)
	if ( UsingAppleStorage )
	{
		ofScopeTimerWarning Timer("glTexImage2D(GL_APPLE_client_storage)", 4 );
		
		//	https://developer.apple.com/library/mac/documentation/graphicsimaging/conceptual/opengl-macprogguide/opengl_texturedata/opengl_texturedata.html
		glTexParameteri(mType,
						GL_TEXTURE_STORAGE_HINT_APPLE,
						GL_STORAGE_SHARED_APPLE);
			
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
		
		//	make sure there is never a reallocation, unless it's the first
		auto& PixelsBuffer = *ClientStorage;
		bool NewTexture = !PixelsBuffer.IsValid();
		PixelsBuffer.Copy( FinalPixels, NewTexture );
		
		//	need to work out if it's new to the GPU in case pixels were already allocated
		//if ( NewTexture )
		{
			//GLuint InternalPixelFormat = Opengl::GetNewTexturePixelFormat( Meta.GetFormat() );
			glTexImage2D(mType, MipLevel, TextureInternalFormat, PixelsBuffer.GetWidth(), PixelsBuffer.GetHeight(), 0, GlPixelsFormat, GlPixelsStorage, PixelsBuffer.GetPixelsArray().GetArray() );
			Opengl::IsOkay("glTexImage2D(GL_APPLE_client_storage) glTexImage2D");
		}
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
		Opengl::IsOkay("glTexImage2D(GL_APPLE_client_storage) glPixelStorei");
	}
	else
#endif
	if ( !SubImage )	//	gr: if we find glTexImage2D faster add || IsSameDimensions
	{
		int Border = 0;
		
		//	if texture doesnt fit we'll get GL_INVALID_VALUE
		//	if frame is bigger than texture, it will mangle (bad stride)
		//	if pixels is smaller, we'll just get the sub-image drawn
		auto Width = std::min<GLsizei>( TextureWidth, FinalPixels.GetWidth() );
		auto Height = std::min<GLsizei>( TextureHeight, FinalPixels.GetHeight() );
		
		const ArrayInterface<uint8>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();
	
		//	only for "new" textures
		
		GLuint TargetFormat = GL_RGBA;
	
		ofScopeTimerWarning Timer("glTexImage2D", 10 );
		glTexImage2D( mType, MipLevel, TargetFormat,  Width, Height, Border, GlPixelsFormat, GlPixelsStorage, PixelsArrayData );
		Opengl_IsOkay();
	}
	else
	{
		int XOffset = 0;
		int YOffset = 0;
		
		auto Width = std::min<GLsizei>( TextureWidth, FinalPixels.GetWidth() );
		auto Height = std::min<GLsizei>( TextureHeight, FinalPixels.GetHeight() );

		const ArrayInterface<uint8>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();
		
		//	invalid operation here means the unity pixel format is probably different to the pixel data we're trying to push now
		ofScopeTimerWarning Timer("glTexSubImage2D", 10 );
		glTexSubImage2D( mType, MipLevel, XOffset, YOffset, Width, Height, GlPixelsFormat, GlPixelsStorage, PixelsArrayData );
		Timer.Stop();
		
		std::stringstream Context;
		Context << __func__ << "glTexSubImage2D(" << Opengl::GetEnumString(GlPixelsFormat) << ")";
		
		if ( !Opengl::IsOkay( Context.str(),false) )
		{
			//	on ios the internal format must match the pixel format. No conversion!
			GLenum TargetFormat = GlPixelsFormat;
			int Border = 0;
			
			//	gr: first copy needs to initialise the texture... before we can use subimage
			ofScopeTimerWarning Timer2("glTexImage2D fallback", 10 );
			glTexImage2D( GL_TEXTURE_2D, MipLevel, TargetFormat, Width, Height, Border, GlPixelsFormat, GL_UNSIGNED_BYTE, PixelsArrayData );
			Opengl::IsOkay("glTexImage2D",false);
		}
	}

	//	gr: crashes often on OSX... only on NPOT textures?
	//glGenerateMipmap( mType );
	Opengl_IsOkay();
	
	Unbind();
	Opengl_IsOkay();
}

SoyPixelsMeta Opengl::TTexture::GetInternalMeta(GLenum& RealType)
{
#if defined(OPENGL_ES_3)
	std::Debug << "Warning, using " << __func__ << " on opengl es (no such info)" << std::endl;
	return mMeta;
#else
	
	//	try all types
	GLenum Types_[] =
	{
		mType,
		GL_TEXTURE_2D,
#if defined(GL_TEXTURE_RECTANGLE)
		GL_TEXTURE_RECTANGLE,
#endif
#if defined(GL_TEXTURE_EXTERNAL_OES)
		GL_TEXTURE_EXTERNAL_OES,
#endif
		GL_TEXTURE_1D,
		GL_TEXTURE_3D,
		GL_TEXTURE_CUBE_MAP,
	};
	
	auto Types = GetRemoteArray(Types_);
	for ( int t=0;	t<Types.GetSize();	t++ )
	{
		auto Type = Types[t];
		if ( Type == GL_INVALID_VALUE )
			continue;
	
		Opengl::TTexture TempTexture( mTexture.mName, mMeta, Type );
		if ( !TempTexture.Bind() )
			continue;
		
		GLint MipLevel = 0;
		GLint Width = 0;
		GLint Height = 0;
		GLint Format = 0;
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_WIDTH, &Width);
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_HEIGHT, &Height);
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &Format );
		
		TempTexture.Unbind();
		
		if ( !Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()", false ) )
			continue;
		
		//	we probably won't get an opengl error, but the values won't be good. We can assume it's not that type
		//	tested on osx
		if ( Width==0 || Height==0 || Format==0 )
			continue;
		
		if ( Type != mType )
			std::Debug << "Determined that texture is " << GetEnumString(Type) << " not " << GetEnumString(mType) << std::endl;
		
		RealType = Type;
		auto PixelFormat = GetDownloadPixelFormat( Format );
		return SoyPixelsMeta( Width, Height, PixelFormat );
	}
	
	//	couldn't determine any params... not a known texture type?
	RealType = GL_INVALID_VALUE;
	return SoyPixelsMeta();
#endif
}




Opengl::TShaderState::TShaderState(const Opengl::TShader& Shader) :
	mTextureBindCount	( 0 ),
	mShader				( Shader )
{
	glUseProgram( Shader.mProgram.mName );
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



bool Opengl::TShaderState::SetUniform(const char* Name,const float& v)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;
	Opengl::SetUniform( Uniform, v );
	return true;
}


bool Opengl::TShaderState::SetUniform(const char* Name,const vec4f& v)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;
	Opengl::SetUniform( Uniform, v );
	return true;
}


bool Opengl::TShaderState::SetUniform(const char* Name,const vec2f& v)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;
	Opengl::SetUniform( Uniform, v );
	return true;
}

bool Opengl::TShaderState::SetUniform(const char* Name,const TTexture& Texture)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;

	auto BindTextureIndex = size_cast<GLint>( mTextureBindCount );
	BindTexture( BindTextureIndex, Texture );
	glUniform1i( Uniform.mIndex, BindTextureIndex );
	Opengl_IsOkay();
	mTextureBindCount++;
	return true;
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

Opengl::TShader::TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const TGeometryVertex& Vertex,const std::string& ShaderName,Opengl::TContext& Context)
{
	std::string ShaderNameVert = ShaderName + " (vert)";
	std::string ShaderNameFrag = ShaderName + " (frag)";
	
	Array<std::string> VertShader;
	Array<std::string> FragShader;
	Soy::SplitStringLines( GetArrayBridge(VertShader), vertexSrc );
	Soy::SplitStringLines( GetArrayBridge(FragShader), fragmentSrc );
	
	OpenglShaderVersion::Type UpgradeVersion = OpenglShaderVersion::Invalid;
	
	//	not required for android
#if defined(OPENGL_ES_3)
	if ( Context.mVersion.mMajor >= 3 )
		UpgradeVersion = OpenglShaderVersion::glsl300;
#endif
	
#if defined(OPENGL_CORE_3)
	if ( Context.mVersion.mMajor >= 3 )
		UpgradeVersion = OpenglShaderVersion::glsl150;
#endif
	
	if ( UpgradeVersion != OpenglShaderVersion::Invalid )
	{
		SoyShader::Opengl::UpgradeVertShader( GetArrayBridge(VertShader), UpgradeVersion );
		SoyShader::Opengl::UpgradeFragShader( GetArrayBridge(FragShader), UpgradeVersion );
	}
	
	mVertexShader.mName = glCreateShader( GL_VERTEX_SHADER );
	CompileShader( mVertexShader, GetArrayBridge(VertShader), ShaderNameVert );

	mFragmentShader.mName = glCreateShader( GL_FRAGMENT_SHADER );
	CompileShader( mFragmentShader, GetArrayBridge(FragShader), ShaderNameFrag );
	
	mProgram.mName = glCreateProgram();
	auto& ProgramName = mProgram.mName;
	glAttachShader( ProgramName, mVertexShader.mName );
	glAttachShader( ProgramName, mFragmentShader.mName );
	
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
	}
	
	//	gr: validate only if we have VAO bound
	bool DoValidate = false;
	if ( DoValidate )
	{
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
			//return TShader();
		}
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
		
		//	todo: check is valid type etc
		mAttributes.PushBack( Uniform );
	}
	
	for( GLint attrib=0;	attrib<numActiveUniforms;	++attrib )
	{
		std::vector<GLchar> nameData( MaxNameLength );
		
		TUniform Uniform;
		Uniform.mIndex = attrib;
		
		GLsizei actualLength = 0;
		glGetActiveUniform( ProgramName, attrib, size_cast<GLsizei>(nameData.size()), &actualLength, &Uniform.mArraySize, &Uniform.mType, &nameData[0]);
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	todo: check is valid type etc
		mUniforms.PushBack( Uniform );
	}

	std::Debug << ShaderName << " has " << mAttributes.GetSize() << " attributes; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(mAttributes), "\n" );
	std::Debug << std::endl;

	std::Debug << ShaderName << " has " << mUniforms.GetSize() << " uniforms; " << std::endl;
	std::Debug << Soy::StringJoin( GetArrayBridge(mUniforms), "\n" );
	std::Debug << std::endl;
}


Opengl::TShaderState Opengl::TShader::Bind()
{
	Opengl_IsOkay();
	glUseProgram( mProgram.mName );
	Opengl_IsOkay();
	
	TShaderState ShaderState( *this );
	return ShaderState;
}

Opengl::TShader::~TShader()
{
	if ( mProgram.mName != GL_ASSET_INVALID )
	{
		glDeleteProgram( mProgram.mName );
		mProgram.mName = GL_ASSET_INVALID;
	}
	if ( mVertexShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( mVertexShader.mName );
		mVertexShader.mName = GL_ASSET_INVALID;
	}
	if ( mFragmentShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( mFragmentShader.mName );
		mFragmentShader.mName = GL_ASSET_INVALID;
	}
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

Opengl::TGeometry::TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<GLshort>&& Indexes,const Opengl::TGeometryVertex& Vertex,TContext& Context) :
	mVertexBuffer( GL_ASSET_INVALID ),
	mIndexBuffer( GL_ASSET_INVALID ),
	mVertexArrayObject( GL_ASSET_INVALID ),
	mVertexCount( GL_ASSET_INVALID ),
	mIndexCount( GL_ASSET_INVALID ),
	mIndexType(GL_ASSET_INVALID),
	mVertexDescription	( Vertex )
{
	Opengl::IsOkay("Opengl::CreateGeometry flush", false);
		
	//	fill vertex array
	Opengl::GenVertexArrays( 1, &mVertexArrayObject );
	Opengl::IsOkay( std::string(__func__) + " glGenVertexArrays" );
	
	glGenBuffers( 1, &mVertexBuffer );
	glGenBuffers( 1, &mIndexBuffer );
	Opengl::IsOkay( std::string(__func__) + " glGenBuffers" );

	Bind();
	glBindBuffer( GL_ARRAY_BUFFER, mVertexBuffer );
	Opengl_IsOkay();
	
	//	gr: buffer data before setting attrib pointer (needed for ios)
	//	push data
	glBufferData( GL_ARRAY_BUFFER, Data.GetDataSize(), Data.GetArray(), GL_STATIC_DRAW );
	mVertexCount = size_cast<GLsizei>( Data.GetDataSize() / Vertex.GetDataSize() );
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
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, Indexes.GetDataSize(), Indexes.GetArray(), GL_STATIC_DRAW );
	mIndexCount = size_cast<GLsizei>( Indexes.GetSize() );
	mIndexType = Opengl::GetTypeEnum<GLshort>();
	Opengl_IsOkay();

	//	
	glFinish();

	//	unbind
	//	gr: don't unbind buffers from the VAO, leave bound for life of VAO (maybe for GL 3 only?)
	//glBindBuffer( GL_ARRAY_BUFFER, 0 );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	Unbind();
}

void Opengl::TGeometry::Bind()
{
	Opengl::BindVertexArray( mVertexArrayObject );
	Opengl_IsOkay();
}

void Opengl::TGeometry::Unbind()
{
	//	unbinding so nothing alters it
#if defined(TARGET_IOS)|| defined(TARGET_WINDOWS)
	Opengl::BindVertexArray( 0 );
	Opengl_IsOkay();
#endif
}


void Opengl::TGeometry::Draw()
{
	Bind();

	//	should already be bound, so these should do nothing
	glBindBuffer( GL_ARRAY_BUFFER, mVertexBuffer );
	Opengl_IsOkay();
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
	Opengl_IsOkay();

#if !defined(TARGET_IOS)
	//	should be left enabled
	mVertexDescription.EnableAttribs();
	Opengl_IsOkay();
#endif
	
	//	gr: shouldn't use this, only for debug
	//glDrawArrays( GL_TRIANGLES, 0, this->vertexCount );
	//Opengl_IsOkay();
	
	
	glDrawElements( GL_TRIANGLES, mIndexCount, mIndexType, nullptr );
	Opengl::IsOkay("glDrawElements");

	Unbind();
}

Opengl::TGeometry::~TGeometry()
{
	if ( mVertexArrayObject != GL_ASSET_INVALID )
	{
		glDeleteVertexArrays( 1, &mVertexArrayObject );
		mVertexArrayObject = GL_ASSET_INVALID;
	}
	
	if ( mIndexBuffer != GL_ASSET_INVALID )
	{
		glDeleteBuffers( 1, &mIndexBuffer );
		mIndexBuffer = GL_ASSET_INVALID;
		mIndexCount = 0;
	}
	
	if ( mVertexBuffer != GL_ASSET_INVALID )
	{
		glDeleteBuffers( 1, &mVertexBuffer );
		mVertexBuffer = GL_ASSET_INVALID;
		mVertexCount = 0;
	}
}



const Array<TPixelFormatMapping>& Opengl::GetPixelFormatMap()
{
	static TPixelFormatMapping _PixelFormatMap[] =
	{
		TPixelFormatMapping( SoyPixelsFormat::RGB, GL_RGB, GL_RGB, GL_RGB ),
		TPixelFormatMapping( SoyPixelsFormat::RGBA, GL_RGBA, GL_RGBA, GL_RGBA ),

		//	alternatives for GL -> soy
		TPixelFormatMapping( SoyPixelsFormat::RGB, GL_RGB8, GL_RGB8, GL_RGB8 ),
		TPixelFormatMapping( SoyPixelsFormat::RGBA, GL_RGBA8, GL_RGBA8, GL_RGBA8 ),

#if defined(TARGET_IOS)
		TPixelFormatMapping( SoyPixelsFormat::BGRA, GL_BGRA, GL_BGRA, GL_BGRA ),
		TPixelFormatMapping( SoyPixelsFormat::LumaFull, GL_LUMINANCE, GL_LUMINANCE, GL_LUMINANCE ),
		TPixelFormatMapping( SoyPixelsFormat::LumaVideo, GL_LUMINANCE, GL_LUMINANCE, GL_LUMINANCE ),
		TPixelFormatMapping( SoyPixelsFormat::Greyscale, GL_LUMINANCE, GL_LUMINANCE, GL_LUMINANCE ),
		TPixelFormatMapping( SoyPixelsFormat::GreyscaleAlpha, GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA ),
#endif
#if defined(TARGET_WINDOWS)
		TPixelFormatMapping( SoyPixelsFormat::LumaFull, GL_RED, GL_RED, GL_RED),
		TPixelFormatMapping( SoyPixelsFormat::LumaVideo, GL_RED, GL_RED, GL_RED),
		TPixelFormatMapping(SoyPixelsFormat::Greyscale, GL_RED, GL_RED, GL_RED),
		TPixelFormatMapping(SoyPixelsFormat::GreyscaleAlpha, GL_RG, GL_RG, GL_RG),

#endif
#if defined(GL_VERSION_3_0)
		TPixelFormatMapping( SoyPixelsFormat::BGR, GL_BGR, GL_BGR, GL_BGR ),
		TPixelFormatMapping( SoyPixelsFormat::LumaFull, GL_RED, GL_RED, GL_RED ),
		TPixelFormatMapping( SoyPixelsFormat::LumaVideo, GL_RED, GL_RED, GL_RED ),
		TPixelFormatMapping( SoyPixelsFormat::Greyscale, GL_RED, GL_RED, GL_RED ),
		TPixelFormatMapping( SoyPixelsFormat::GreyscaleAlpha, GL_RG, GL_RG, GL_RG ),
		TPixelFormatMapping( SoyPixelsFormat::BGRA, GL_RGBA, GL_BGRA, GL_RGBA ),
#endif
	};
	static Array<TPixelFormatMapping> PixelFormatMap( _PixelFormatMap );
	return PixelFormatMap;
}


TPixelFormatMapping Opengl::GetPixelMapping(SoyPixelsFormat::Type PixelFormat)
{
	auto* Mapping = GetPixelFormatMap().Find( PixelFormat );
	if ( !Mapping )
		return TPixelFormatMapping();
	return *Mapping;
}

GLenum Opengl::GetUploadPixelFormat(const Opengl::TTexture& Texture,SoyPixelsFormat::Type Format,bool AllowConversion)
{
	if ( Texture.IsValid() )
	{
		//	ios requires formats to match, no internal conversion
	#if defined(TARGET_IOS)
		Soy::Assert( Texture.GetFormat() == Format, "IOS requires texture upload format to match" );
		auto Mapping = GetPixelMapping( Texture.GetFormat() );
		return Mapping.mUploadFormat;
	#endif
	}
	
	auto Mapping = GetPixelMapping( Format );
	return Mapping.mUploadFormat;
}


GLenum Opengl::GetNewTexturePixelFormat(SoyPixelsFormat::Type Format)
{
	auto Mapping = GetPixelMapping( Format );
	return Mapping.mNewTextureFormat;
}


GLenum Opengl::GetDownloadPixelFormat(const TTexture& Texture,SoyPixelsFormat::Type& PixelFormat)
{
	auto Mapping = GetPixelMapping( PixelFormat );
	return Mapping.mNewTextureFormat;
}


SoyPixelsFormat::Type Opengl::GetDownloadPixelFormat(GLenum PixelFormat)
{
	auto& FormatMaps = GetPixelFormatMap();
	for ( int i=0;	i<FormatMaps.GetSize();	i++ )
	{
		auto& FormatMap = FormatMaps[i];
		
		if ( PixelFormat == FormatMap.mDownloadFormat )
			return FormatMap.mPixelFormat;
	}

	std::Debug << "Failed to convert glpixelformat " << PixelFormat << " to soy pixel format" << std::endl;
	return SoyPixelsFormat::Invalid;
}


/*
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
*/

void Opengl::SetViewport(Soy::Rectf Viewport)
{
	auto x = size_cast<GLint>(Viewport.x);
	auto y = size_cast<GLint>(Viewport.y);
	auto w = size_cast<GLsizei>(Viewport.w);
	auto h = size_cast<GLsizei>(Viewport.h);
	glViewport( x, y, w, h );
	Opengl_IsOkay();
}

void Opengl::ClearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Opengl::ClearStencil()
{
	glClear(GL_STENCIL_BUFFER_BIT);
}

void Opengl::ClearColour(Soy::TRgb Colour,float Alpha)
{
	glClearColor( Colour.r(), Colour.g(), Colour.b(), Alpha );
	glClear(GL_COLOR_BUFFER_BIT);
}
