#include "SoyOpengl.h"



namespace Opengl
{
	const char*		ErrorToString(GLenum Error);
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

GlProgram BuildProgram( const char * vertexSrc,
					   const char * fragmentSrc,std::ostream& Error )
{
	GlProgram prog;

	bool Success = true;
	prog.vertexShader = glCreateShader( GL_VERTEX_SHADER );
	if ( !CompileShader( prog.vertexShader, vertexSrc, "Vertex Shader", Error ) )
	{
		Success = false;
	}
	
	prog.fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	if ( !CompileShader( prog.fragmentShader, fragmentSrc, "Fragment Shader", Error ) )
	{
		Success = false;
	}
	
	if ( !Success )
		return GlProgram();
	
	prog.program = glCreateProgram();
	glAttachShader( prog.program, prog.vertexShader );
	glAttachShader( prog.program, prog.fragmentShader );
	
	// set attributes before linking
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_POSITION,			"Position" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_NORMAL,			"Normal" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_TANGENT,			"Tangent" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_BINORMAL,			"Binormal" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_COLOR,			"VertexColor" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_UV0,				"TexCoord" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_UV1,				"TexCoord1" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	"JointWeights" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES,	"JointIndices" );
	glBindAttribLocation( prog.program, VERTEX_ATTRIBUTE_LOCATION_FONT_PARMS,		"FontParms" );
	
	// link and error check
	glLinkProgram( prog.program );
	GLint r;
	glGetProgramiv( prog.program, GL_LINK_STATUS, &r );
	if ( r == GL_FALSE )
	{
		Error << "Linking vertex & fragment shader failed: ";
		GLchar msg[1024] = {0};
		glGetProgramInfoLog( prog.program, sizeof( msg ), 0, msg );
		Error << msg;
		return GlProgram();
	}
	prog.uMvp = glGetUniformLocation( prog.program, "Mvpm" );
	prog.uModel = glGetUniformLocation( prog.program, "Modelm" );
	prog.uView = glGetUniformLocation( prog.program, "Viewm" );
	prog.uColor = glGetUniformLocation( prog.program, "UniformColor" );
	prog.uFadeDirection = glGetUniformLocation( prog.program, "UniformFadeDirection" );
	prog.uTexm = glGetUniformLocation( prog.program, "Texm" );
	prog.uTexm2 = glGetUniformLocation( prog.program, "Texm2" );
	prog.uJoints = glGetUniformLocation( prog.program, "Joints" );
	prog.uColorTableOffset = glGetUniformLocation( prog.program, "ColorTableOffset" );
	
	glUseProgram( prog.program );
	
	// texture and image_external bindings
	for ( int i = 0; i < 8; i++ )
	{
		char name[32];
		sprintf( name, "Texture%i", i );
		const GLint uTex = glGetUniformLocation( prog.program, name );
		if ( uTex != -1 )
		{
			glUniform1i( uTex, i );
		}
	}
	
	glUseProgram( 0 );
	
	return prog;
}

void DeleteProgram( GlProgram & prog )
{
	if ( prog.program != 0 )
	{
		glDeleteProgram( prog.program );
	}
	if ( prog.vertexShader != 0 )
	{
		glDeleteShader( prog.vertexShader );
	}
	if ( prog.fragmentShader != 0 )
	{
		glDeleteShader( prog.fragmentShader );
	}
	prog.program = 0;
	prog.vertexShader = 0;
	prog.fragmentShader = 0;
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
		case GL_STACK_UNDERFLOW:	return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:		return "GL_STACK_OVERFLOW";
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

bool Opengl::TTexture::IsValid() const
{
	if ( !Opengl::IsInitialised(__func__,false) )
		return false;
	
	//	gr: this is returning false from other threads :/
	//	other funcs are working though
	auto IsTexture = glIsTexture( mTexture.mName );
	if ( IsTexture )
		return true;
	
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


// Returns false and logs the ShaderInfoLog on failure.
bool CompileShader( const Opengl::TAsset& shader, const char * src )
{
	if ( !Opengl_IsInitialised() )
		return false;
	
	glShaderSource( shader.mName, 1, &src, 0 );
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


bool Opengl::GlProgram::IsValid() const
{
	return program.IsValid();
}


Opengl::GlProgram Opengl::BuildProgram( const char * vertexSrc,
									   const char * fragmentSrc )
{
	if ( !Opengl_IsInitialised() )
		return GlProgram();
	
	GlProgram prog;
	
	prog.vertexShader.mName = glCreateShader( GL_VERTEX_SHADER );
	if ( !CompileShader( prog.vertexShader, vertexSrc ) )
	{
		Soy::Assert( false, "Failed to compile vertex shader" );
		return GlProgram();
	}
	prog.fragmentShader.mName = glCreateShader( GL_FRAGMENT_SHADER );
	if ( !CompileShader( prog.fragmentShader, fragmentSrc ) )
	{
		Soy::Assert( false, "Failed to compile fragment shader" );
		return GlProgram();
	}
	
	prog.program.mName = glCreateProgram();
	auto& ProgramName = prog.program.mName;
	glAttachShader( ProgramName, prog.vertexShader.mName );
	glAttachShader( ProgramName, prog.fragmentShader.mName );
	
	// set attributes before linking
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_POSITION,			"Position" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_NORMAL,			"Normal" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_TANGENT,			"Tangent" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_BINORMAL,			"Binormal" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_COLOR,			"VertexColor" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_UV0,				"TexCoord" );
	glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_UV1,				"TexCoord1" );
	//glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	"JointWeights" );
	//glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES,	"JointIndices" );
	//glBindAttribLocation( ProgramName, VERTEX_ATTRIBUTE_LOCATION_FONT_PARMS,		"FontParms" );
	
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
	prog.uMvp = glGetUniformLocation( ProgramName, "Mvpm" );
	prog.uModel = glGetUniformLocation( ProgramName, "Modelm" );
	prog.uView = glGetUniformLocation( ProgramName, "Viewm" );
	prog.uColor = glGetUniformLocation( ProgramName, "UniformColor" );
	prog.uFadeDirection = glGetUniformLocation( ProgramName, "UniformFadeDirection" );
	prog.uTexm = glGetUniformLocation( ProgramName, "Texm" );
	prog.uTexm2 = glGetUniformLocation( ProgramName, "Texm2" );
	prog.uJoints = glGetUniformLocation( ProgramName, "Joints" );
	prog.uColorTableOffset = glGetUniformLocation( ProgramName, "ColorTableOffset" );
	
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
	
	return prog;
}

void Opengl::DeleteProgram( GlProgram & prog )
{
	if ( prog.program.mName != GL_ASSET_INVALID )
	{
		glDeleteProgram( prog.program.mName );
		prog.program.mName = GL_ASSET_INVALID;
	}
	if ( prog.vertexShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( prog.vertexShader.mName );
		prog.vertexShader.mName = GL_ASSET_INVALID;
	}
	if ( prog.fragmentShader.mName != GL_ASSET_INVALID )
	{
		glDeleteShader( prog.fragmentShader.mName );
		prog.fragmentShader.mName = GL_ASSET_INVALID;
	}
}


/*
void Opengl::TGeometry::Create( const VertexAttribs & attribs, const std::vector< TriangleIndex > & indices )
{
	if ( !Opengl_IsInitialised() )
		return;
	
	vertexCount = attribs.position.size();
	indexCount = indices.size();
	
	std::Debug << "glGenBuffers" << std::endl;
	glGenBuffers( 1, &vertexBuffer );
	glGenBuffers( 1, &indexBuffer );
	
	std::Debug << "glGenVertexArraysOES_" << std::endl;
	glGenVertexArraysOES_( 1, &vertexArrayObject );
	std::Debug << "glBindVertexArrayOES_" << std::endl;
	glBindVertexArrayOES_( vertexArrayObject );
	std::Debug << "glBindBuffer" << std::endl;
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
	glBindVertexArrayOES_( 0 );
	
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
}

void Opengl::TGeometry::Update( const VertexAttribs & attribs )
{
	vertexCount = attribs.position.size();
	
	glBindVertexArrayOES_( vertexArrayObject );
	
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

void Opengl::TGeometry::Draw() const
{
	if ( !Opengl_IsInitialised() )
		return;
	
	glBindVertexArrayOES_( vertexArrayObject );
	glDrawElements( GL_TRIANGLES, indexCount, ( sizeof( TriangleIndex ) == 2 ) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, NULL );
}

void Opengl::TGeometry::Free()
{
	glDeleteVertexArraysOES_( 1, &vertexArrayObject );
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
*/


