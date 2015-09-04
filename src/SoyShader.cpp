#include "SoyShader.h"
#include "Array.hpp"
#include "SoyString.h"
#include "SoyOpengl.h"


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
		{
			if ( !Soy::StringBeginsWith( Line, "#define", true ) )
				IsHeader = true;
		}
		
		if ( Soy::StringBeginsWith(Line,"precision ",true) )
			IsHeader = true;
		
		if ( !IsHeader )
			continue;
		
		LastProcessorDirectiveLine = i;
	}
	return LastProcessorDirectiveLine+1;
}


//	change this in the chain so pre-process and #include is done much earlier in the chain
//	see my opencl code for a working example!
void PreprocessShader(ArrayBridge<std::string>& Shader)
{
	for ( int i=0;	i<Shader.GetSize();	i++ )
	{
		auto& Line = Shader[i];
		
		//	take include file, load & pre-process, and insert
		if ( Soy::StringBeginsWith( Line, "#include", true ) )
			throw Soy::AssertException("#includes are still todo");
		
		//	convert types
		//	todo: replace with regex for something more robust
		Soy::StringReplace( Line, "float4", "vec4" );
		Soy::StringReplace( Line, "float3", "vec3" );
		Soy::StringReplace( Line, "float2", "vec2" );
	}
}

//	vert & frag changes
void UpgradeShader(ArrayBridge<std::string>& Shader,Soy::TVersion Version)
{
	//	insert version if there isn't one there
	if ( !Soy::StringBeginsWith(Shader[0],"#version",true) )
	{
		std::string Profile;
		
#if defined(OPENGL_ES_2) || defined(OPENGL_ES_3)
		//	don't specificy a profile for 1.0 (es2)
		Profile = "es";
		if ( Version <= Soy::TVersion(1,0) )
			Profile = "";
#elif defined(OPENGL_CORE_3) || defined(OPENGL_CORE_2)
		//	gr: for 120, no profile specified, should default to "core"
#else
#error Unknown opengl type
#endif
		std::stringstream VersionString;
		VersionString << "#version " << Version.GetHundred() << " " << Profile << "\n";
	
		Shader.InsertAt( 0, VersionString.str() );
	}

	//	glsl over a certain version doesn't allow the precision specifiers
	//	gr: osx, maybe a desktop only thing?
	if ( Version >= Soy::TVersion(1,20) )
	{
		Soy::StringReplace( Shader, "highp", "" );
		Soy::StringReplace( Shader, "mediump", "" );
		Soy::StringReplace( Shader, "lowp", "" );
	}
	
	//	all versions of IOS (es?) require a precision specifier
#if defined(TARGET_IOS)
	//	gr: add something to check if this is already declared
	//	gr: needed vec2 declarations for GLSL 100... not for gles3?
	Shader.InsertAt( GetNonProcessorFirstLine(Shader), "precision highp float;\n" );
#endif
	
}

void SoyShader::Opengl::UpgradeVertShader(ArrayBridge<std::string>&& Shader,Soy::TVersion Version)
{
	PreprocessShader( Shader );
	UpgradeShader( Shader, Version );
	
	if ( Version >= Soy::TVersion(3,00) )
	{
		//	in 3.2, attribute/varying is now in/out
		//			varying is Vert OUT, and INPUT for a frag (it becomes an attribute of the pixel)
		Soy::StringReplace( Shader, "attribute", "in" );
		Soy::StringReplace( Shader, "varying", "out" );
	}
}


void SoyShader::Opengl::UpgradeFragShader(ArrayBridge<std::string>&& Shader,Soy::TVersion Version)
{
	PreprocessShader( Shader );
	UpgradeShader( Shader, Version );
	
	if ( Version >= Soy::TVersion(3,00) )
	{
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
	}
}

