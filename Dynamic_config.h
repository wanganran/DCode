//
// Created by 王安然 on 15/9/29.
//

#ifndef DCODE_DYNAMIC_CONFIG_H
#define DCODE_DYNAMIC_CONFIG_H

#include "utils/utils.h"
#include "palette.h"
#include "PHY_action.h"

class Dynamic_config{
private:

public:


    Dynamic_config(const Dynamic_config& def);
    Option<bool> push_action(const PHY_action& action);

    //layout related
    int get_block_size(bool parity);
    bool adjust_color_palette(bool parity, Palette::Adjuster* adjuster);

    //error correction related
    enum class FEC_LEVEL:double{
        HIGH=0.90,
        MEDIUM=0.75,
        LOW=0.5
    };
    FEC_LEVEL get_FEC_level(bool parity);



    static Dynamic_config& default_config();
};

#endif //DCODE_DYNAMIC_CONFIG_H
