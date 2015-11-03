//
// Created by 王安然 on 15/8/7.
//

#include <math.h>
#include <algorithm>
#include <queue>
#include "symbol_scanner.h"
#include "utils/constants.h"

using namespace std;

Symbol_scanner::Symbol_scanner(Pixel_reader *reader) {
    this->reader_=reader;
    this->config_=Config::current();
}


static inline double __len(int x1, int y1, int x2, int y2){
    return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}
#if 0
Option<int> Symbol_scanner::guess_side_count(Block_anchor anchors) {

    int result=0;

    int possible_size_c=sizeof(config_->barcode_config.supported_block_size)/sizeof(config_->barcode_config.supported_block_size[0]);

    double best=0.0;

    for(int size_i=0;size_i<possible_size_c;size_i++){
        int size=config_->barcode_config.supported_block_size[size_i];
        double y_ratio=
                __len(anchors.left_top->center_x,anchors.left_top->center_y,
                      anchors.left_bottom->center_x,anchors.left_bottom->center_y)/
                __len(anchors.right_top->center_x,anchors.right_top->center_y,
                      anchors.right_bottom->center_x,anchors.right_bottom->center_y);
        double x_ratio=
                __len(anchors.left_top->center_x,anchors.left_top->center_y,
                      anchors.right_top->center_x,anchors.right_top->center_y)/
                __len(anchors.left_bottom->center_x,anchors.left_bottom->center_y,
                      anchors.right_bottom->center_x,anchors.right_bottom->center_y);
        double k_x=pow(y_ratio, 1.0/size);
        double k_y=pow(x_ratio, 1.0/size);

        double left_top_x_should=__len(anchors.left_top->center_x,anchors.left_top->center_y,
                                       anchors.right_top->center_x,anchors.right_top->center_y)*
                                       (1-pow(k_x,size))/(1-k_x);
        double left_top_y_should=__len(anchors.left_top->center_x,anchors.left_top->center_y,
                                       anchors.left_bottom->center_x,anchors.left_bottom->center_y)*
                                       (1-pow(k_y,size))/(1-k_y);
        double right_top_x_should=__len(anchors.left_top->center_x,anchors.left_top->center_y,
                                       anchors.right_top->center_x,anchors.right_top->center_y)*
                                       (1-pow(k_x,-size))/(1-1/k_x);
        double right_top_y_should=__len(anchors.right_top->center_x,anchors.right_top->center_y,
                                       anchors.right_bottom->center_x,anchors.right_bottom->center_y)*
                                       (1-pow(k_y,size))/(1-k_y);

        double left_bottom_x_should=__len(anchors.left_bottom->center_x,anchors.left_bottom->center_y,
                                       anchors.right_bottom->center_x,anchors.right_bottom->center_y)*
                                       (1-pow(k_x,size))/(1-k_x);
        double left_bottom_y_should=__len(anchors.left_top->center_x,anchors.left_top->center_y,
                                       anchors.left_bottom->center_x,anchors.left_bottom->center_y)*
                                       (1-pow(k_y,-size))/(1-1/k_y);
        double right_bottom_x_should=__len(anchors.left_bottom->center_x,anchors.left_bottom->center_y,
                                       anchors.right_bottom->center_x,anchors.right_bottom->center_y)*
                                       (1-pow(k_x,-size))/(1-1/k_x);
        double right_bottom_y_should=__len(anchors.right_top->center_x,anchors.right_top->center_y,
                                       anchors.right_bottom->center_x,anchors.right_bottom->center_y)*
                                       (1-pow(k_y,-size))/(1-1/k_y);

        double p=Normal_dist::N(left_top_x_should,anchors.left_top->estimated_symbol_size_x)*
                 Normal_dist::N(left_top_y_should,anchors.left_top->estimated_symbol_size_y)*
                 Normal_dist::N(left_bottom_x_should,anchors.left_bottom->estimated_symbol_size_x)*
                 Normal_dist::N(left_bottom_y_should,anchors.left_bottom->estimated_symbol_size_y)*
                 Normal_dist::N(right_top_x_should,anchors.right_top->estimated_symbol_size_x)*
                 Normal_dist::N(right_top_y_should,anchors.right_top->estimated_symbol_size_y)*
                 Normal_dist::N(right_bottom_x_should,anchors.right_bottom->estimated_symbol_size_x)*
                 Normal_dist::N(right_bottom_y_should,anchors.right_bottom->estimated_symbol_size_y);

        if(p>best){
            p=best;
            result=size;
        }
    }
    if(best<CONST(GUESS_BLOCK_SIZE_PROB_THRES, double))return None<int>();
    else return Option<int>(result);
}
#endif

Symbol_scanner::Block_content Symbol_scanner::Block_anchor::get_block_content(Pixel_reader* reader_, int sidelength) {                                            int side_count) {
    auto fun_locate = [side_count, left_top, right_top, right_bottom, left_bottom](int x, int y) {
        int fmx = side_count;
        int fmy = side_count;
        int fzx1 = x;
        int fzy1 = y;
        int fzx = fmx - x;
        int fzy = fmy - y;

        auto px = (fzx * fzy * left_top->center_x +
                   fzx1 * fzy * right_top->center_x +
                   fzx * fzy1 * left_bottom->center_x +
                   fzx1 * fzy1 * right_bottom->center_x) / (fmx * fmy);
        auto py = (fzx * fzy * left_top->center_y +
                   fzx1 * fzy * right_top->center_y +
                   fzx * fzy1 * left_bottom->center_y +
                   fzx1 * fzy1 * right_bottom->center_y) / (fmx * fmy);

        return Point(px, py);
    };
    return Symbol_scanner::Block_content(side_count, side_count, [reader_, fun_locate](int x, int y) {
                                             auto p = fun_locate(x, y);
                                             return reader_->get_RGB(p.x, p.y);
                                         },
                                         [reader_, fun_locate](int x, int y) {
                                             auto p = fun_locate(x, y);
                                             return reader_->get_smoothed_RGB(p.x, p.y);
                                         });
}


//must guarantee vcnt and hcnt are odd
Mat<Option<Symbol_scanner::Block_anchor>> Symbol_scanner::get_anchors(Mat<Option<Locator_scanner::Locator_scanner_result>>& map){
    Locator_scanner::Locator_scanner_result wont_use;

    return Mat<Option<Symbol_scanner::Block_anchor>>(map.get_width()-1,map.get_height()-1,[&map](int i, int j){
        if(!map[i][j].empty() && !map[i][j+1].empty() && !map[i+1][j].empty() && !map[i+1][j+1].empty()){
            return Some(Symbol_scanner::Block_anchor(
                    &(map[i][j].get_reference_or(wont_use)),&(map[i+1][j].get_reference_or(wont_use)),
                    &(map[i][j+1].get_reference_or(wont_use)),&(map[i+1][j+1].get_reference_or(wont_use))));
        }
        else return None<Symbol_scanner::Block_anchor>();
    });
}
