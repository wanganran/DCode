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
    Rx_adaptive_parameters(bool p):parity(p),update_tick(p?0:1),
                             block_sidelength(12),
                             color_sec_mask(0),
                             FEC_strength_secondary(FEC_level::MID),
                             FEC_strength_primary(FEC_level::MID),
                             palette_matcher(nullptr),
    {
    }

    int64_t update_tick;
public:
    const bool parity;
    int block_sidelength;
    uint8_t color_sec_mask;
    std::unique_ptr<Palette::Matcher> palette_matcher;
    FEC_level FEC_strength_primary;
    FEC_level FEC_strength_secondary;

    std::vector<int> disabled_blocks;

    //for each frame/probe/..., call this function to adjust the palette_matcher by the analysis from palette_analyzer, and clear the palette_analyzer for next use.
    bool palette_analyzer_combine(){
        //TODO
    }

    static Rx_adaptive_parameters& get_global_by_parity(bool parity, bool& out_is_newest){
        static Rx_adaptive_parameters singleton_even(false);
        static Rx_adaptive_parameters singleton_odd(true);

        if(parity)
            out_is_newest=singleton_even.update_tick<singleton_odd.update_tick;
        else out_is_newest=singleton_even.update_tick>=singleton_odd.update_tick;

        return parity?singleton_odd:singleton_even;
    }
    static Rx_adaptive_parameters& get_newest(bool& out_parity){
        bool newest;
        auto& even=get_global_by_parity(true, newest);
        if(newest){
            out_parity=true;
            return even;
        }
        else{
            out_parity=false;
            return get_global_by_parity(false, newest);
        }
    }
    static Rx_adaptive_parameters& get_older(){
        bool out_p;
        Rx_adaptive_parameters& r=get_newest(out_p);
        return get_global_by_parity(!out_p,out_p);
    }
    static void swap_and_update(){
        Rx_adaptive_parameters& older=get_older();
        older.update_tick=get_current_millis();
    }
};

struct Tx_adaptive_parameters{
private:
    Tx_adaptive_parameters():has_next(false),
                             block_sidelength(12),
    color_sec_mask(0),
    FEC_strength_primary(FEC_level::MID),
    FEC_strength_secondary(FEC_level::MID),
    parity(false)
    {
    };

    uint8_t next_block_sidelength;
    uint8_t next_color_sec_mask;
    FEC_level  next_primary, next_secondary;
    bool has_next;

    std::vector<int> next_disabled_blocks;

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

    std::vector<int> disabled_blocks;

    void update_next_frame(uint8_t block_sidelength, uint8_t color_sec_mask, FEC_level FEC_pri, FEC_level FEC_sec, const std::vector<int>& disabled_blocks){
        next_block_sidelength=block_sidelength;
        next_color_sec_mask=color_sec_mask;
        next_primary=FEC_pri;
        next_secondary=FEC_sec;
        next_disabled_blocks=disabled_blocks;
        has_next=true;
    }

    static Tx_adaptive_parameters& current(){
        static Tx_adaptive_parameters singleton;
    }
};

#endif //DCODE_ADAPTIVE_PARAMETERS_H
