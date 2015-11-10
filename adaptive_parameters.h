//
// Created by 王安然 on 15/10/21.
//

#ifndef DCODE_ADAPTIVE_PARAMETERS_H
#define DCODE_ADAPTIVE_PARAMETERS_H

#include <_types/_uint64_t.h>
#include "palette.h"
#include "structures.h"

struct Rx_adaptive_parameters{
private:
    Rx_adaptive_parameters(){
        //TODO: initial configurations
    }

public:

    uint64_t update_tick;
    int block_sidelength;
    std::unique_ptr<Palette::Matcher> palette_matcher;
    std::unique_ptr<Palette::Analyzer> palette_analyzer;
    FEC_level FEC_strength_primary;
    FEC_level FEC_strength_secondary;

    bool palette_analyzer_combine();

    static Rx_adaptive_parameters& get_global_by_parity(bool parity, bool& out_is_newest){
        static Rx_adaptive_parameters singleton_odd;
        static Rx_adaptive_parameters singleton_even;

        if(parity)
            out_is_newest=singleton_even.update_tick>singleton_odd.update_tick;
        else out_is_newest=singleton_even.update_tick<singleton_odd.update_tick;

        return parity?singleton_even:singleton_odd;
    }
    static Rx_adaptive_parameters& get_newest(bool& out_parity){
        bool newest;
        auto& odd=get_global_by_parity(true, newest);
        if(newest){
            out_parity=true;
            return odd;
        }
        else{
            out_parity=false;
            return get_global_by_parity(false, newest);
        }
    }
};

struct Tx_adaptive_parameters{
private:
    Tx_adaptive_parameters():has_next(false){
        //TODO: initial configurations
    };

    uint8_t next_block_sidelength;
    uint8_t next_color_sec_mask;
    FEC_level  next_primary, next_secondary;
    bool has_next;


public:
    bool shift_next(){
        if(has_next){
            block_sidelength=next_block_sidelength;
            color_sec_mask=next_color_sec_mask;
            FEC_strength_primary=next_primary;
            FEC_strength_secondary=next_secondary;
            parity=!parity;
            has_next=false;
            return true;
        }
        return false;
    }

    uint8_t block_sidelength;
    uint8_t color_sec_mask;
    FEC_level FEC_strength_primary;
    FEC_level FEC_strength_secondary;
    bool parity;

    void update_next_frame(uint8_t block_sidelength, uint8_t color_sec_mask, FEC_level FEC_pri, FEC_level FEC_sec){
        next_block_sidelength=block_sidelength;
        next_color_sec_mask=color_sec_mask;
        next_primary=FEC_pri;
        next_secondary=FEC_sec;
        has_next=true;
    }

    static Tx_adaptive_parameters& current(){
        static Tx_adaptive_parameters singleton;
    }
};

#endif //DCODE_ADAPTIVE_PARAMETERS_H
