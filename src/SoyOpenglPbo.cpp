#include "SoyOpenglPbo.h"


//	gr: this is being used as a minimal include at the moment
//		to ease GLES3 on linux vs normal opengl
//		PBO is all I need to go with sokol, so currently IsOkay etc is ere

void Opengl::GetReadPixelsFormats(ArrayBridge<GLenum> &&Formats)
{
	Formats.SetSize(5);
	
	Formats[0] = GL_INVALID_VALUE;
	#if defined(GL_RED)
	Formats[1] = GL_RED;
	#else
	Formats[1] = GL_LUMINANCE;//GL_ALPHA;	//	gr: neither of these seem to work on ios, ES3, only GL_RED
	#endif
	Formats[2] = GL_INVALID_VALUE;
	Formats[3] = GL_RGB;
	Formats[4] = GL_RGBA;
}

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

//	gr: turn this into SoyPixelsImpl* return
const uint8* Opengl::TPbo::LockBuffer()
{
	Bind();
#if OPENGL_ES==3
	GLintptr Offset = 0;
	GLsizeiptr Length = mMeta.GetDataSize();
	GLbitfield Access = GL_MAP_READ_BIT;
	auto* Buffer = glMapBufferRange( GL_PIXEL_PACK_BUFFER, Offset, Length, Access );
#elif defined(OPENGL_ES)
	//	gr: come back to this... when needed, I think it's supported
	const uint8* Buffer = nullptr;
#else
	Bind();
	auto* Buffer = glMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY );
#endif

	Opengl_IsOkay();
	return reinterpret_cast<const uint8*>( Buffer );
}

void Opengl::TPbo::UnlockBuffer()
{
#if OPENGL_ES==3
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
#elif defined(OPENGL_ES)
	throw Soy::AssertException("Lock buffer should not have succeeded on ES");
#else
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
#endif
	Opengl_IsOkay();
	Unbind();
}




