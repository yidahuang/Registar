#ifndef SET_COLOR_H
#define SET_COLOR_H

#include <vector>

struct RGB_
{
	unsigned char r; // 0 ~ 255
	unsigned char g; // 0 ~ 255
	unsigned char b; // 0 ~ 255
	RGB_(){r = 0; g = 0; b = 0;}
};

struct HSV
{
	float h; // 0 ~ 359.99f
	float s; // 0 ~ 1.0f
	float v; // 0 ~ 1.0f
	HSV(){h = 0; s = 0; v = 0;}
};

RGB_ hsv2rgb(const HSV &_hsv);

HSV rgb2hsv(const RGB_ &_rgb);

std::vector<RGB_> generateUniformColors(int _n, float start_degree = 0, float end_degree = 359.99f);

#endif 
