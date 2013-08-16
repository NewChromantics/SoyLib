#include "SoyMath.h"




float hue2rgb(float p,float q,float t)
{
    while(t < 0) t += 1.f;
    while(t > 1) t -= 1.f;
    if(t < 1.f/6.f) return p + (q - p) * 6.f * t;
    if(t < 1.f/2.f) return q;
    if(t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
    return p;
}

//	http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion/9493060#9493060
ofColour GetRgb(float h,float s,float l)
{
    float r, g, b;

    if(s == 0){
        r = g = b = l; // achromatic
    }else{
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2.f * l - q;
        r = hue2rgb(p, q, h + 1.f/3.f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.f/3.f);
    }

    return ofColour( r * 255, g * 255, b * 255 );
}

//	gr: copy of opencl code	http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion/9493060#9493060
vec3f GetHsl(const ofColour& Rgba)
{
	float r = Rgba.r / 255.f;
	float g = Rgba.g / 255.f;
	float b = Rgba.b / 255.f;

	float Max = ofMax( r, ofMax( g, b ) );
	float Min = ofMin( r, ofMin( g, b ) );
    
	float h = 0;
	float s = 0;
	float l = ( Max + Min ) / 2.f;

	if ( Max == Min )
	{
		//	achromatic/grey
        h = s = 0; 
    }
	else
	{
        float d = Max - Min;
        s = l > 0.5f ? d / (2 - Max - Min) : d / (Max + Min);
        if ( Max == r )
		{
            h = (g - b) / d + (g < b ? 6 : 0);
		}
		else if ( Max == g )
		{
            h = (b - r) / d + 2;
        }
		else //if ( Max == b )
		{
			h = (r - g) / d + 4;
		}

        h /= 6;
    }

	return vec3f( h, s, l );
}

TColourHsl::TColourHsl(const ofColour& Rgb) :
	mHsl	( ::GetHsl( Rgb ) )
{

}

ofColour TColourHsl::GetRgb() const
{
	return ::GetRgb( GetHue(), GetSaturation(), GetLightness() );
}
