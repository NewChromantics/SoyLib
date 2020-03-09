#include "SoyOpengl.h"
#include "RemoteArray.h"
#include "SoyOpenglContext.h"
#include "SoyShader.h"
#include <regex>


#if defined(TARGET_IOS)
#define glProgramUniformMatrix4fv	glProgramUniformMatrix4fvEXT

#define glProgramUniform1fv			glProgramUniform1fvEXT
#define glProgramUniform2fv			glProgramUniform2fvEXT
#define glProgramUniform3fv			glProgramUniform3fvEXT
#define glProgramUniform4fv			glProgramUniform4fvEXT

#define glProgramUniform1iv			glProgramUniform1ivEXT
#define glProgramUniform2iv			glProgramUniform2ivEXT
#define glProgramUniform3iv			glProgramUniform3ivEXT
#define glProgramUniform4iv			glProgramUniform4ivEXT

#define glProgramUniform1uiv		glProgramUniform1uivEXT
#define glProgramUniform2uiv		glProgramUniform2uivEXT
#define glProgramUniform3uiv		glProgramUniform3uivEXT
#define glProgramUniform4uiv		glProgramUniform4uivEXT

#define glProgramUniform1i			glProgramUniform1iEXT


#endif

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
	std::pair<GLenum,GLint>	GetType(SoyGraphics::TElementType::Type Type);
	
	extern ssize_t	gTextureAllocationCount;
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
			//if ( !Texture.mClientBuffer )
			//	Texture.mClientBuffer.reset( new SoyPixels );
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


template<> GLenum Opengl::GetTypeEnum<uint16_t>()	{	return GL_UNSIGNED_SHORT;	}
template<> GLenum Opengl::GetTypeEnum<GLshort>()	{	return GL_UNSIGNED_SHORT;	}
//template<> GLenum Opengl::GetTypeEnum<GLushort>()	{	return GL_UNSIGNED_SHORT;	}

template<> GLenum Opengl::GetTypeEnum<uint32_t>()	{	return GL_UNSIGNED_INT;	}
//template<> GLenum Opengl::GetTypeEnum<GLuint>()		{	return GL_UNSIGNED_INT;	}
template<> GLenum Opengl::GetTypeEnum<GLint>()		{	return GL_UNSIGNED_INT;	}

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

#if defined(GL_R32F)
			CASE_ENUM_STRING( GL_R32F );
#endif
#if defined(GL_RG32F)
			CASE_ENUM_STRING( GL_RG32F );
#endif
#if defined(GL_RGB32F)
			CASE_ENUM_STRING( GL_RGB32F );
#endif
#if defined(GL_RGBA32F)
			CASE_ENUM_STRING( GL_RGBA32F );
#endif

	};
#undef CASE_ENUM_STRING
	std::stringstream Unknown;
	Unknown << "<GLenum 0x" << std::hex << Type << std::dec << ">";
	return Unknown.str();
}



SoyGraphics::TElementType::Type SoyGraphics::GetType(GLenum Type)
{
	switch ( Type )
	{
		case GL_INVALID_ENUM:	return SoyGraphics::TElementType::Invalid;
		case GL_INT:			return SoyGraphics::TElementType::Int;
		case GL_INT_VEC2:		return SoyGraphics::TElementType::Int2;
		case GL_INT_VEC3:		return SoyGraphics::TElementType::Int3;
		case GL_INT_VEC4:		return SoyGraphics::TElementType::Int4;
		case GL_UNSIGNED_INT:			return SoyGraphics::TElementType::Uint;
		case GL_UNSIGNED_INT_VEC2:		return SoyGraphics::TElementType::Uint2;
		case GL_UNSIGNED_INT_VEC3:		return SoyGraphics::TElementType::Uint3;
		case GL_UNSIGNED_INT_VEC4:		return SoyGraphics::TElementType::Uint4;
		case GL_FLOAT:			return SoyGraphics::TElementType::Float;
		case GL_FLOAT_VEC2:		return SoyGraphics::TElementType::Float2;
		case GL_FLOAT_VEC3:		return SoyGraphics::TElementType::Float3;
		case GL_FLOAT_VEC4:		return SoyGraphics::TElementType::Float4;
		case GL_FLOAT_MAT3:		return SoyGraphics::TElementType::Float3x3;
		case GL_FLOAT_MAT4:		return SoyGraphics::TElementType::Float4x4;
		case GL_SAMPLER_2D:		return SoyGraphics::TElementType::Texture2D;
		case GL_BOOL:			return SoyGraphics::TElementType::Bool;

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

std::pair<GLenum,GLint> Opengl::GetType(SoyGraphics::TElementType::Type Type)
{
	switch ( Type )
	{
		case SoyGraphics::TElementType::Invalid:	return std::make_pair( GL_INVALID_ENUM, 0 );
		case SoyGraphics::TElementType::Int:		return std::make_pair( GL_INT, 1 );
		case SoyGraphics::TElementType::Int2:		return std::make_pair( GL_INT, 2 );
		case SoyGraphics::TElementType::Int3:		return std::make_pair( GL_INT, 3 );
		case SoyGraphics::TElementType::Int4:		return std::make_pair( GL_INT, 4 );
		case SoyGraphics::TElementType::Uint:		return std::make_pair( GL_UNSIGNED_INT, 1 );
		case SoyGraphics::TElementType::Uint2:		return std::make_pair( GL_UNSIGNED_INT, 2 );
		case SoyGraphics::TElementType::Uint3:		return std::make_pair( GL_UNSIGNED_INT, 3 );
		case SoyGraphics::TElementType::Uint4:		return std::make_pair( GL_UNSIGNED_INT, 4 );
		case SoyGraphics::TElementType::Float:		return std::make_pair( GL_FLOAT, 1 );
		case SoyGraphics::TElementType::Float2:		return std::make_pair( GL_FLOAT, 2 );
		case SoyGraphics::TElementType::Float3:		return std::make_pair( GL_FLOAT, 3 );
		case SoyGraphics::TElementType::Float4:		return std::make_pair( GL_FLOAT, 4 );
		case SoyGraphics::TElementType::Bool:		return std::make_pair( GL_BOOL, 1 );

		//	see here on how to handle matrixes
		//	calling code needs to change
		//	https://www.opengl.org/discussion_boards/showthread.php/164099-how-to-specify-a-matrix-vertex-attribute
		case SoyGraphics::TElementType::Float3x3:
			throw Soy::AssertException("Not currently handling matrixes in vertex attribs");
		case SoyGraphics::TElementType::Float4x4:
			throw Soy::AssertException("Not currently handling matrixes in vertex attribs");

		case SoyGraphics::TElementType::Texture2D:	return std::make_pair( GL_SAMPLER_2D, 1 );
	}

	throw Soy::AssertException( std::string("Unknown SoyGraphics::TElementType::Type->Glenum conversion for ") );
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const float3x3& Value)
{
	GLsizei ArraySize = 1;
	static GLboolean Transpose = false;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniformMatrix3fv( static_cast<GLint>(Uniform.mIndex), ArraySize, Transpose, Value.m );
	Opengl_IsOkay();
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const vec4f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform4fv( static_cast<GLint>(Uniform.mIndex), ArraySize, &Value.x );
	Opengl_IsOkay();
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const vec3f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform3fv( static_cast<GLint>(Uniform.mIndex), ArraySize, &Value.x );
	Opengl_IsOkay();
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const vec2f& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform2fv( static_cast<GLint>(Uniform.mIndex), ArraySize, &Value.x );
	Opengl_IsOkay();
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const float& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform1fv( static_cast<GLint>(Uniform.mIndex), ArraySize, &Value );
	Opengl_IsOkay();
}

template<>
void Opengl::SetUniform(const TUniform& Uniform,const int& Value)
{
	GLsizei ArraySize = 1;
	Soy::Assert( ArraySize == Uniform.mArraySize, "Uniform array size mis match" );
	glUniform1iv( static_cast<GLint>(Uniform.mIndex), ArraySize, &Value );
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

GLenum GetGlPixelStorageFormat(SoyPixelsFormat::Type Format)
{
	if ( SoyPixelsFormat::IsFloatChannel(Format) )
		return GL_FLOAT;
	
	auto ComponentSize = SoyPixelsFormat::GetBytesPerChannel(Format);
	switch ( ComponentSize )
	{
		case 1:	return GL_UNSIGNED_BYTE;
		case 2:	return GL_UNSIGNED_SHORT;
		case 4:	return GL_INT;
			
		default:	break;
	}
	
	std::stringstream Error;
	Error << "Unhandled format (" << Format << ") component size " << ComponentSize;
	throw Soy_AssertException(Error);
}

Opengl::TTexture::TTexture(SoyPixelsMeta Meta,GLenum Type,size_t TextureSlot) :
	mAutoRelease	( true ),
	mType			( Type )
{
	static bool DebugContruction = false;
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
	gTextureAllocationCount += 1;
	
	Bind(TextureSlot);
	Opengl::IsOkay("glGenTextures");
	
	//	set mip-map levels to 0..0
	GLint MipLevel = 0;
	//	storage = u8, u16, u32, float
	GLenum GlPixelsStorage = GetGlPixelStorageFormat(Meta.GetFormat());
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
		int FailedTrys = 0;
		bool Created = false;
		GLenum SuccessfullInternalPixelFormat = 0;
		GLenum SuccessfullExternalPixelFormat = 0;
		
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
					Error << "glTexImage2D texture construction " << Meta << " InternalPixelFormat=" << GetEnumString(InternalPixelFormat) << " ExternalPixelFormat=" << GetEnumString(ExternalPixelFormat) << ", GlPixelsStorage=" << GetEnumString(GlPixelsStorage);
					Opengl::IsOkay( Error.str() );
					
					Created = true;
					SuccessfullInternalPixelFormat = InternalPixelFormat;
					SuccessfullExternalPixelFormat = ExternalPixelFormat;
				}
				catch( std::exception& e)
				{
					FailedTrys++;
					if ( DebugContruction )
						std::Debug << e.what() << std::endl;
				}
			}
		}
		
		if ( !Created )
		{
			std::stringstream Error;
			Error << "Failed to create texture with " << Meta;
			throw Soy::AssertException(Error);
		}
		
		//	if it did succeed, but some failed first, report them for debugging
		if ( DebugContruction )//&& FailedTrys > 0 )
		{
			std::Debug << "Successfull glTexImage2D texture construction " << Meta;
			std::Debug << " InternalPixelFormat=" << GetEnumString(SuccessfullInternalPixelFormat);
			std::Debug << " ExternalPixelFormat=" << GetEnumString(SuccessfullExternalPixelFormat);
			std::Debug << " GlPixelsStorage=" << GetEnumString(GlPixelsStorage);
			std::Debug << std::endl;
		}
	}
	
	//	verify params
	//	gr: this should be using internal meta
#if (OPENGL_CORE==3)
	GLint TextureWidth = -1;
	GLint TextureHeight = -1;
	GLint TextureInternalFormat = -1;
	
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_WIDTH, &TextureWidth);
	Opengl::IsOkay( "TTexture::TTexture() glGetTexLevelParameteriv(GL_TEXTURE_WIDTH)" );
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_HEIGHT, &TextureHeight);
	Opengl::IsOkay( "TTexture::TTexture() glGetTexLevelParameteriv(GL_TEXTURE_HEIGHT)" );
	glGetTexLevelParameteriv (mType, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &TextureInternalFormat);
	Opengl::IsOkay( "TTexture::TTexture() glGetTexLevelParameteriv(GL_TEXTURE_INTERNAL_FORMAT)" );
	
	Soy::Assert( TextureWidth==Meta.GetWidth(), "Texture width doesn't match initialisation");
	Soy::Assert( TextureHeight==Meta.GetHeight(), "Texture height doesn't match initialisation");

	//	gr: internal format comes out as RGBA8 even if we specify RGBA. Need to have some "is same format" func
	//Soy::Assert( TextureInternalFormat==InternalPixelFormat, "Texture format doesn't match initialisation");
	//std::Debug << "new texture format "  << Opengl::GetEnumString(TextureInternalFormat) << std::endl;
#endif
	
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
		gTextureAllocationCount -= 1;
		mTexture.mName = GL_ASSET_INVALID;
	}
	
	mWriteSync.reset();
}

void Opengl::TTexture::Bind(size_t& TextureSlot) const
{
	Opengl::IsOkay("Texture bind flush",false);
	
	const GLenum _TexturesBindings[] =
	{
		GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7, GL_TEXTURE8, GL_TEXTURE9,
		GL_TEXTURE10, GL_TEXTURE11, GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14, GL_TEXTURE15, GL_TEXTURE16, GL_TEXTURE17, GL_TEXTURE18, GL_TEXTURE19,
		GL_TEXTURE20, GL_TEXTURE21, GL_TEXTURE22, GL_TEXTURE23, GL_TEXTURE24, GL_TEXTURE25, GL_TEXTURE26, GL_TEXTURE27, GL_TEXTURE28, GL_TEXTURE29,
	};
	auto TextureBindings = GetRemoteArray( _TexturesBindings );
	TextureSlot %= TextureBindings.GetSize();

	Soy::Assert( mTexture.IsValid(), "Trying to bind invalid texture" );

	//std::Debug << "glActiveTexture(" << TextureSlot << ")" << std::endl;
	glActiveTexture( TextureBindings[TextureSlot] );
	Opengl::IsOkay("Opengl::TTexture::Bind() glActiveTexture");
	
	glBindTexture( mType, mTexture.mName );
	Opengl::IsOkay("TShaderState::BindTexture glBindTexture");
}

void Opengl::TTexture::Unbind() const
{
	//	check if texture slot/active texture has changed?
	glBindTexture( mType, GL_ASSET_INVALID );
	Opengl_IsOkay();
}

void Opengl::TTexture::CheckIsBound() const
{
	GLint CurrentBound;
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &CurrentBound );
	Opengl::IsOkay("glGetIntegerv( GL_TEXTURE_BINDING_2D);");
	if ( CurrentBound == mTexture.mName )
		return;
	
	throw Soy::AssertException("Texture is not bound");
}

void Opengl::TTexture::SetRepeat(bool Repeat)
{
	CheckIsBound();
	
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
}

void Opengl::TTexture::SetFilter(bool Linear)
{
	CheckIsBound();

	auto Type = mType;
	glTexParameteri( Type, GL_TEXTURE_MIN_FILTER, Linear ? GL_LINEAR : GL_NEAREST );
	glTexParameteri( Type, GL_TEXTURE_MAG_FILTER, Linear ? GL_LINEAR : GL_NEAREST );
	Opengl_IsOkay();
}

void Opengl::TTexture::GenerateMipMaps()
{
	Soy::Assert( IsValid(), "Generate mip maps on invalid texture");
	
	//	no mip maps on external textures [citation needed - extension spec IIRC]
#if defined(GL_TEXTURE_EXTERNAL_OES)
	if ( mType == GL_TEXTURE_EXTERNAL_OES )
		return;
#endif
	
	CheckIsBound();

	//	gr: this can be slow, highlight it
	Soy::TScopeTimerPrint Timer("glGenerateMipmap",2);
	
	Opengl::GenerateMipmap( mType );
	std::stringstream Error;
	Error << "Texture(" << Opengl::GetEnumString(mType) << " " << mMeta << ")::GenerateMipMaps";
	Opengl::IsOkay( Error.str(), false );
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

void Opengl::TPbo::ReadPixels(GLenum PixelType)
{
	GLint x = 0;
	GLint y = 0;
	
	BufferArray<GLenum,5> FboFormats;
	GetReadPixelsFormats( GetArrayBridge(FboFormats) );

	auto ChannelCount = mMeta.GetChannels();
	
	Bind();

	auto ColourFormat = FboFormats[ChannelCount];
	auto ReadFormat = PixelType;
	
	auto w = size_cast<GLsizei>(mMeta.GetWidth());
	auto h = size_cast<GLsizei>(mMeta.GetHeight());
	glReadPixels( x, y, w, h, ColourFormat, ReadFormat, nullptr );
	Opengl_IsOkay();
	
	Unbind();
}

const uint8* Opengl::TPbo::LockBuffer()
{
#if defined(TARGET_IOS) || defined(TARGET_ANDROID)
	//	gr: come back to this... when needed, I think it's supported
	const uint8* Buffer = nullptr;
#else
	Bind();
	//glMapNamedBuffer would be nicer;
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
	Unbind();
#endif
}

void Opengl::TTexture::Read(SoyPixelsImpl& Pixels,SoyPixelsFormat::Type ForceFormat,bool Flip) const
{
	Soy::Assert( IsValid(), "Trying to read from invalid texture" );
	std::stringstream TimerName;
	TimerName << "Opengl::TTexture::Read " << Pixels.GetMeta();
	Soy::TScopeTimerPrint Timer( TimerName.str().c_str(), 30);
	CheckIsBound();
	
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
	auto PixelBytesSize = Pixels.GetPixelsArray().GetDataSize();

	
	//	try to use PBO's
	static bool Default_UsePbo = true;
	static bool Default_UseFbo = true;

	bool UsePbo = Default_UsePbo;
	bool UseFbo = Default_UseFbo;
	
	if ( UsePbo )
	{
		TFbo FrameBuffer( *this );
		FrameBuffer.Bind();
		TPbo PixelBuffer( Pixels.GetMeta() );
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

	//	gr; bodge for floats which don't change the output type
	//		need to change GetDownloadPixelFormat to handle 2 types of output (eg. RGBA and Flaot4)
	bool FboFloatFormat = false;
	//	gr: sometimes the caller wants a specific format, and FBO(glReadPixels) doesn't allow that
	if ( UseFbo )
	{
		auto PrefferedFormat = Pixels.GetFormat();
		
		if ( SoyPixelsFormat::IsFloatChannel(PrefferedFormat) )
		{
			FboFloatFormat = true;
		}
		else
		{
			if ( PrefferedFormat != GetDownloadPixelFormat(FboFormats[1]) &&
				PrefferedFormat != GetDownloadPixelFormat(FboFormats[3]) &&
				PrefferedFormat != GetDownloadPixelFormat(FboFormats[4]) )
			{
				UseFbo = false;
			}
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
		
#define GL_HALF_FLOAT_OES	0x8d61
		//	GL_RGBA16F	//	unity uses this as the colour for half floats
		//	GL_HALF_FLOAT for ES3	implicit type is still GL_RGBA

		static bool UseHalfFloat = false;
		
		auto ReadFormat = FboFloatFormat ? (UseHalfFloat ? GL_HALF_FLOAT_OES : GL_FLOAT) : GL_UNSIGNED_BYTE;
		auto Width = size_cast<GLsizei>( Pixels.GetWidth() );
		auto Height = size_cast<GLsizei>( Pixels.GetHeight() );
		
		//	just to check as we'll crash hard if we over-write
		auto FormatComponentSize = (ReadFormat == GL_HALF_FLOAT_OES) ? 2 : ((ReadFormat == GL_FLOAT) ? 4 : 1);
		auto ReadSize = Width * Height * ChannelCount * FormatComponentSize;
		if ( ReadSize > PixelBytesSize )
		{
			std::stringstream ErrorStr;
			ErrorStr << __func__ << "about to glReadPixels " << ReadSize << " bytes into " << PixelBytesSize << " buffer";
			throw Soy::AssertException( ErrorStr.str() );
		}
		
		auto ColourFormat = FboFormats[ChannelCount];
		static bool UseRgba16f = false;
		if ( UseRgba16f )
			ColourFormat = GL_RGBA16F;

		
		//	query internal format to avoid conversion.
		//	glReadPixels ES3 allows RGBA and RGBA_INTEGER, and the internal format
		//	https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glReadPixels.xhtml
		
		// no-conversion reading format/type
		GLenum implReadFormat=0, implReadType=0;
		try
		{
			glGetIntegerv( GL_IMPLEMENTATION_COLOR_READ_FORMAT, (GLint*)&implReadFormat );
			Opengl::IsOkay("GL_IMPLEMENTATION_COLOR_READ_FORMAT");
			glGetIntegerv( GL_IMPLEMENTATION_COLOR_READ_TYPE, (GLint*)&implReadType );
			Opengl::IsOkay("GL_IMPLEMENTATION_COLOR_READ_TYPE");
		}
		catch(std::exception& e)
		{
			Platform::DebugPrint( e.what() );
		}

		static bool UseImplReadFormat = false;
		if ( UseImplReadFormat && implReadFormat != 0 )
			ColourFormat = implReadFormat;
		
		static bool UseImplReadType = false;
		if ( UseImplReadType && implReadType != 0 )
			ReadFormat = implReadType;
		
		
		glReadPixels( x, y, Width, Height, ColourFormat, ReadFormat, PixelBytes );
		Opengl::IsOkay("glReadPixels");

		if ( !FboFloatFormat )
		{
			//	as glReadPixels forces us to a format, we need to update the meta on the pixels
			auto NewFormat = GetDownloadPixelFormat( FboFormats[ChannelCount] );
			Pixels.GetMeta().DumbSetFormat( NewFormat );
		}
		
		FrameBuffer.Unbind();
		Opengl_IsOkay();
	}
	else
	{
		//	no glGetTexImage on opengl ES
#if defined(TARGET_IOS)||defined(TARGET_ANDROID)
		throw Soy::AssertException("glGetTexImage not supported on opengl ES");
#else
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
#endif
	}
	
	static int DebugPixelCount_ = 0;
	if ( DebugPixelCount_ > 0 )
	{
		//auto DebugPixelCount = std::min<size_t>( DebugPixelCount_, Pixels.GetPixelsArray().GetDataSize() );
		auto& DebugStream = std::Debug.LockStream();
		Pixels.PrintPixels("Read pixels from texture: ", DebugStream, false, " " );
		std::Debug.UnlockStream( DebugStream );
	}
	
	Opengl_IsOkay();
	
	//	textures are stored upside down so flip here. (maybe store as transform meta data?)
	if ( Flip )
	{
		Pixels.Flip();
	}

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

SoyPixelsRemote	GetRealignedSinglePlanePixels(const SoyPixelsImpl& Pixels)
{
	//	maybe a better approach by finding pixels array size vs w/h
	if (Pixels.GetFormat() == SoyPixelsFormat::Yuv_8_8_8_Full ||
		Pixels.GetFormat() == SoyPixelsFormat::Yuv_8_8_8_Ntsc )
	{
		auto* Data = Pixels.GetPixelsArray().GetArray();
		auto DataSize = Pixels.GetPixelsArray().GetDataSize();
		auto Width = Pixels.GetWidth();
		auto Height = DataSize / Width;
		auto Format = SoyPixelsFormat::Greyscale;
		return SoyPixelsRemote( const_cast<uint8_t*>(Data), Width, Height, DataSize, Format);
	}

	return SoyPixelsRemote();
}


void Opengl::TTexture::Write(const SoyPixelsImpl& SourcePixels,SoyGraphics::TTextureUploadParams Params)
{
	std::stringstream WholeFunctionContext;
	WholeFunctionContext << __func__ << " (" << SourcePixels.GetMeta() << "->" << this->GetMeta() << ")";
	Soy::TScopeTimerPrint WholeTimer( WholeFunctionContext.str().c_str(), 30 );
	
	Soy::Assert( IsValid(), "Trying to upload to invalid texture ");
	CheckIsBound();

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

	//	yuv_8_8_8 is 3 planes (different sizes) crammed into one buffer
	//	as the plane sizes are different, we can't do multiple channels,
	//	so in this case (and maybe others) we need to turn it into one giant luma image
	auto RealignedForUploadPixels = GetRealignedSinglePlanePixels(*UsePixels);
	if (RealignedForUploadPixels.IsValid() )
		UsePixels = &RealignedForUploadPixels;

	
	//	we CANNOT go from big to small, only small to big, so to avoid corrupted textures, lets shrink
	SoyPixels Resized;
	static bool AllowClipResize = false;
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
	GLenum FinalPixelsStorage = GetGlPixelStorageFormat( SourcePixels.GetFormat() );
														
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
	
	//	if there's a crash below, (reading out of bounds) may be beause opengl is expecting padding of 2/4/8
	//	our pixels are not padded!
	//	make sure no padding is applied so glGetTexImage & glReadPixels doesn't override tail memory
	//glPixelStorei(GL_PACK_ROW_LENGTH, FinalPixels.GetWidth() );	//	gr: not sure this had any effect, but implemented anyway
	glPixelStorei(GL_PACK_ROW_LENGTH, 0 );	//	gr: not sure this had any effect, but implemented anyway
	Opengl::IsOkay("glPixelStorei(GL_PACK_ROW_LENGTH,0)");
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	Opengl::IsOkay("glPixelStorei(GL_PACK_ALIGNMENT,1)");

	//	this was failing to upload 50x50 images correctly.
	//	todo: fix the unpack alignment so it's more efficient when widths are aligned
	//	I thought the packing was the problem, but it's the unpacking (reading of data provided?)
	//	https://stackoverflow.com/a/11264136/355753
	//	alignment of 1 means it's byte aligned, 2 is even, 4 is aligned to 4
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//	gr: this is still crashing from overread!	
	if ( !IsSameDimensions )
		SubImage = false;
	
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
		bool ResizeTexture = true;
		
		
		//	if texture doesnt fit we'll get GL_INVALID_VALUE
		//	if frame is bigger than texture, it will mangle (bad stride)
		//	if pixels is smaller, we'll just get the sub-image drawn
		auto Width = std::min( TextureWidth, size_cast<GLsizei>(FinalPixels.GetWidth()) );
		auto Height = std::min( TextureHeight, size_cast<GLsizei>(FinalPixels.GetHeight()) );
		
		if ( ResizeTexture )
		{
			Width = size_cast<GLsizei>(FinalPixels.GetWidth());
			Height = size_cast<GLsizei>(FinalPixels.GetHeight());
		}
		
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


void Opengl::TTexture::RefreshMeta()
{
	GLenum Type = 0;
	auto NewMeta = GetInternalMeta(Type);
	if ( !NewMeta.IsValid() )
	{
		//std::Debug << "Opengl::TTexture meta not refreshed as internal meta invalid " << NewMeta << std::endl;
		//	BAD!
		NewMeta.DumbSetFormat(SoyPixelsFormat::RGBA);
		//return;
	}

	if ( this->mMeta != NewMeta )
	{
		//std::Debug << "Opengl::TTexture meta changed from " << this->mMeta << " " << NewMeta << std::endl;
		this->mMeta = NewMeta;
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
		CheckIsBound();
		
		GLint MipLevel = 0;
		GLint Width = 0;
		GLint Height = 0;
		GLint Format = 0;
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_WIDTH, &Width);
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_HEIGHT, &Height);
		glGetTexLevelParameteriv( Type, MipLevel, GL_TEXTURE_INTERNAL_FORMAT, &Format );
		
		if ( !Opengl::IsOkay( std::string(__func__) + " glGetTexLevelParameteriv()", false ) )
			continue;

#if defined(GL_TEXTURE_COMPONENTS)
		if (Format == 0)
		{
			GLint Components = 0;
			glGetTexLevelParameteriv(Type, MipLevel, GL_TEXTURE_COMPONENTS, &Components);
			if (Opengl::IsOkay("glGetTexLevelParameteriv(GL_TEXTURE_COMPONENTS)", false))
			{
				if (Components!=0)
					std::Debug << "Invalid format, but components = " << Components << std::endl;
			}
		}
#endif

		//	we probably won't get an opengl error, but the values won't be good. We can assume it's not that type
		//	tested on osx
		if ( Width==0 || Height==0 )
			continue;
		
		//	gr: openvr shared texture has format=0 & components=0
		//if (Format == 0)
		//	continue;

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
	//	do we need to unbind textres?
	//	glActiveTexture should be done properly now via Bind()
	/*
	//	unbind textures
	TTexture NullTexture;
	while ( mTextureBindCount > 0 )
	{
		//	uniform doesn't matter for unbinding
		BindTexture( mTextureBindCount-1, NullTexture, nullptr );
		mTextureBindCount--;
	}
	
	std::Debug << "glActiveTexture(" << 0 << ")" << std::endl;
	glActiveTexture( GL_TEXTURE0 );
	Opengl::IsOkay("~TShaderState glActiveTexture( GL_TEXTURE0 )");
*/
	
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
	BindTexture( BindTextureIndex, Texture, Uniform.mIndex );
	mTextureBindCount++;
	return true;
}


void Opengl::TShaderState::BindTexture(size_t TextureIndex,TTexture Texture,std::function<void(GLuint)> SetUniform)
{
	Texture.Bind( TextureIndex );

	if ( SetUniform != nullptr )
	{
		auto TextureIndexInt = size_cast<GLuint>(TextureIndex);
		SetUniform( TextureIndexInt );
		Opengl::IsOkay("TShaderState::BindTexture SetUniform()",false);
	}
}

void Opengl::TShaderState::BindTexture(size_t TextureIndex,TTexture Texture,size_t UniformIndex)
{
	auto SetUniformIndex = [&](GLuint TextureIndex)
	{
		auto UniformIndexInt = size_cast<GLint>(UniformIndex);
		glUniform1i( UniformIndexInt, TextureIndex );
	};
	BindTexture( TextureIndex, Texture, SetUniformIndex );
}


Opengl::TShader::TShader(const std::string& vertexSrc,const std::string& fragmentSrc,const std::string& ShaderName,Opengl::TContext& Context)
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
	
	static bool DebugUniforms = false;

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
		Opengl::IsOkay("glGetActiveAttrib");
		
		Uniform.mArraySize = ArraySize;
		Uniform.mType = SoyGraphics::GetType( Type );
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	refetch the real location. This changes when we have arrays and structs
		auto Location = glGetAttribLocation( ProgramName, nameData.data() );
		Opengl::IsOkay( std::string("glGetAttribLocation( ") + Uniform.mName );
		Uniform.mIndex = Location;
		
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
		Opengl::IsOkay("glGetActiveUniform");
		
		Uniform.mArraySize = ArraySize;
		Uniform.mType = SoyGraphics::GetType( Type );
		Uniform.mName = std::string( nameData.data(), actualLength );
		
		//	refetch the real location. This changes when we have arrays and structs
		auto Location = glGetUniformLocation( ProgramName, nameData.data() );
		Opengl::IsOkay( std::string("glGetUniformLocation( ") + Uniform.mName );
		Uniform.mIndex = Location;
		
		//	some hardware returns things like
		//		Colours[0] or Colours[] for
		//	in vec4 Colours[1000];
		//	other data is good, so strip the name
		if ( Soy::StringContains( Uniform.mName, "[", true ) )
		{
			if ( DebugUniforms )
				std::Debug << "Stripping uniform name " << Uniform.mName;
			
			Uniform.mName = Soy::StringPopUntil( Uniform.mName, '[' );
			
			if ( DebugUniforms )
				std::Debug << " to " << Uniform.mName << std::endl;
		}
		//	todo: check is valid type etc
		mUniforms.PushBack( Uniform );
	}

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
	Opengl_IsOkayFlush();
	glUseProgram( mProgram.mName );
	Opengl::IsOkay("Opengl::TShader::Bind");

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


template<typename TYPE>
std::function<void(GLuint,GLint,GLsizei,const TYPE*)> GetglProgramUniformXv(SoyGraphics::TElementType::Type ElementType)
{
	static_assert( sizeof(TYPE) == -1, "this MUST be specialised" );
}


template<>
std::function<void(GLuint,GLint,GLsizei,const float*)> GetglProgramUniformXv<float>(SoyGraphics::TElementType::Type ElementType)
{
	auto glProgramUniform4x4fv = [](GLuint program,GLint location,GLsizei count,const GLfloat * value)
	{
		GLboolean transpose = GL_FALSE;
		glProgramUniformMatrix4fv( program, location, count, transpose, value );
	};
	
	switch ( ElementType )
	{
		case SoyGraphics::TElementType::Float:		return glProgramUniform1fv;
		case SoyGraphics::TElementType::Float2:		return glProgramUniform2fv;
		case SoyGraphics::TElementType::Float3:		return glProgramUniform3fv;
		case SoyGraphics::TElementType::Float4:		return glProgramUniform4fv;
		case SoyGraphics::TElementType::Float4x4:	return glProgramUniform4x4fv;
		default:break;
	}
	
	std::stringstream Error;
	Error << "Don't know which glUniformXv to use for  " << ElementType;
	throw Soy::AssertException(Error.str());
}


template<>
std::function<void(GLuint,GLint,GLsizei,const int32_t*)> GetglProgramUniformXv<int32_t>(SoyGraphics::TElementType::Type ElementType)
{
	switch ( ElementType )
	{
		case SoyGraphics::TElementType::Int:		return glProgramUniform1iv;
		case SoyGraphics::TElementType::Int2:		return glProgramUniform2iv;
		case SoyGraphics::TElementType::Int3:		return glProgramUniform3iv;
		case SoyGraphics::TElementType::Int4:		return glProgramUniform4iv;
		default:break;
	}
	
	std::stringstream Error;
	Error << "Don't know which glUniformXv to use for  " << ElementType;
	throw Soy::AssertException(Error.str());
}

template<>
std::function<void(GLuint,GLint,GLsizei,const uint32_t*)> GetglProgramUniformXv<uint32_t>(SoyGraphics::TElementType::Type ElementType)
{
	switch ( ElementType )
	{
		case SoyGraphics::TElementType::Uint:		return glProgramUniform1uiv;
		case SoyGraphics::TElementType::Uint2:		return glProgramUniform2uiv;
		case SoyGraphics::TElementType::Uint3:		return glProgramUniform3uiv;
		case SoyGraphics::TElementType::Uint4:		return glProgramUniform4uiv;
		default:break;
	}
	
	std::stringstream Error;
	Error << "Don't know which glUniformXv to use for  " << ElementType;
	throw Soy::AssertException(Error.str());
}

template<typename TYPE>
void SetUniform_Array(Opengl::TShader& This,const SoyGraphics::TUniform& Uniform,ArrayBridge<TYPE>& Floats)
{
	//	work out how many floats we're expecting
	auto FloatCount = Uniform.GetElementCount();
	if ( Floats.GetSize() != FloatCount )
	{
		std::stringstream Error;
		Error << __func__ << " expected " << FloatCount << " floats but got " << Floats.GetSize();
		throw Soy::AssertException(Error.str());
	}
	
	Opengl_IsOkayFlush();
	
	auto UniformIndex = size_cast<GLint>( Uniform.mIndex );
	auto ArraySize = size_cast<GLsizei>(Uniform.GetArraySize());
	auto pFloats = Floats.GetArray();
	auto glProgramUniformXv = GetglProgramUniformXv<TYPE>( Uniform.mType );
	auto ProgramName = This.mProgram.mName;
	
	//	for debugging, we can set individually elements
	static bool SetManually = false;
	if ( SetManually && ArraySize > 1 )
	{
		for ( int i=0;	i<Uniform.GetArraySize();	i++ )
		{
			std::stringstream UniformName;
			UniformName << Uniform.mName << "[" << i << "]";
			auto Location = glGetUniformLocation( ProgramName, UniformName.str().c_str() );
			Opengl::IsOkay( std::string("glGetUniformLocation( ") + UniformName.str() );
			
			std::Debug << UniformName.str() << " at uniform location " << Location << " (orig: " << UniformIndex << ")" << std::endl;
			
			auto ElementFloatCount = Uniform.GetElementVectorSize();
			auto* FloatsX = &pFloats[ i * ElementFloatCount ];
			glProgramUniformXv( ProgramName, Location, 1, FloatsX );
			
			std::stringstream Error;
			Error << "SetUniform( Array[" << i << ", location: " << Location << ": " << Uniform << ")";
			Opengl::IsOkay( Error.str() );
		}
	}
	else
	{
		glProgramUniformXv( ProgramName, UniformIndex, ArraySize, pFloats );
		try
		{
			Opengl::IsOkay("SetUniform");
		}
		catch(std::exception& e)
		{
			//	expensive!
			std::stringstream Error;
			Error << "SetUniform( " << Uniform << ")" << e.what();
			throw Soy::AssertException( Error.str() );
		}
	}
}

void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,ArrayBridge<float>&& Floats)
{
	SetUniform_Array<float>( *this, Uniform, Floats );
}

void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,ArrayBridge<int32_t>&& Floats)
{
	SetUniform_Array<int32_t>( *this, Uniform, Floats );
}

void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,ArrayBridge<uint32_t>&& Floats)
{
	SetUniform_Array<uint32_t>( *this, Uniform, Floats );
}



void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,bool Bool)
{
	auto UniformIndex = size_cast<GLint>( Uniform.mIndex );
	auto BoolValue = static_cast<GLint>(Bool);
	
	glProgramUniform1i( mProgram.mName, UniformIndex, BoolValue );
	Opengl::IsOkay("SetUniform(bool)");
}


void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,int32_t Integer)
{
	auto UniformIndex = size_cast<GLint>( Uniform.mIndex );
	auto IntValue = size_cast<GLint>(Integer);
	
	glProgramUniform1i( mProgram.mName, UniformIndex, IntValue );
	Opengl::IsOkay("SetUniform(int32_t)");
}


void Opengl::TShader::SetUniform(const SoyGraphics::TUniform& Uniform,const TTexture& Texture,size_t BindIndex)
{
	if ( Uniform.mType != SoyGraphics::TElementType::Texture2D )
		throw Soy::AssertException( std::string("Trying to set image on non-image uniform ") + Uniform.mName );
	
	
	auto SetUniformIndex = [&](GLuint TextureIndex)
	{
		auto UniformIndexInt = size_cast<GLint>( Uniform.mIndex );
		glProgramUniform1i( mProgram.mName, UniformIndexInt, TextureIndex );
		
		try
		{
			Opengl::IsOkay(__func__);
		}
		catch(std::exception& e)
		{
			std::stringstream Error;
			Error << "glProgramUniform1i( " << mProgram.mName << ", " << UniformIndexInt << ", " << TextureIndex << "; " << Uniform << " ) " << e.what();
			throw Soy::AssertException(Error.str());
		}
	};
	TShaderState::BindTexture( BindIndex, Texture, SetUniformIndex );
}

void Opengl::TGeometry::EnableAttribs(const SoyGraphics::TGeometryVertex& Descripton,bool Enable)
{
	auto& mElements = Descripton.mElements;
	for ( int at=0;	at<mElements.GetSize();	at++ )
	{
		auto& Element = mElements[at];
		auto AttribIndex = static_cast<GLint>(Element.mIndex);

		if ( Enable )
			glEnableVertexAttribArray( AttribIndex );
		else
			glDisableVertexAttribArray( AttribIndex );
		Opengl_IsOkay();
	}
}

Opengl::TGeometry::TGeometry(const ArrayBridge<uint8>&& Data,const ArrayBridge<uint32_t>&& Indexes,const SoyGraphics::TGeometryVertex& Vertex) :
	mVertexBuffer( GL_ASSET_INVALID ),
	mIndexBuffer( GL_ASSET_INVALID ),
	mVertexArrayObject( GL_ASSET_INVALID ),
	mVertexCount( GL_ASSET_INVALID ),
	mIndexCount( GL_ASSET_INVALID ),
	mIndexType(GL_ASSET_INVALID),
	mVertexDescription	( Vertex )
{
	Opengl_IsOkayFlush();
	
	bool GenerateIndexes = (Indexes.GetSize() > 0);
	
	//	fill vertex array
	Opengl::GenVertexArrays( 1, &mVertexArrayObject );
	Opengl::IsOkay( "TGeometry::TGeometry glGenVertexArrays" );
	
	glGenBuffers( 1, &mVertexBuffer );
	if ( GenerateIndexes )
		glGenBuffers( 1, &mIndexBuffer );
	Opengl::IsOkay( "TGeometry::TGeometry  glGenBuffers" );

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
		auto AttribIndex = static_cast<GLuint>(Element.mIndex);

		//std::Debug << "Pushing attrib " << AttribIndex << ", arraysize " << Element.mArraySize << ", stride " << Stride << std::endl;
		glEnableVertexAttribArray( AttribIndex );
		Opengl_IsOkay();

		//	https://www.opengl.org/sdk/docs/man/html/glVertexAttribPointer.xhtml
		//	must be 1,2,3 or 4
		auto ArraySize = size_cast<GLint>( Element.GetArraySize() );
		auto Type = GetType( Element.mType );

		glVertexAttribPointer( AttribIndex, ArraySize * Type.second, Type.first, Normalised, Stride, ElementPointer );
		Opengl_IsOkay();
	}
	
	//	gr: disabling vertex attribs stops rendering on ios
	//Vertex.DisableAttribs();

	if ( GenerateIndexes )
	{
		//	push indexes as glshorts
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, Indexes.GetDataSize(), Indexes.GetArray(), GL_STATIC_DRAW );
		mIndexCount = size_cast<GLsizei>( Indexes.GetSize() );
		//mIndexType = Opengl::GetTypeEnum<Indexes::TYPE>();
		mIndexType = Opengl::GetTypeEnum<uint32_t>();
		Opengl_IsOkay();
	}
	
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
	
	bool HasIndexes = ( mIndexBuffer != GL_ASSET_INVALID );
	
	if ( HasIndexes )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
		Opengl_IsOkay();
	}
	
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
	
	if ( !HasIndexes )
	{
		//	gr: shouldn't use this, only for debug
		glDrawArrays( GL_TRIANGLES, 0, this->mVertexCount );
		Opengl_IsOkay();
	}
	else
	{
		Opengl::IsOkay("pre glDrawElements");
		glDrawElements( GL_TRIANGLES, mIndexCount, mIndexType, nullptr );
		Opengl::IsOkay("glDrawElements");
	}
	
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
const std::initializer_list<GLenum> Opengl1ChannelFormats =
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

const std::initializer_list<GLenum> Opengl2ChannelFormats =
{
	GL_RG,
#if defined(GL_LUMINANCE_ALPHA)
	GL_LUMINANCE_ALPHA,
#endif
};


const std::initializer_list<GLenum> Opengl3ChannelFormats =
{
	//	gr: for openvr/steamvr, we're prioritising GL_RGBA8 over GL_RGBA
	GL_RGB,
#if defined(GL_RGB8)
	GL_RGB8
#endif
};

const std::initializer_list<GLenum> Opengl4ChannelFormats =
{
#if defined(GL_RGBA8)
	GL_RGBA8,
#endif
	GL_RGBA
};



const Array<TPixelFormatMapping>& Opengl::GetPixelFormatMap()
{
	static TPixelFormatMapping _PixelFormatMap[] =
	{
		TPixelFormatMapping( SoyPixelsFormat::RGB,			Opengl3ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::RGBA,			Opengl4ChannelFormats),

		//	gr: untested
		//	gr: use this with 8_8_8_REV to convert to BGRA!
		TPixelFormatMapping( SoyPixelsFormat::ARGB,			Opengl4ChannelFormats),
		
		TPixelFormatMapping( SoyPixelsFormat::Luma_Full,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::Luma_Ntsc,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::Luma_Smptec,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::Greyscale,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::ChromaUV_8_8,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::ChromaU_8,	Opengl1ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::ChromaV_8,	Opengl1ChannelFormats),

		//	note: this needs to align with GetGlPixelStorageFormat
		//		if GetGlPixelStorageFormat is 16bit, then this needs to be one channel
		TPixelFormatMapping( SoyPixelsFormat::ChromaUV_88,			Opengl2ChannelFormats),
		TPixelFormatMapping( SoyPixelsFormat::GreyscaleAlpha,		Opengl2ChannelFormats),
		//	one channel, 16 bit
		TPixelFormatMapping(SoyPixelsFormat::KinectDepth,			Opengl1ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::FreenectDepth10bit,	Opengl1ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::FreenectDepth11bit,	Opengl1ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Depth16mm,		Opengl1ChannelFormats),

		TPixelFormatMapping(SoyPixelsFormat::Uvy_844_Full,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Yuv_844_Full,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Yuv_844_Ntsc,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Yuv_844_Smptec,	Opengl2ChannelFormats),
		
		TPixelFormatMapping(SoyPixelsFormat::YYuv_8888_Full,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::YYuv_8888_Ntsc,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::YYuv_8888_Smptec,		Opengl2ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::uyvy,					Opengl2ChannelFormats),

		TPixelFormatMapping(SoyPixelsFormat::Yuv_8_8_8_Full,		Opengl1ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Yuv_8_8_8_Ntsc,		Opengl1ChannelFormats),
		TPixelFormatMapping(SoyPixelsFormat::Yuv_8_8_8_Smptec,		Opengl1ChannelFormats),

		
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
		
		TPixelFormatMapping( SoyPixelsFormat::Float1,		{GL_RED},	{GL_R32F} ),
		TPixelFormatMapping( SoyPixelsFormat::Float2,		{GL_RG},	{GL_RG32F} ),
		TPixelFormatMapping( SoyPixelsFormat::Float3,		{GL_RGB},	{GL_RGB32F}	),
		TPixelFormatMapping( SoyPixelsFormat::Float4,		{GL_RGBA},	{GL_RGBA32F} ),
		
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
		
		//	gr: should we ONLY be checking this (which may be empty)
		if ( FormatMap.mOpenglInternalFormats.Find(Format) )
			return FormatMap.mPixelFormat;

		if ( FormatMap.mOpenglFormats.Find(Format) )
			return FormatMap.mPixelFormat;
	}
	

	//std::Debug << "Failed to convert glpixelformat " << GetEnumString(Format) << " to soy pixel format" << std::endl;
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
		if ( Opengl::FenceSync )
		{
			mSyncObject = Opengl::FenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
			Opengl::IsOkay("glFenceSync");
		}
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
