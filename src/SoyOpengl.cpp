#include "SoyOpengl.h"
#include "RemoteArray.h"
#include "SoyOpenglContext.h"
#include "SoyShader.h"
#include <regex>


typedef SoyGraphics::TUniform TUniform;

std::ostream& operator<<(std::ostream& out,const Opengl::TTextureMeta& MetaAndType)
{
	out << MetaAndType.mMeta << '/' << Opengl::GetEnumString( MetaAndType.mType );
	return out;
}

namespace SoyGraphics
{
	TElementType::Type	GetType(GLenum Type);
}

namespace Opengl
{
	GLenum				GetType(SoyGraphics::TElementType::Type Type);
}


class TPixelFormatMapping
{
public:
	TPixelFormatMapping() :
		mPixelFormat		( SoyPixelsFormat::Invalid )
	{
	}
	TPixelFormatMapping(SoyPixelsFormat::Type PixelFormat,const std::initializer_list<GLenum>& OpenglFormats) :
		mPixelFormat		( PixelFormat )
	{
		for ( auto OpenglFormat : OpenglFormats )
			mOpenglFormats.PushBack( OpenglFormat );
	}
	TPixelFormatMapping(SoyPixelsFormat::Type PixelFormat,const std::initializer_list<GLenum>& OpenglFormats,const std::initializer_list<GLenum>& InternalOpenglFormats) :
		mPixelFormat		( PixelFormat )
	{
		for ( auto OpenglFormat : OpenglFormats )
			mOpenglFormats.PushBack( OpenglFormat );
		for ( auto OpenglFormat : InternalOpenglFormats )
			mOpenglInternalFormats.PushBack( OpenglFormat );
	}
	
	bool					operator==(const SoyPixelsFormat::Type Format) const	{	return mPixelFormat == Format;	}
	
	SoyPixelsFormat::Type	mPixelFormat;
	
	//	gr: after exhaustive work.... these have all ended up being the same.
	//	ES requires this, some don't but in the end they've all been the same anyway...
	//	BUT, some platforms (android ES2) don't support all formats, so we want alternatives...
	//	gr: there is ONE case; BGRA can be uploaded, but not an internal format (OSX)
	BufferArray<GLenum,5>	mOpenglFormats;
	BufferArray<GLenum,5>	mOpenglInternalFormats;	//	if empty, use OpenglFormats
};

namespace Opengl
{
	const Array<TPixelFormatMapping>&	GetPixelFormatMap();
	TPixelFormatMapping							GetPixelMapping(SoyPixelsFormat::Type Format);
	
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
		case GL_BYTE:			return Soy::GetTypeName<sint8>();
		case GL_UNSIGNED_BYTE:	return Soy::GetTypeName<uint8>();
		case GL_SHORT:			return Soy::GetTypeName<sint16>();
		case GL_UNSIGNED_SHORT:	return Soy::GetTypeName<uint16>();
		case GL_INT:			return Soy::GetTypeName<int>();
		case GL_UNSIGNED_INT:	return Soy::GetTypeName<uint32>();
		case GL_FLOAT:			return Soy::GetTypeName<float>();
		case GL_FLOAT_VEC2:		return Soy::GetTypeName<vec2f>();
		case GL_FLOAT_VEC3:		return Soy::GetTypeName<vec3f>();
		case GL_FLOAT_VEC4:		return Soy::GetTypeName<vec4f>();
			CASE_ENUM_STRING( GL_INT_VEC2 );
			CASE_ENUM_STRING( GL_INT_VEC3 );
			CASE_ENUM_STRING( GL_INT_VEC4 );
		case GL_BOOL:			return Soy::GetTypeName<bool>();
			CASE_ENUM_STRING( GL_SAMPLER_2D );
			CASE_ENUM_STRING( GL_SAMPLER_CUBE );
			CASE_ENUM_STRING( GL_FLOAT_MAT2 );
			CASE_ENUM_STRING( GL_FLOAT_MAT3 );
			CASE_ENUM_STRING( GL_FLOAT_MAT4 );
#if defined(GL_DOUBLE)
			CASE_ENUM_STRING( GL_DOUBLE );
#endif
#if defined(GL_SAMPLER_1D)
			CASE_ENUM_STRING( GL_SAMPLER_1D );
#endif
#if defined(GL_SAMPLER_3D)
			CASE_ENUM_STRING( GL_SAMPLER_3D );
#endif
			
			//	colours
#if defined(GL_BGRA)
			CASE_ENUM_STRING( GL_BGRA );
#endif
			CASE_ENUM_STRING( GL_RGBA );
			CASE_ENUM_STRING( GL_RGB );
#if defined(GL_RGB8)
			CASE_ENUM_STRING( GL_RGB8 );
#endif
#if defined(GL_RGBA8)
			CASE_ENUM_STRING( GL_RGBA8 );
#endif
#if defined(GL_RED)
			CASE_ENUM_STRING( GL_RED );
#endif

#if defined(GL_R8)
			CASE_ENUM_STRING( GL_R8 );
#endif
#if defined(GL_RG8)
			CASE_ENUM_STRING( GL_RG8 );
#endif
#if defined(GL_RG)
			CASE_ENUM_STRING( GL_RG );
#endif
			CASE_ENUM_STRING( GL_ALPHA );
#if defined(GL_LUMINANCE)
			CASE_ENUM_STRING( GL_LUMINANCE );
#endif
#if defined(GL_LUMINANCE_ALPHA)
			CASE_ENUM_STRING( GL_LUMINANCE_ALPHA );
#endif

#if defined(GL_TEXTURE_1D)
			CASE_ENUM_STRING( GL_TEXTURE_1D );
#endif
			CASE_ENUM_STRING( GL_TEXTURE_2D );
#if defined(GL_TEXTURE_3D)
			CASE_ENUM_STRING( GL_TEXTURE_3D );
#endif
#if defined(GL_TEXTURE_RECTANGLE)
			CASE_ENUM_STRING( GL_TEXTURE_RECTANGLE );
#endif
#if defined(GL_TEXTURE_EXTERNAL_OES)
			CASE_ENUM_STRING( GL_TEXTURE_EXTERNAL_OES );
#endif
#if defined(GL_SAMPLER_EXTERNAL_OES)
			CASE_ENUM_STRING( GL_SAMPLER_EXTERNAL_OES );
#endif
#if defined(GL_SAMPLER_2D_RECT)
			CASE_ENUM_STRING( GL_SAMPLER_2D_RECT );
#endif

	};
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "Unknown GL enum 0x" << std::hex << Type;
	return Unknown.str();
}



SoyGraphics::TElementType::Type SoyGraphics::GetType(GLenum Type)
{
	switch ( Type )
	{
		case GL_INVALID_ENUM:	return SoyGraphics::TElementType::Invalid;
		case GL_INT:			return SoyGraphics::TElementType::Int;
		case GL_FLOAT:			return SoyGraphics::TElementType::Float;
		case GL_FLOAT_VEC2:		return SoyGraphics::TElementType::Float2;
		case GL_FLOAT_VEC3:		return SoyGraphics::TElementType::Float3;
		case GL_FLOAT_VEC4:		return SoyGraphics::TElementType::Float4;
		case GL_FLOAT_MAT3:		return SoyGraphics::TElementType::Float3x3;
		case GL_SAMPLER_2D:		return SoyGraphics::TElementType::Texture2D;

		//	gr: do these need specific element types to convert back?
		//	only used for vertex generation so, no?
#if defined(GL_SAMPLER_EXTERNAL_OES)
		case GL_SAMPLER_EXTERNAL_OES:	return SoyGraphics::TElementType::Texture2D;
#endif
		default:
			break;
	}

	throw Soy::AssertException( std::string("Unknown Glenum->SoyGraphics::TElementType::Type conversion for ") + Opengl::GetEnumString(Type) );
}

GLenum Opengl::GetType(SoyGraphics::TElementType::Type Type)
{
	switch ( Type )
	{
		case SoyGraphics::TElementType::Invalid:	return GL_INVALID_ENUM;
		case SoyGraphics::TElementType::Int:		return GL_INT;
		case SoyGraphics::TElementType::Float:		return GL_FLOAT;
		case SoyGraphics::TElementType::Float2:		return GL_FLOAT_VEC2;
		case SoyGraphics::TElementType::Float3:		return GL_FLOAT_VEC3;
		case SoyGraphics::TElementType::Float4:		return GL_FLOAT_VEC4;
		case SoyGraphics::TElementType::Float3x3:	return GL_FLOAT_MAT3;
		case SoyGraphics::TElementType::Texture2D:	return GL_SAMPLER_2D;
	}

	throw Soy::AssertException( std::string("Unknown SoyGraphics::TElementType::Type->Glenum conversion for ") );
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const float3x3& Value)
{
	GLsizei ArraySize = 1;
	static GLboolean Transpose = false;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniformMatrix3fv( Uniform.mIndex, ArraySize, Transpose, Value.m );
	Opengl_IsOkay();
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
void Opengl::SetUniform(const TUniform& Uniform,const vec3f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform3fv( Uniform.mIndex, ArraySize, &Value.x );
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

template<>
void Opengl::SetUniform(const TUniform& Uniform,const int& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform1iv( Uniform.mIndex, ArraySize, &Value );
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

void Opengl::FlushError(const char* Context)
{
	//	silently flush any errors
	//	may expand in future
	glGetError();
}


bool Opengl::IsOkay(const char* Context,std::function<void(const std::string&)> ExceptionContainer)
{
	auto Error = glGetError();
	if ( Error == GL_NONE )
		return true;
	
	std::stringstream ErrorStr;
	ErrorStr << "Opengl error in " << Context << ": " << Opengl::GetEnumString(Error);
	
	ExceptionContainer( ErrorStr.str() );
	return false;
}


bool Opengl::IsOkay(const char* Context,bool ThrowException)
{
	auto Error = glGetError();
	if ( Error == GL_NONE )
		return true;

	std::stringstream ErrorStr;
	ErrorStr << "Opengl error in " << Context << ": " << Opengl::GetEnumString(Error);
	
	if ( !ThrowException )
	{
		std::Debug << ErrorStr.str() << std::endl;
		return false;
	}
	
	throw Soy::AssertException( ErrorStr.str() );
}


/*
bool Opengl::IsOkay(const char* Context,bool ThrowException)
{
	if ( ThrowException )
	{
		auto Throw = [](const std::string& Error)
		{
			throw Soy::AssertException( Error );
		};
		return IsOkay( Context, Throw );
	}
	else
	{
		auto Print = [](const std::string& Error)
		{
			std::Debug << Error << std::endl;
		};
		return IsOkay( Context, Print );
	}
}
 */


void Opengl::GetReadPixelsFormats(ArrayBridge<GLenum> &&Formats)
{
	Formats.SetSize(5);
	
	Formats[0] = GL_INVALID_VALUE;
	#if defined(OPENGL_ES)
	Formats[1] = GL_ALPHA;
	#else
	Formats[1] = GL_RED;
	#endif
	Formats[2] = GL_INVALID_VALUE;
	Formats[3] = GL_RGB;
	Formats[4] = GL_RGBA;
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
	//	gr: this is ONLY for opengles 3... add a context to check version? remove entirely?
	/*
#if defined(TARGET_IOS)
	//	remove other mip levels
	Opengl_IsOkayFlush();
	glBindTexture( mType, mFboTextureName );
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, 0);
	Opengl::IsOkay("FBO remove texture GL_TEXTURE_BASE_LEVEL");
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, 0);
	Opengl::IsOkay("FBO remove texture GL_TEXTURE_MAX_LEVEL");
	glBindTexture( mType, 0 );
	Opengl::IsOkay("FBO remove texture mip map levels");
#endif
	 */
	
	//std::Debug << "Creating FBO " << mFboMeta << ", texture name: " << mFboTextureName << std::endl;
	Opengl_IsOkayFlush();
	Opengl::GenFramebuffers( 1, &mFbo.mName );
	Opengl::IsOkay("FBO glGenFramebuffers");
	Opengl::BindFramebuffer( GL_FRAMEBUFFER, mFbo.mName );
	Opengl::IsOkay("FBO glBindFramebuffer2");

	GLint MipLevel = 0;
	if ( mType == GL_TEXTURE_2D )
		Opengl::FramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mType, mFboTextureName, MipLevel );
	else
		throw Soy::AssertException("Don't currently support frame buffer texture if not GL_TEXTURE_2D");
	Opengl::IsOkay("FBO glFramebufferTexture2D");
	

	//	this doesn't exist on es2, should fallback to a stub function
	//	Set the list of draw buffers.
	GLenum DrawBufferAttachments[1] = {GL_COLOR_ATTACHMENT0};
	Opengl::DrawBuffers(1, DrawBufferAttachments);
	Opengl::IsOkay("assigning glDrawBuffer attachments");

	CheckStatus();
	Unbind();
}



Opengl::TFbo::~TFbo()
{
	Delete();
}

void Opengl::TFbo::Delete(Opengl::TContext &Context,bool Blocking)
{
	if ( !mFbo.IsValid() )
		return;

	GLuint FboName = mFbo.mName;
	auto DefferedDelete = [FboName]
	{
		Opengl::DeleteFramebuffers( 1, &FboName );
		Opengl::IsOkay("Deffered FBO delete");
	};

	if ( Blocking )
	{
		Soy::TSemaphore Semaphore;
		Context.PushJob( DefferedDelete, Semaphore );
		Semaphore.Wait();
	}
	else
	{
		Context.PushJob( DefferedDelete );
	}
	
	//	dont-auto delete later
	mFbo.mName = GL_ASSET_INVALID;
}

void Opengl::TFbo::Delete()
{
	if ( mFbo.mName != GL_ASSET_INVALID )
	{
		//	gr: this often gives an error that shouldn't occur, try flushing
		Opengl_IsOkayFlush();
		
		Opengl::DeleteFramebuffers( 1, &mFbo.mName );
		Opengl::IsOkay("glDeleteFramebuffers", false);
		mFbo.mName = GL_ASSET_INVALID;
	}
}

void Opengl::TFbo::InvalidateContent()
{
#if (OPENGL_ES==3)
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

	bool IsFrameBuffer = Opengl::IsFramebuffer(mFbo.mName);
	Opengl_IsOkay();
	if ( !Soy::Assert( IsFrameBuffer, "Frame buffer no longer valid" ) )
		return false;

	Opengl::BindFramebuffer(GL_FRAMEBUFFER, mFbo.mName );
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
	auto FrameBufferStatus = Opengl::CheckFramebufferStatus(GL_FRAMEBUFFER);
	Opengl::IsOkay("glCheckFramebufferStatus");
	Soy::Assert(FrameBufferStatus == GL_FRAMEBUFFER_COMPLETE, "DIdn't complete framebuffer setup");

	//	check target is still valid
	Soy::Assert(mTarget.IsValid(), "FBO texture invalid");
}

void Opengl::TFbo::Unbind()
{
	CheckStatus();
	Opengl::BindFramebuffer(GL_FRAMEBUFFER, 0);
	Opengl_IsOkay();
}

size_t Opengl::TFbo::GetAlphaBits() const
{
#if (OPENGL_ES==2)
	throw Soy::AssertException("Framebuffer alpha size query not availible in opengles2");
	return 0;
#else
	GLint AlphaSize = -1;
	Opengl::GetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &AlphaSize );
	Opengl::IsOkay("Get FBO GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE");
	std::Debug << "FBO has " << AlphaSize << " alpha bits" << std::endl;

	return AlphaSize < 0 ? 0 :AlphaSize;
#endif
}

Opengl::TTexture::TTexture(SoyPixelsMeta Meta,GLenum Type) :
	mAutoRelease	( true ),
	mType			( Type )
{
	Opengl_IsOkayFlush();

	Soy::Assert( Meta.IsValid(), "Cannot setup texture with invalid meta" );
	
	bool AllowOpenglConversion = true;
	BufferArray<GLenum,10> ExternalPixelsFormats;
	Opengl::GetUploadPixelFormats( GetArrayBridge(ExternalPixelsFormats), *this, Meta.GetFormat(), AllowOpenglConversion );
	BufferArray<GLenum,10> InternalPixelFormats;
	Opengl::GetNewTexturePixelFormats( GetArrayBridge(InternalPixelFormats), Meta.GetFormat() );
	if ( ExternalPixelsFormats.IsEmpty() || InternalPixelFormats.IsEmpty() )
	{
		std::stringstream Error;
		Error << "Failed to create texture, unsupported format " << Meta.GetFormat();
		throw Soy::AssertException( Error.str() );
	}

	//	allocate texture
	glGenTextures( 1, &mTexture.mName );
	Opengl::IsOkay("glGenTextures");
	Soy::Assert( mTexture.IsValid(), "Failed to allocate texture" );
	
	if ( !Bind() )
		throw Soy::AssertException("failed to bind after texture allocation");
	Opengl::IsOkay("glGenTextures");
	
	//	set mip-map levels to 0..0
	GLint MipLevel = 0;
	GLenum GlPixelsStorage = GL_UNSIGNED_BYTE;
	GLint Border = 0;
	
	//	disable other mip map levels
	//	gr: this fails if building with opengl_es3 in opengles2 mode...
#if (OPENGL_ES!=2)
	//	gr: change this to glGenerateMipMaps etc
	glTexParameteri(mType, GL_TEXTURE_BASE_LEVEL, MipLevel );
	glTexParameteri(mType, GL_TEXTURE_MAX_LEVEL, MipLevel );
	Opengl::IsOkay("glTexParameteri set mip levels", false);
#endif
	
	//	debug construction
	//std::Debug << "glTexImage2D(" << Opengl::GetEnumString( mType ) << "," << Opengl::GetEnumString( InternalPixelFormat ) << "," << Opengl::GetEnumString( PixelsFormat ) << "," << Opengl::GetEnumString(GlPixelsStorage) << ")" << std::endl;
	
	//	initialise to set dimensions
	{
		/*
		SoyPixels InitFramePixels;
		InitFramePixels.Init( Meta.GetWidth(), Meta.GetHeight(), Meta.GetFormat() );
		auto& PixelsArray = InitFramePixels.GetPixelsArray();
		 glTexImage2D( mType, MipLevel, InternalPixelFormat, Meta.GetWidth(), Meta.GetHeight(), Border, PixelsFormat, GlPixelsStorage, PixelsArray.GetArray() );
		 OnWrite();
		 std::stringstream Error;
		 Error << "glTexImage2D texture construction " << Meta << " InternalPixelFormat=" << GetEnumString(InternalPixelFormat) << " PixelsFormat=" << GetEnumString(PixelsFormat) << ", GlPixelsStorage=" << GetEnumString(GlPixelsStorage);
		 Opengl::IsOkay( Error.str() );
		 */
		bool Created = false;
		for ( int e=0;	!Created && e<ExternalPixelsFormats.GetSize();	e++ )
		{
			for ( int i=0;	!Created && i<InternalPixelFormats.GetSize();	i++ )
			{
				try
				{
					auto InternalPixelFormat = InternalPixelFormats[i];
					auto ExternalPixelFormat = ExternalPixelsFormats[e];
					glTexImage2D( mType, MipLevel, InternalPixelFormat, size_cast<GLsizei>(Meta.GetWidth()), size_cast<GLsizei>(Meta.GetHeight()), Border, ExternalPixelFormat, GlPixelsStorage, nullptr );
					std::stringstream Error;
					Error << "glTexImage2D texture construction " << Meta << " InternalPixelFormat=" << GetEnumString(InternalPixelFormat) << " PixelsFormat=" << GetEnumString(ExternalPixelFormat) << ", GlPixelsStorage=" << GetEnumString(GlPixelsStorage);
					Opengl::IsOkay( Error.str() );
					Created = true;
				}
				catch( std::exception& e)
				{
				}
			}
		}
		std::stringstream Error;
		Error << "Failed to create texture with " << Meta;
		Soy::Assert( Created, Error.str() );
	}
	
	//	verify params
	//	gr: this should be using internal meta
#if (OPENGL_CORE==3)
	GLint TextureWidth = -1;
	GLint TextureHeight = -1;
	GLint TextureInternalFormat = -1;
	
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv(GL_TEXTURE_WIDTH)" );
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv(GL_TEXTURE_HEIGHT)" );
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
	Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT)" );
	
	Soy::Assert( TextureWidth==Meta.GetWidth(), "Texture width doesn't match initialisation");
	Soy::Assert( TextureHeight==Meta.GetHeight(), "Texture height doesn't match initialisation");

	//	gr: internal format comes out as RGBA8 even if we specify RGBA. Need to have some "is same format" func
	//Soy::Assert( TextureInternalFormat==InternalPixelFormat, "Texture format doesn't match initialisation");
	//std::Debug << "new texture format "  << Opengl::GetEnumString(TextureInternalFormat) << std::endl;
#endif
	
	Unbind();
	
	//	default to linear
	SetFilter(true);
	
	//	built, save meta
	mMeta = Meta;
}


bool Opengl::TTexture::IsValid(bool InvasiveTest) const
{
#if defined(TARGET_IOS)
	if ( !InvasiveTest )
	{
		throw Soy::AssertException("Because of metal support, IOS currently doesn't allow non-invasive testing until I have something reliable");
	}
#endif

	if ( InvasiveTest )
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
#endif
	}
	
	if ( mTexture.mName != GL_ASSET_INVALID )
		return true;
	
	return false;
}

void Opengl::TTexture::Delete()
{
	if ( mTexture.mName != GL_ASSET_INVALID )
	{
		glDeleteTextures( 1, &mTexture.mName );
		mTexture.mName = GL_ASSET_INVALID;
	}
	
	mWriteSync.reset();
}

bool Opengl::TTexture::Bind() const
{
	Opengl::IsOkay("Texture bind flush",false);
	Soy::Assert( mTexture.IsValid(), "Trying to bind invalid texture" );
	glBindTexture( mType, mTexture.mName );
	return Opengl_IsOkay();
}

void Opengl::TTexture::Unbind() const
{
	glBindTexture( mType, GL_ASSET_INVALID );
	Opengl_IsOkay();
}

void Opengl::TTexture::SetRepeat(bool Repeat)
{
	Bind();
#if defined(GL_TEXTURE_RECTANGLE)
	//	gr: on OSX, using a non-2D/Cubemap texture gives invalid enum
	//	this doesn't work, but doesn't give an error
	//	maybe allow the throw?
	auto Type = (mType == GL_TEXTURE_RECTANGLE) ? GL_TEXTURE_2D : mType;
#else
	auto Type = mType;
#endif
	glTexParameteri( Type, GL_TEXTURE_WRAP_S, Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri( Type, GL_TEXTURE_WRAP_T, Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	Opengl_IsOkay();
	Unbind();
}

void Opengl::TTexture::SetFilter(bool Linear)
{
	Bind();
	auto Type = mType;
	glTexParameteri( Type, GL_TEXTURE_MIN_FILTER, Linear ? GL_LINEAR : GL_NEAREST );
	glTexParameteri( Type, GL_TEXTURE_MAG_FILTER, Linear ? GL_LINEAR : GL_NEAREST );
	Opengl_IsOkay();
	Unbind();
}

void Opengl::TTexture::GenerateMipMaps()
{
	Soy::Assert( IsValid(), "Generate mip maps on invalid texture");
	
	//	no mip maps on external textures [citation needed - extension spec IIRC]
#if defined(GL_TEXTURE_EXTERNAL_OES)
	if ( mType == GL_TEXTURE_EXTERNAL_OES )
		return;
#endif
	
	if ( !Bind() )
		return;
	
	//	gr: this can be slow, highlight it
	Soy::TScopeTimerPrint Timer("glGenerateMipmap",2);
	
	Opengl::GenerateMipmap( mType );
	std::stringstream Error;
	Error << "Texture(" << Opengl::GetEnumString(mType) << " " << mMeta << ")::GenerateMipMaps";
	Opengl::IsOkay( Error.str(), false );
	Unbind();
}




Opengl::TPbo::TPbo(SoyPixelsMeta Meta) :
	mMeta	( Meta ),
	mPbo	( GL_ASSET_INVALID )
{
	glGenBuffers( 1, &mPbo );
	Opengl_IsOkay();
	
	//	can only read back 1, 3 or 4 channels
	auto ChannelCount = Meta.GetChannels();
	if ( ChannelCount != 1 && ChannelCount != 3 && ChannelCount != 4 )
		throw Soy::AssertException("PBO channel count must be 1 3 or 4");

	
#if (OPENGL_ES==2)
	throw Soy::AssertException("read from buffer data not supported on es2? need to test");
#else
	auto DataSize = Meta.GetDataSize();
	Bind();
	glBufferData( GL_PIXEL_PACK_BUFFER, DataSize, nullptr, GL_STREAM_READ);
	Opengl_IsOkay();
	Unbind();
#endif
}

Opengl::TPbo::~TPbo()
{
	if ( mPbo != GL_ASSET_INVALID )
	{
		glDeleteBuffers( 1, &mPbo );
		Opengl_IsOkay();
		mPbo = GL_ASSET_INVALID;
	}
}

size_t Opengl::TPbo::GetDataSize()
{
	return mMeta.GetDataSize();
}

void Opengl::TPbo::Bind()
{
#if (OPENGL_ES==2)
	throw Soy::AssertException("PBO not supported es2?");
#else
	glBindBuffer( GL_PIXEL_PACK_BUFFER, mPbo );
	Opengl_IsOkay();
#endif
}

void Opengl::TPbo::Unbind()
{
#if (OPENGL_ES==2)
	throw Soy::AssertException("PBO not supported es2?");
#else
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	Opengl_IsOkay();
#endif
}

void Opengl::TPbo::ReadPixels()
{
	GLint x = 0;
	GLint y = 0;
	
	BufferArray<GLenum,5> FboFormats;
	GetReadPixelsFormats( GetArrayBridge(FboFormats) );

	auto ChannelCount = mMeta.GetChannels();
	
	Bind();

	glReadPixels( x, y, size_cast<GLsizei>(mMeta.GetWidth()), size_cast<GLsizei>(mMeta.GetHeight()), FboFormats[ChannelCount], GL_UNSIGNED_BYTE, nullptr );
	Opengl_IsOkay();
	
	Unbind();
}

const uint8* Opengl::TPbo::LockBuffer()
{
#if defined(TARGET_IOS) || defined(TARGET_ANDROID)
	//	gr: come back to this... when needed, I think it's supported
	const uint8* Buffer = nullptr;
#else
	auto* Buffer = glMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY );
#endif

	Opengl_IsOkay();
	return reinterpret_cast<const uint8*>( Buffer );
}

void Opengl::TPbo::UnlockBuffer()
{
#if defined(TARGET_IOS) || defined(TARGET_ANDROID)
	throw Soy::AssertException("Lock buffer should not have succeeded on ES");
#else
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	Opengl_IsOkay();
#endif
}

void Opengl::TTexture::Read(SoyPixelsImpl& Pixels,SoyPixelsFormat::Type ForceFormat,bool Flip) const
{
	Soy::Assert( IsValid(), "Trying to read from invalid texture" );
	
	//	not currently supported directly in opengl ES (need to make a pixel buffer, copy to it and read, I think)
#if defined(TARGET_ANDROID) || defined(TARGET_IOS)
	throw Soy::AssertException( std::string(__func__) + " not supported on opengl es yet");
	return;
#else
	
	Bind();
	
	if ( !Pixels.IsValid() )
	{
		if ( ForceFormat == SoyPixelsFormat::Invalid )
		{
			//	todo: work out fastest format!
			static SoyPixelsFormat::Type DefaultFormat = SoyPixelsFormat::RGBA;
			
			//	gr: assuming the fastest is the same as the internal format, but that doesn't allow FBO
			//	gr: changed this default to true for reading gif greyscale/index. rather than forcing format, I want to know what it is internally...
			static bool UseInternalFormatAsDefault = true;
			if ( UseInternalFormatAsDefault )
				ForceFormat = GetFormat();
			else
				ForceFormat = DefaultFormat;
		}
	}
	
	//	allocate pixels
	if ( !Pixels.IsValid() )
	{
		Soy::Assert( ForceFormat != SoyPixelsFormat::Invalid, "Format should be valid here");
		Pixels.Init( GetWidth(), GetHeight(), ForceFormat );
	}

	{
		//	gr: it's important for memory reasons that the sizes are correct.
		//	glGetTexImage will be corrupt if they mismatch (smaller), which is kinda fine, but maybe will crash on some driver?
		//	glReadPixels from fbo will be in the corner.
		GLenum InternalType;
		auto InternalMeta = GetInternalMeta( InternalType );
		if ( Pixels.GetWidth() != InternalMeta.GetWidth() || Pixels.GetHeight() != InternalMeta.GetHeight() )
		{
			std::Debug << __func__ << " warning; meta (" << Pixels.GetMeta() << ") dimensions different to internal " << InternalMeta << " forcing resize of pixels" << std::endl;
		
			Pixels.ResizeClip( InternalMeta.GetWidth(), InternalMeta.GetHeight() );
		}
	}
	
	//	resolve GL & soy pixel formats
	BufferArray<GLenum,10> DownloadFormats;
	{
		Opengl::GetDownloadPixelFormats( GetArrayBridge(DownloadFormats), *this, Pixels.GetFormat() );
		if ( DownloadFormats.IsEmpty() || Pixels.GetFormat() == SoyPixelsFormat::Invalid )
		{
			std::stringstream Error;
			Error << "Failed to resolve pixel(" << Pixels.GetFormat() << ") and opengl(" << "XXX" << ") for downloading texture (" << mMeta << ")";
			throw Soy::AssertException( Error.str() );
		}
	}
	auto* PixelBytes = Pixels.GetPixelsArray().GetArray();

	
	//	try to use PBO's
	static bool Default_UsePbo = false;
	static bool Default_UseFbo = true;

	bool UsePbo = Default_UsePbo;
	bool UseFbo = Default_UseFbo;
	
	if ( UsePbo )
	{
		TFbo FrameBuffer( *this );
		FrameBuffer.Bind();
		TPbo PixelBuffer( mMeta );
		PixelBuffer.ReadPixels();
		auto* pData = PixelBuffer.LockBuffer();
		auto DataSize = std::min( PixelBuffer.GetDataSize(), Pixels.GetPixelsArray().GetDataSize() );
		memcpy( PixelBytes, pData, DataSize );
		PixelBuffer.UnlockBuffer();
		return;
	}
	
	//	glReadPixels only accepts the formats below
	BufferArray<GLenum,5> FboFormats;
	GetReadPixelsFormats( GetArrayBridge(FboFormats) );
	
	//	gr: sometimes the caller wants a specific format, and FBO(glReadPixels) doesn't allow that
	if ( UseFbo )
	{
		auto PrefferedFormat = Pixels.GetFormat();
		if ( PrefferedFormat != GetDownloadPixelFormat(FboFormats[1]) &&
			PrefferedFormat != GetDownloadPixelFormat(FboFormats[3]) &&
			PrefferedFormat != GetDownloadPixelFormat(FboFormats[4]) )
		{
			UseFbo = false;
		}
	}
	
	
	GLint MipLevel = 0;
	GLenum PixelStorage = GL_UNSIGNED_BYTE;


	if ( UseFbo )
	{
		TFbo FrameBuffer( *this );
		GLint x = 0;
		GLint y = 0;
	
		//	gr: this code is forcing us to use the pixels.init we did earlier...
		auto ChannelCount = Pixels.GetChannels();

		FrameBuffer.Bind();
		//	make sure no padding is applied so glGetTexImage & glReadPixels doesn't override tail memory
		glPixelStorei(GL_PACK_ROW_LENGTH,0);	//	gr: not sure this had any effect, but implemented anyway
		Opengl_IsOkay();
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		Opengl_IsOkay();
		glReadPixels( x, y, size_cast<GLsizei>(Pixels.GetWidth()), size_cast<GLsizei>(Pixels.GetHeight()), FboFormats[ChannelCount], GL_UNSIGNED_BYTE, PixelBytes );
		Opengl::IsOkay("glReadPixels");

		//	as glReadPixels forces us to a format, we need to update the meta on the pixels
		auto NewFormat = GetDownloadPixelFormat( FboFormats[ChannelCount] );
		Pixels.GetMeta().DumbSetFormat( NewFormat );
		
		FrameBuffer.Unbind();
		Opengl_IsOkay();
	}
	else
	{
		//	make sure no padding is applied so glGetTexImage & glReadPixels doesn't override tail memory
		glPixelStorei(GL_PACK_ROW_LENGTH,0);	//	gr: not sure this had any effect, but implemented anyway
		Opengl_IsOkay();
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		Opengl_IsOkay();
		
		bool Success = false;
		for ( int i=0;	!Success && i<DownloadFormats.GetSize();	i++ )
		{
			//	4.5 has byte-saftey with glGetTextureImage glGetnTexImage
			auto GlFormat = DownloadFormats[i];
			glGetTexImage( mType, MipLevel, GlFormat, PixelStorage, PixelBytes );
			Success = Opengl::IsOkay("glGetTexImage", false);
		}
		Soy::Assert( Success, "glGetTextImage failed");
	}
	
	static int DebugPixelCount_ = 0;
	if ( DebugPixelCount_ > 0 )
	{
		//auto DebugPixelCount = std::min<size_t>( DebugPixelCount_, Pixels.GetPixelsArray().GetDataSize() );
		auto& DebugStream = std::Debug.LockStream();
		Pixels.PrintPixels("Read pixels from texture: ", DebugStream, false, " " );
		std::Debug.UnlockStream( DebugStream );
	}
	
	Unbind();
	Opengl_IsOkay();
	
	//	textures are stored upside down so flip here. (maybe store as transform meta data?)
	if ( Flip )
	{
		Pixels.Flip();
	}
#endif
}


class TTryCase
{
public:
	GLenum		mInternalFormat;
	GLenum		mExternalFormat;
	SoyTime		mDuration;
	std::string	mError;
};

std::ostream& operator<<(std::ostream &out,const TTryCase& in)
{
	out << Opengl::GetEnumString( in.mInternalFormat ) << "->" << Opengl::GetEnumString( in.mInternalFormat ) << " took " << in.mDuration.GetTime() << "ms. ";
	if ( !in.mError.empty() )
		out << "Error=" << in.mError;
	return out;
}



void TryFunctionWithFormats(ArrayBridge<GLenum>&& InternalTextureFormats,ArrayBridge<GLenum>&& ExternalTextureFormats,const std::string& Context,std::function<void(GLenum,GLenum)> Function)
{
	BufferArray<TTryCase,100> Trys;

	static bool Profile = false;		//	execute all cases and find best (and show errors)
	bool Success = false;
	
	//	minimise cost of lambda
	static TTryCase* CurrentTry = nullptr;
	std::function<void(const std::string&)> ExceptionContainer = [](const std::string& Error)
	{
		CurrentTry->mError = Error;
	};
	
	for ( int i=0;	i<InternalTextureFormats.GetSize();	i++ )
	{
		for ( int e=0;	e<ExternalTextureFormats.GetSize();	e++ )
		{
			TTryCase& Try = Trys.PushBack();
			Try.mInternalFormat = InternalTextureFormats[i];
			Try.mExternalFormat = ExternalTextureFormats[e];

			if ( Profile )
			{
				auto RecordDuration = [&Try](SoyTime Duration)
				{
					Try.mDuration = Duration;
				};
				Soy::TScopeTimer Timer(nullptr,0,RecordDuration,true);
				
				Function( Try.mInternalFormat, Try.mExternalFormat );
			}
			else
			{
				Function( Try.mInternalFormat, Try.mExternalFormat );
			}
			
			CurrentTry = &Try;
			Success |= Opengl::IsOkay("",ExceptionContainer);
			
			//	return immediately if we aren't profiling
			if ( Success && !Profile )
				return;
		}
	}

	//	display profile info
	//	gr: seperated with / for unity output
	//auto LineFeed = std::endl;
	auto LineFeed = '/';
	std::Debug << "Opengl-TryFormats(" << Context << ") tried x" << Trys.GetSize() << LineFeed;
	for ( int i=0;	i<Trys.GetSize();	i++ )
		std::Debug << Trys[i] << LineFeed;
	std::Debug << std::endl;

	if ( !Success )
		throw Soy::AssertException(Context);
}


void Opengl::TTexture::Write(const SoyPixelsImpl& SourcePixels,SoyGraphics::TTextureUploadParams Params)
{
	std::stringstream WholeFunctionContext;
	WholeFunctionContext << __func__ << " (" << SourcePixels.GetMeta() << "->" << this->GetMeta() << ")";
	Soy::TScopeTimerPrint WholeTimer( WholeFunctionContext.str().c_str(), 10 );

	Soy::Assert( IsValid(), "Trying to upload to invalid texture ");
	
	Bind();
	Opengl::IsOkay( std::string(__func__) + " Bind()" );

	int MipLevel = 0;
	
	//	grab the texture's width & height so we can clip, if we try and copy pixels bigger than the texture we'll get an error
	GLint TextureWidth = 0;
	GLint TextureHeight = 0;
	Array<GLenum> TextureInternalFormats;
	
	//	opengl es doesn't have access!
#if defined(OPENGL_ES)
	{
		TextureWidth = size_cast<GLint>( this->GetWidth() );
		TextureHeight = size_cast<GLint>( this->GetHeight() );
		GetNewTexturePixelFormats( GetArrayBridge(TextureInternalFormats), this->GetFormat() );
	}
#else
	{
		GLint TextureInternalFormat = 0;
		glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
		glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
		glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
		Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()" );
		TextureInternalFormats.PushBack( TextureInternalFormat );
	}
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

	static bool Prefer_SubImage = true;
	static bool Prefer_TexImage = false;
	bool IsSameDimensions = (UsePixels->GetWidth()==TextureWidth) && (UsePixels->GetHeight()==TextureHeight);
	bool SubImage = (!(Params.mStretch)) || (Prefer_SubImage&&IsSameDimensions);
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
	

	/*	gr: re-implement this some time
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
	 */
	
	
	auto& FinalPixels = *UsePixels;
	GLenum FinalPixelsStorage = GL_UNSIGNED_BYTE;
	//	pixel data format
	BufferArray<GLenum,10> FinalPixelsFormats;
	Opengl::GetUploadPixelFormats( GetArrayBridge(FinalPixelsFormats), *this, FinalPixels.GetFormat(), Params.mAllowOpenglConversion );
	if ( FinalPixelsFormats.IsEmpty() )
	{
		std::stringstream Error;
		Error << "Failed to write texture, unsupported upload format " << SourcePixels.GetFormat();
		throw Soy::AssertException(Error.str());
	}


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
		if ( NewTexture )
			PixelsBuffer.Copy( FinalPixels, TSoyPixelsCopyParams() );
		else
			PixelsBuffer.Copy( FinalPixels, TSoyPixelsCopyParams(true,true,true,false,false) );
		
		//	need to work out if it's new to the GPU in case pixels were already allocated
		//if ( NewTexture )
		{
			auto PushTexture = [&](GLenum InternalFormat,GLenum ExternalFormat)
			{
				auto Width = size_cast<GLsizei>(PixelsBuffer.GetWidth());
				auto Height = size_cast<GLsizei>(PixelsBuffer.GetHeight());
				auto* PixelsArrayData = PixelsBuffer.GetPixelsArray().GetArray();
				
				//	gr: very little difference between these two
				//	gr: allowed both here for profiling
				static bool UseSubImage = true;
				static bool UseTexImage = false;
				if ( UseSubImage )
				{
					int XOffset = 0;
					int YOffset = 0;
					glTexSubImage2D( mType, MipLevel, XOffset, YOffset, Width, Height, ExternalFormat, FinalPixelsStorage, PixelsArrayData );
				}
				
				if ( UseTexImage )
				{
					glTexImage2D( mType, MipLevel, InternalFormat, Width, Height, 0, ExternalFormat, FinalPixelsStorage, PixelsArrayData );
				}
			};
			TryFunctionWithFormats( GetArrayBridge(TextureInternalFormats), GetArrayBridge(FinalPixelsFormats), "glTexImage2D(GL_APPLE_client_storage) glSub/TexImage2D", PushTexture );
		}
		glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
		Opengl::IsOkay("glTexImage2D(GL_APPLE_client_storage) glPixelStorei");
		
		//	gr: crashes often on OSX... only on NPOT textures?
		//Opengl::GenerateMipmap( mType );
		Opengl::IsOkay( std::string(__func__) + " post mipmap" );
		
		Unbind();
		Opengl::IsOkay( std::string(__func__) + " unbind()" );
		
		OnWrite();
		return;
	}
#endif
	
	//	try pixel buffer object on desktop
#if !defined(OPENGL_ES)
#endif
	
	//	try subimage
	//	gr: if we find glTexImage2D faster add && !IsSameDimensions
	if ( SubImage && ! (Prefer_TexImage && !IsSameDimensions) )
	{
		int XOffset = 0;
		int YOffset = 0;
		
		auto Width = std::min( TextureWidth, size_cast<GLsizei>(FinalPixels.GetWidth()) );
		auto Height = std::min( TextureHeight, size_cast<GLsizei>(FinalPixels.GetHeight()) );
		
		const ArrayInterface<uint8>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();
		
		//	invalid operation here means the unity pixel format is probably different to the pixel data we're trying to push now
		auto PushTexture = [&](GLenum InternalFormat,GLenum ExternalFormat)
		{
			glTexSubImage2D( mType, MipLevel, XOffset, YOffset, Width, Height, ExternalFormat, FinalPixelsStorage, PixelsArrayData );
		};
		try
		{
			std::stringstream Context;
			Context << FinalPixels.GetMeta() << " glTexImage2D subimage";
			TryFunctionWithFormats( GetArrayBridge(TextureInternalFormats), GetArrayBridge(FinalPixelsFormats), Context.str(), PushTexture );
			//	gr: crashes often on OSX... only on NPOT textures?
			//glGenerateMipmap( mType );
			Opengl::IsOkay( std::string(__func__) + " post mipmap" );
			
			Unbind();
			Opengl::IsOkay( std::string(__func__) + " unbind()" );
			
			OnWrite();
			return;
		}
		catch (std::exception& e)
		{
			std::Debug << "Sub-image failed; " << e.what() << std::endl;
		}
	}
	
	//	final try
	{
		int Border = 0;
		
		//	if texture doesnt fit we'll get GL_INVALID_VALUE
		//	if frame is bigger than texture, it will mangle (bad stride)
		//	if pixels is smaller, we'll just get the sub-image drawn
		auto Width = std::min( TextureWidth, size_cast<GLsizei>(FinalPixels.GetWidth()) );
		auto Height = std::min( TextureHeight, size_cast<GLsizei>(FinalPixels.GetHeight()) );
		
		const ArrayInterface<uint8>& PixelsArray = FinalPixels.GetPixelsArray();
		auto* PixelsArrayData = PixelsArray.GetArray();
	
		BufferArray<GLenum,10> TargetFormats;
		
		//	es requires target & internal format to match
		//	https://www.khronos.org/opengles/sdk/docs/man/xhtml/glTexImage2D.xml
		//	gr: android only?
#if defined(OPENGL_ES)
		TargetFormats.Copy(FinalPixelsFormats);
#else
		//	if conversion allowed...
		TargetFormats.PushBack( GL_RGBA );
		TargetFormats.PushBackArray(FinalPixelsFormats);
#endif
	
		auto PushTexture = [&](GLenum InternalFormat,GLenum ExternalFormat)
		{
			glTexImage2D( mType, MipLevel, InternalFormat,  Width, Height, Border, ExternalFormat, FinalPixelsStorage, PixelsArrayData );
		};
		std::stringstream Context;
		Context << FinalPixels.GetMeta() << " glTexImage2D !subimage";
		TryFunctionWithFormats( GetArrayBridge(TargetFormats), GetArrayBridge(FinalPixelsFormats), Context.str(), PushTexture );

		//	gr: crashes often on OSX... only on NPOT textures?
		//Opengl::GenerateMipmap( mType );
		Opengl::IsOkay( std::string(__func__) + " post mipmap" );
		
		Unbind();
		Opengl::IsOkay( std::string(__func__) + " unbind()" );
		
		OnWrite();
		return;
	}
}

SoyPixelsMeta Opengl::TTexture::GetInternalMeta(GLenum& RealType) const
{
#if defined(OPENGL_ES)
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
#if defined(GL_TEXTURE_1D)
		GL_TEXTURE_1D,
#endif
#if defined(GL_TEXTURE_3D)
		GL_TEXTURE_3D,
#endif
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

void Opengl::TTexture::OnWrite()
{
	//	bit expensive and currently not required on mobile
#if !defined(TARGET_ANDROID) && !defined(TARGET_IOS)
	//	make a new sync fence so we can tell when this write has finished
	mWriteSync.reset( new TSync(true) );
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



bool Opengl::TShaderState::SetUniform(const char* Name,const float3x3& v)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;
	Opengl::SetUniform( Uniform, v );
	return true;
}



bool Opengl::TShaderState::SetUniform(const char* Name,const float& v)
{
	auto Uniform = mShader.GetUniform( Name );
	if ( !Uniform.IsValid() )
		return false;
	Opengl::SetUniform( Uniform, v );
	return true;
}

bool Opengl::TShaderState::SetUniform(const char* Name,const int& v)
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

bool Opengl::TShaderState::SetUniform(const char* Name,const vec3f& v)
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

Opengl::TShader::TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const SoyGraphics::TGeometryVertex& Vertex,const std::string& ShaderName,Opengl::TContext& Context)
{
	std::string ShaderNameVert = ShaderName + " (vert)";
	std::string ShaderNameFrag = ShaderName + " (frag)";
	
	Array<std::string> VertShader;
	Array<std::string> FragShader;
	
	//	gr: strip empty lines as the c include can often have linefeeds at the start
	static bool IncludeEmpty = false;
	Soy::SplitStringLines( GetArrayBridge(VertShader), vertexSrc, IncludeEmpty );
	Soy::SplitStringLines( GetArrayBridge(FragShader), fragmentSrc, IncludeEmpty );
	
	SoyShader::Opengl::UpgradeVertShader( GetArrayBridge(VertShader), Context.mShaderVersion );
	SoyShader::Opengl::UpgradeFragShader( GetArrayBridge(FragShader), Context.mShaderVersion );
	
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
		GLenum Type;
		GLsizei ArraySize = 0;
		glGetActiveAttrib( ProgramName, attrib, size_cast<GLsizei>(nameData.size()), &actualLength, &ArraySize, &Type, &nameData[0]);
		Uniform.mArraySize = ArraySize;
		Uniform.mType = SoyGraphics::GetType( Type );
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
		GLenum Type;
		GLsizei ArraySize = 0;
		glGetActiveUniform( ProgramName, attrib, size_cast<GLsizei>(nameData.size()), &actualLength, &ArraySize, &Type, &nameData[0]);
		Uniform.mArraySize = ArraySize;
		Uniform.mType = SoyGraphics::GetType( Type );
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	todo: check is valid type etc
		mUniforms.PushBack( Uniform );
	}

	static bool DebugUniforms = false;
	if ( DebugUniforms )
	{
		std::Debug << ShaderName << " has " << mAttributes.GetSize() << " attributes; " << std::endl;
		std::Debug << Soy::StringJoin( GetArrayBridge(mAttributes), "\n" );
		std::Debug << std::endl;

		std::Debug << ShaderName << " has " << mUniforms.GetSize() << " uniforms; " << std::endl;
		std::Debug << Soy::StringJoin( GetArrayBridge(mUniforms), "\n" );
		std::Debug << std::endl;
	}
}


Opengl::TShaderState Opengl::TShader::Bind()
{
	Opengl_IsOkay();
	glUseProgram( mProgram.mName );
	Opengl_IsOkay();

	//	gr: this is doing a copy constructor I think. fix this and add NonCopyable to TShaderState
	return TShaderState( *this );
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


void Opengl::TGeometry::EnableAttribs(const SoyGraphics::TGeometryVertex& Descripton,bool Enable)
{
	auto& mElements = Descripton.mElements;
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

Opengl::TGeometry::TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<size_t>&& Indexes,const SoyGraphics::TGeometryVertex& Vertex) :
	mVertexBuffer( GL_ASSET_INVALID ),
	mIndexBuffer( GL_ASSET_INVALID ),
	mVertexArrayObject( GL_ASSET_INVALID ),
	mVertexCount( GL_ASSET_INVALID ),
	mIndexCount( GL_ASSET_INVALID ),
	mIndexType(GL_ASSET_INVALID),
	mVertexDescription	( Vertex )
{
	Opengl_IsOkayFlush();
		
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

		//std::Debug << "Pushing attrib " << AttribIndex << ", arraysize " << Element.mArraySize << ", stride " << Stride << std::endl;
		glEnableVertexAttribArray( AttribIndex );
		glVertexAttribPointer( AttribIndex, Element.mArraySize, GetType(Element.mType), Normalised, Stride, ElementPointer );
		Opengl_IsOkay();
	}
	
	//	gr: disabling vertex attribs stops rendering on ios
	//Vertex.DisableAttribs();

	//	push indexes as glshorts
	Array<GLshort> Indexes16;
	for ( int i=0;	i<Indexes.GetSize();	i++ )
		Indexes16.PushBack( size_cast<GLshort>(Indexes[i]) );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, Indexes16.GetDataSize(), Indexes16.GetArray(), GL_STATIC_DRAW );
	mIndexCount = size_cast<GLsizei>( Indexes16.GetSize() );
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
	Opengl_IsOkayFlush();
	Opengl::BindVertexArray( mVertexArrayObject );
	Opengl_IsOkay();

	//	gr: this only returns true AFTER bind
	if ( !Opengl::IsVertexArray( mVertexArrayObject ) )
		throw Soy::AssertException("Bound VAO is reported not a VAO");
}

void Opengl::TGeometry::Unbind()
{
	//	unbinding so nothing alters it
	//	gr: if we don't unbind on windows, we crash in nvidia driver (win7)
#if defined(TARGET_IOS)|| defined(TARGET_WINDOWS)
	static bool DoUnbind = true;
#else
	static bool DoUnbind = false;
#endif
	if ( DoUnbind )
	{
		Opengl::BindVertexArray( 0 );
		Opengl_IsOkay();
	}
}


void Opengl::TGeometry::Draw()
{
	Bind();

	//	should already be bound, so these should do nothing
	glBindBuffer( GL_ARRAY_BUFFER, mVertexBuffer );
	Opengl_IsOkay();
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
	Opengl_IsOkay();

#if defined(TARGET_IOS)
	static bool DoEnableAttribs = false;
#else
	static bool DoEnableAttribs = true;
#endif
	if ( DoEnableAttribs )
	{
		//	should be left enabled
		EnableAttribs(mVertexDescription);
		Opengl_IsOkay();
	}

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
		Opengl::DeleteVertexArrays( 1, &mVertexArrayObject );
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

//	gr: put R8 first as note4 works with glTexImage R8->RED, but not RED->RED or RED->R8
//	gr: GLR8 doesn't work on windows8, glcore, so put GL_RED first
const std::initializer_list<GLenum> Opengl8BitFormats =
{
#if defined(TARGET_WINDOWS)
	GL_RED,
	GL_R8,
#else
	GL_R8,
	GL_RED,
#endif

#if defined(GL_LUMINANCE)
	GL_LUMINANCE,
#endif
	GL_ALPHA,
};
const std::initializer_list<GLenum> Opengl16BitFormats =
{
	GL_RG,
#if defined(GL_LUMINANCE_ALPHA)
	GL_LUMINANCE_ALPHA,
#endif
};

const Array<TPixelFormatMapping>& Opengl::GetPixelFormatMap()
{
	static TPixelFormatMapping _PixelFormatMap[] =
	{
#if defined(GL_RGB8)
		TPixelFormatMapping( SoyPixelsFormat::RGB,			{GL_RGB, GL_RGB8} ),
#else
		TPixelFormatMapping( SoyPixelsFormat::RGB,			{GL_RGB} ),
#endif
		
#if defined(GL_RGBA8)
		TPixelFormatMapping( SoyPixelsFormat::RGBA,			{GL_RGBA, GL_RGBA8} ),
#else
		TPixelFormatMapping( SoyPixelsFormat::RGBA,			{GL_RGBA} ),
#endif

		//	gr: untested
		//	gr: use this with 8_8_8_REV to convert to BGRA!
		TPixelFormatMapping( SoyPixelsFormat::ARGB,			{GL_RGBA, GL_RGBA8} ),
		
		TPixelFormatMapping( SoyPixelsFormat::Luma_Full,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::Luma_Ntsc,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::Luma_Smptec,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::Greyscale,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::ChromaUV_8_8,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::ChromaU_8,	Opengl8BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::ChromaV_8,	Opengl8BitFormats ),

		TPixelFormatMapping( SoyPixelsFormat::ChromaUV_88,			Opengl16BitFormats ),
		TPixelFormatMapping( SoyPixelsFormat::GreyscaleAlpha,		Opengl16BitFormats ),
		TPixelFormatMapping(SoyPixelsFormat::KinectDepth,			Opengl16BitFormats ),
		TPixelFormatMapping(SoyPixelsFormat::FreenectDepth10bit,	Opengl16BitFormats ),
		TPixelFormatMapping(SoyPixelsFormat::FreenectDepth11bit,	Opengl16BitFormats ),
		TPixelFormatMapping(SoyPixelsFormat::FreenectDepthmm,		Opengl16BitFormats ),

#if defined(GL_BGRA)
		//	BGRA is not a valid internal format
		TPixelFormatMapping( SoyPixelsFormat::BGRA,			{ GL_BGRA	}, { GL_RGBA	} ),
#else
		TPixelFormatMapping( SoyPixelsFormat::BGRA,			{ GL_RGBA	} ),
#endif

#if defined(GL_BGR)
		TPixelFormatMapping( SoyPixelsFormat::BGR,			{ GL_BGR	} ),
#else
		TPixelFormatMapping( SoyPixelsFormat::BGR,			{ GL_RGB	} ),
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

void Opengl::GetUploadPixelFormats(ArrayBridge<GLenum>&& Formats,const Opengl::TTexture& Texture,SoyPixelsFormat::Type Format,bool AllowConversion)
{
	/*
	if ( Texture.IsValid() )
	{
		//	ios requires formats to match, no internal conversion
	#if defined(TARGET_IOS)
		bool ForceAllow = false;

		//	allow RGBA as BGRA
		ForceAllow = ( Texture.GetFormat() == SoyPixelsFormat::RGBA && Format == SoyPixelsFormat::BGRA );
			
		if ( Texture.GetFormat() != Format && !ForceAllow )
		{
			std::stringstream Error;
			Error << "ios texture upload requires texture format(" << Texture.GetFormat() << " to match source format (" << Format << ")";
			throw Soy::AssertException( Error.str() );
		}
		auto Mapping = GetPixelMapping( Texture.GetFormat() );
		return Mapping.mUploadFormat;
	#endif
	}
	*/
	auto Mapping = GetPixelMapping( Format );
	Formats.PushBackArray( Mapping.mOpenglFormats );
}


void Opengl::GetNewTexturePixelFormats(ArrayBridge<GLenum>&& Formats,SoyPixelsFormat::Type Format)
{
	auto Mapping = GetPixelMapping( Format );
	
	if ( Mapping.mOpenglInternalFormats.IsEmpty() )
	{
		Formats.PushBackArray( Mapping.mOpenglFormats );
	}
	else
	{
		Formats.PushBackArray( Mapping.mOpenglInternalFormats );
	}
}


void Opengl::GetDownloadPixelFormats(ArrayBridge<GLenum>&& Formats,const TTexture& Texture,SoyPixelsFormat::Type PixelFormat)
{
	auto Mapping = GetPixelMapping( PixelFormat );
	Formats.PushBackArray( Mapping.mOpenglFormats );
}


SoyPixelsFormat::Type Opengl::GetDownloadPixelFormat(GLenum Format)
{
	auto& FormatMaps = GetPixelFormatMap();
	for ( int i=0;	i<FormatMaps.GetSize();	i++ )
	{
		auto& FormatMap = FormatMaps[i];
		
		if ( FormatMap.mOpenglFormats.Find(Format) )
			return FormatMap.mPixelFormat;
	}

	std::Debug << "Failed to convert glpixelformat " << GetEnumString(Format) << " to soy pixel format" << std::endl;
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


Opengl::TSync::TSync(bool Create) :
	mSyncObject	( nullptr )
{
	//	gr: swap this for Context::IsSupported
	if ( Create )
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
	{
		mSyncObject = Opengl::FenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		Opengl::IsOkay("glFenceSync");
	}
#else
	{
		mSyncObject = true;
	}
#endif
}

void Opengl::TSync::Delete()
{
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
	if ( mSyncObject )
	{
		Opengl::DeleteSync( mSyncObject );
		Opengl::IsOkay("glDeleteSync");
		mSyncObject = nullptr;
	}
#else
	mSyncObject = false;
#endif
}

void Opengl::TSync::Wait(const char* TimerName)
{
#if (OPENGL_ES==3) || (OPENGL_CORE==3)
	if ( !Opengl::IsSync( mSyncObject ) )
		return;
	
	//glWaitSync( mSyncObject, 0, GL_TIMEOUT_IGNORED );
	GLbitfield Flags = GL_SYNC_FLUSH_COMMANDS_BIT;
	SoyTime Timeout( std::chrono::milliseconds(1000) );
	GLuint64 TimeoutNs = Timeout.GetNanoSeconds();
	GLenum Result = GL_INVALID_ENUM;
	do
	{
		Result = Opengl::ClientWaitSync( mSyncObject, Flags, TimeoutNs );
	}
	while ( Result == GL_TIMEOUT_EXPIRED );
	
	if ( Result == GL_WAIT_FAILED )
		Opengl::IsOkay("glClientWaitSync GL_WAIT_FAILED");
	
#else
	if ( mSyncObject )
		glFinish();
#endif

	Delete();
}
