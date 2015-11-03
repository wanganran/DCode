//
// Created by 王安然 on 15/8/6.
//

#include "utils/RGB.h"
#include "screen_painter_Android.h"

inline static void rgb24(RGB color, uint8_t* ptr, int x, int y, int width, int height){
    int id=y*width+x;
    ptr[id*3]=color.R;
    ptr[id*3+1]=color.G;
    ptr[id*3+2]=color.B;
}

inline static void rgb565(RGB color, uint8_t* ptr, int x, int y, int width, int height){
    int id=y*width+x;
    *((uint16_t*)(ptr+id*2))=(((color.R&0xf8)<<8)|((color.G&0xfc)<<5)|(color.B>>3));
}

inline static uint8_t _limit(int x){
    if(x>255)return 255;
    if(x<0)return 0;
    return x;
}

inline static void yuv(RGB color, uint8_t* ptr, int x, int y, int width, int height){
    int Y=306*color.R+601*color.G+117*color.B;
    int U=-151*color.R-296*color.G+446*color.B;
    int V=630*color.R-527*color.G-102*color.B;

    int id=y*width+x;
    ptr[id]=_limit(Y<<10);
    int uvp=width*height+(y>>1)*width;
    int UVindex=uvp+(x&0xfffffffe);
    ptr[UVindex]=_limit((V<<10)+128);
    ptr[UVindex+1]=_limit((U<<10)+128);
}

#define PAINT(c,p,x,y,w,h) rgb24(c,p,x,y,w,h)

void Screen_painter_Android::paint(int x, int y, RGB color) {
    if(x<0||y<0)return;
    PAINT(color,ptr_,x,y,width_,height_);
}

void Screen_painter_Android::fill(int x, int y, int w, int h, RGB color) {
    if(x<0)x=0;
    if(y<0)y=0;
    if(x+w>width_)w=width_-x;
    if(y+h>height_)h=height_-y;
    for(int i=x;i<x+w;i++)
        for(int j=y;j<y+h;j++)
            PAINT(color,ptr_,i,j,width_,height_);
}

//currently not implemented.
RGB Screen_painter_Android::get(int x, int y) {
    return RGB();
}
