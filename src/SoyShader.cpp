#include "SoyShader.h"
#include "Array.hpp"
#include "SoyString.h"



std::map<OpenglShaderVersion::Type,std::string> OpenglShaderVersion::EnumMap =
{
	{ OpenglShaderVersion::Invalid,	"" },
	{ OpenglShaderVersion::glsl300,	"#version 300 es" },
	{ OpenglShaderVersion::glsl150,	"#version 150 core" },
};


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
void UpgradeShader(ArrayBridge<std::string>& Shader,OpenglShaderVersion::Type Version)
{
	//	insert version if there isn't one there
	if ( !Soy::StringBeginsWith(Shader[0],"#version",true) )
	{
		auto VersionString = OpenglShaderVersion::ToString( Version );
		Shader.InsertAt( 0, VersionString );
	}
	
#if defined(TARGET_IOS)
	if ( Version == OpenglShaderVersion::glsl300 )
	{
		//	ios requires precision
		//	gr: add something to check if this is already declared
		Shader.InsertAt( GetNonProcessorFirstLine(Shader), "precision highp float;\n" );
	}
#endif
	
}

void SoyShader::Opengl::UpgradeVertShader(ArrayBridge<std::string>&& Shader,OpenglShaderVersion::Type Version)
{
	PreprocessShader( Shader );
	UpgradeShader( Shader, Version );
	
	if ( Version != OpenglShaderVersion::Invalid )
	{
		//	in 3.2, attribute/varying is now in/out
		//			varying is Vert OUT, and INPUT for a frag (it becomes an attribute of the pixel)
		Soy::StringReplace( Shader, "attribute", "in" );
		Soy::StringReplace( Shader, "varying", "out" );
	}
}


void SoyShader::Opengl::UpgradeFragShader(ArrayBridge<std::string>&& Shader,OpenglShaderVersion::Type Version)
{
	PreprocessShader( Shader );
	UpgradeShader( Shader, Version );
	
	if ( Version != OpenglShaderVersion::Invalid )
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

