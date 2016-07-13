#include "SoyMath.h"





float Soy::AngleDegDiff(float Angle,float Base)
{
	float Diff = Angle - Base;
	while ( Diff < -180.f )
		Diff += 360.f;
	while ( Diff > 180.f )
		Diff -= 360.f;
	return Diff;
}




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
Soy::TRgb::TRgb(const THsl& Hsl)
{
	auto& h = Hsl.h();
	auto& s = Hsl.s();
	auto& l = Hsl.l();
	auto& r = this->r();
	auto& g = this->g();
	auto& b = this->b();
	
	if(s == 0){
		r = g = b = l; // achromatic
	}else{
		float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
		float p = 2.f * l - q;
		r = hue2rgb(p, q, h + 1.f/3.f);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1.f/3.f);
	}
}


//	gr: copy of opencl code	http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion/9493060#9493060


Soy::THsl::THsl(const TRgb& rgb)
{
	float r = rgb.r();
	float g = rgb.g();
	float b = rgb.b();

	float Max = std::max( r, g, b );
	float Min = std::min( r, g, b );
	
	auto& h = this->h();
	auto& s = this->s();
	auto& l = this->l();
	h = 0;
	s = 0;
	l = ( Max + Min ) / 2.f;

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
}


float3x3 SoyMath::GetFlipMatrix3x3()
{
	//	y should be 1-y so -y + 1
	return float3x3(	1, 0, 0, 
						0, -1, 0, 
						0, 1, 0 );
}


uint32_t SoyMath::GetNextPower2(uint32_t x)
{
	//	from hackers delight.
	if ( x == 0 )
		return 0;

	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x+1;
}


bool SoyMath::IsPower2(uint32_t x)
{
	auto Powx = GetNextPower2(x);
	return Powx == x;
}

