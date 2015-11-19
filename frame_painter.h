//
// Created by 王安然 on 15/11/18.
//

#ifndef DCODE_FRAME_PAINTER_H
#define DCODE_FRAME_PAINTER_H

#include "physical.h"
#include "structures.h"
#include "config.h"
#include "utils/constants.h"

class Frame_painter {
    static bool paint_to_screen(Screen_painter *painter, const Tx_frame &frame){
        auto& config=Config::current();
        auto width=config->hardware_config.tx_width;
        auto height=config->hardware_config.tx_height;
        auto sidelength=frame.get_block_ref(0,0).sidelength;
        auto vcount=frame.get_total_symbol_vertical_count_with_border(sidelength);
        auto hcount=frame.get_total_symbol_horizontal_count_with_border(sidelength);

        LOCALCONST(MARGIN_LEFT_BLOCK_RATIO,double);
        LOCALCONST(MARGIN_TOP_BLOCK_RATIO,double);

        auto& margin_left=MARGIN_LEFT_BLOCK_RATIO*width/(frame.horizontal_count+MARGIN_LEFT_BLOCK_RATIO*2);
        auto& margin_top=MARGIN_TOP_BLOCK_RATIO*height/(frame.vertical_count+MARGIN_TOP_BLOCK_RATIO*2);

        auto effect_width=width-2*margin_left;
        auto effect_height=height-2*margin_top;
        for(int i=0;i<vcount;i++)
            for(int j=0;j<hcount;j++){
                auto color=frame.get_symbol_at_with_border(j,i,sidelength);
                auto x=j*effect_width/hcount;
                auto y=i*effect_height/vcount;
                auto w=(j+1)*effect_width/hcount-x;
                auto h=(i+1)*effect_width/vcount-y;

                painter->fill(x,y,w,h,color);
            }

        return true;
    }
};
#endif //DCODE_FRAME_PAINTER_H
