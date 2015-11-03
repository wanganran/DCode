//
// Created by 王安然 on 15/8/6.
//

#ifndef DCODE_SCREEN_PAINTER_ANDROID_H
#define DCODE_SCREEN_PAINTER_ANDROID_H

#include "utils/RGB.h"

class Physical;

//naive screen painter, just fill the pixels.
class Screen_painter_Android{
    friend class Physical;
private:
    uint8_t* const ptr_;
    int width_,height_;
    Screen_painter_Android(uint8_t* ptr, int width, int height):
            ptr_(ptr),width_(width),height_(height){}
public:
    inline void paint(int x, int y, RGB color);
    inline void fill(int x, int y, int w, int h, RGB color);
    inline RGB get(int x, int y);
};

#endif //DCODE_SCREEN_PAINTER_ANDROID_H
