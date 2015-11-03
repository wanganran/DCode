//
// Created by Anran on 15/5/12.
// First version finished on 15/6/10.

#ifndef DCODE_PIXEL_READER_ANDROID_H
#define DCODE_PIXEL_READER_ANDROID_H

#include <cstdint>
#include "utils/RGB.h"
class Pixel_reader_Android{
friend class Physical;
private:
	const int width_;
	const int height_;
	const int size_;
	uint8_t* data_;

    Pixel_reader_Android(int w, int h, uint8_t* ptr):width_(w),height_(h),size_(w*h),data_(ptr){
    }
public:
	inline int get_width() const{
		return width_;
	}
	
	inline int get_height() const{
		return height_;
	}
	
	inline uint8_t get_brightness(int x, int y) const{
		return data_[y*width_ + x];
	}
	
	inline uint16_t get_color(int x, int y) const{
		int uvp = size_ + (y >> 1)*width_;
		int xa = x & 0xfffffffe;

		return *(uint16_t*)(data_ + uvp + xa);
	}
    inline RGB get_RGB(int x, int y) const{
        int Y=data_[y*width_+x]-16;
        if(Y<0)Y=0;

        int uvp = size_ + (y >> 1)*width_;
        int UVindex=uvp+(x&0xfffffffe);
        int V=data_[UVindex]-128;
        int U=data_[UVindex+1]-128;
        Y = 1192 * Y;
        int r = (Y + 1634 * V);
        int g = (Y - 833 * V - 400 * U);
        int b = (Y + 2066 * U);

        if(r>262143)r=262143;
        if(g>262143)g=262143;
        if(b>262143)b=262143;
        if(r<0)r=0;
        if(g<0)g=0;
        if(b<0)b=0;

        return RGB((uint8_t)(r>>10),(uint8_t)(g>>10),(uint8_t)(b>>10));
    }
    inline RGB get_smoothed_RGB(int x, int y) const{
        if(x==0)x=1;
        if(y==0)y=1;
        if(x==width_-1)x=width_-2;
        if(y==height_-1)y=height_-2;
        int r=0,g=0,b=0;
        for(int i=-1;i<=1;i++)
            for(int j=-1;j<=1;j++) {
                RGB c = get_RGB(x - 1, y - 1);
                r += c.R;
                g += c.G;
                b += c.B;
            }
        return RGB(r/9,g/9,b/9);
    }
};
typedef Pixel_reader_Android Pixel_reader;
#endif