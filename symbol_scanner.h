//
// Created by 王安然 on 15/8/7.
//

#ifndef DCODE_SYMBOL_SCANNER_H
#define DCODE_SYMBOL_SCANNER_H

#include "utils/utils.h"
#include "Physical.h"
#include "locator_scanner.h"

class Symbol_scanner{
private:
    Pixel_reader* reader_;
    Config* config_;
public:
    Symbol_scanner(Pixel_reader* reader);

    class Block_content{
    private:
        int vcnt_;
        int hcnt_;
    public:
        Lazy_mat<RGB> centered_;
        Lazy_mat<RGB> smoothed_;
        int get_vertical_symbol_count(){return vcnt_;}
        int get_horizontal_symbol_count(){return hcnt_;}
        RGB get_center_color(int x_id, int y_id){return centered_[x_id][y_id];}
        RGB get_smoothed_color(int x_id, int y_id){return smoothed_[x_id][y_id];}

        Block_content(int vcnt, int hcnt, std::function<RGB(int,int)> func_center, std::function<RGB(int,int)> func_smoothed):
                vcnt_(vcnt),hcnt_(hcnt),centered_(Lazy_mat<RGB>(hcnt,vcnt,func_center)),smoothed_(Lazy_mat<RGB>(hcnt,vcnt,func_smoothed)){}
    };

    struct Block_anchor{
        Locator_scanner::Locator_scanner_result
                *left_top,
                *right_top,
                *left_bottom,
                *right_bottom;

        Block_anchor(const Locator_scanner::Locator_scanner_result* _left_top, const Locator_scanner::Locator_scanner_result* _right_top,
                     const Locator_scanner::Locator_scanner_result* _left_bottom, const Locator_scanner::Locator_scanner_result* _right_bottom):
                left_top(_left_top),right_top(_right_top), left_bottom(_left_bottom), right_bottom(_right_bottom){}

        Block_content get_block_content(Pixel_reader* reader, int sidelength);
    };

    Block_content locate(Block_anchor anchor, int side_count);
    Mat<Option<Block_anchor>> get_anchors(Mat<Option<Locator_scanner::Locator_scanner_result>>& map);
};
#endif //DCODE_SYMBOL_SCANNER_H
