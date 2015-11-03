//
// Created by Anran on 15/6/15.
//

#ifndef DCODE_RGB_H
#define DCODE_RGB_H
#include <cstdint>
#include <string>
#include <stdio.h>

struct RGB{
    static const int SECONDARY_GAP=60;
    uint8_t R;
    uint8_t G;
    uint8_t B;

    RGB(uint8_t r,uint8_t g, uint8_t b):R(r),G(g),B(b){}
    RGB():R(0),G(0),B(0){}
    RGB(const RGB& color):R(color.R),G(color.G),B(color.B){}
    RGB(uint8_t data):R(data&4?255:0),
                      G(data&2?255:0),
                      B(data&1?255:0){}
    void set_secondary(uint8_t mask, uint8_t data){
        if(mask&1) {
            if (B > 128)B = (data & 1) ? 255 : (255 - SECONDARY_GAP);
            else B = (data & 1) ? SECONDARY_GAP : 0;
        }
        if(mask&2) {
            if (G > 128)G = (data & 2) ? 255 : (255 - SECONDARY_GAP);
            else G = (data & 2) ? SECONDARY_GAP : 0;
        }
        if(mask&4) {
            if (R > 128)R = (data & 4) ? 255 : (255 - SECONDARY_GAP);
            else R = (data & 4) ? SECONDARY_GAP : 0;
        }
    }
    std::string to_string(){
        char buffer[50];
        sprintf(buffer,"%d, %d, %d",R,G,B);
        return std::string(buffer);
    }
    static RGB get_max(){return RGB(255,255,255);}
    static RGB get_min(){return RGB(0,0,0);}
};

#endif //DCODE_RGB_H
