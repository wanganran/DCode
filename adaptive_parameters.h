//
// Created by 王安然 on 15/10/21.
//

#ifndef DCODE_ADAPTIVE_PARAMETERS_H
#define DCODE_ADAPTIVE_PARAMETERS_H

#include <_types/_uint64_t.h>
#include "palette.h"
#include "structures.h"

struct Adaptive_parameters{
private:
    Adaptive_parameters(){}

public:

    uint64_t update_tick;
    int block_sidelength;
    std::unique_ptr<Palette::Matcher> palette_matcher;
    std::unique_ptr<Palette::Analyzer> palette_analyzer;
    FEC_level FEC_strength_primary;
    FEC_level FEC_strength_secondary;

    bool palette_analyzer_combine();

    Adaptive_parameters& get_global_by_parity(bool parity, bool& out_is_newest){
        static Adaptive_parameters singleton_odd;
        static Adaptive_parameters singleton_even;

        if(parity)
            out_is_newest=singleton_even.update_tick>singleton_odd.update_tick;
        else out_is_newest=singleton_even.update_tick<singleton_odd.update_tick;

        return parity?singleton_even:singleton_odd;
    }
};

#endif //DCODE_ADAPTIVE_PARAMETERS_H
