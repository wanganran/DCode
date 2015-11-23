//
// Created by 王安然 on 15/6/23.
//

#ifndef DCODE_CONSTANTS_H
#define DCODE_CONSTANTS_H
#include <cstdint>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include "utils.h"


class Constants: public Noncopyable{
private:
    inline static bool __exists (const char* name) {
        struct stat buffer;
        return (stat (name, &buffer) == 0);
    }
    static const char* CONST_PATH="/Users/wanganran/const.txt";
    int64_t table_[1024];//maximum 1024 entries
    Constants(){}
public:
    enum{
        //All necessary constants name

        //category #1: memory allocation and buffering
        //range 0 to 31

        SCREEN_PAINTER_QUEUE_SIZE=1,//def:3

        //category #2: locating related
        //range 32 to 63

        INITIAL_BLACK_WHITE_THRES=32, //def:100     the Y component threshold
        WHITE_DISTORTION_THRES_MIN=33, //def:0.9    the average minimum ratio between the white length and the actual length
        WHITE_DISTORTION_THRES_MAX=34, //def:1.4
        BLACK_DISTORTION_THRES_MIN=35, //def:0.7
        BLACK_DISTORTION_THRES_MAX=36, //def:1.1
        SAME_DISTORTION_THRES=37, //def:0.2         let it be x, then the average ratio between two same color length is within (1-x, 1+x)
        SYMBOL_SIZE_MIN_PIXEL=38, //def: 2          minimum pixel count per symbol side
        SYMBOL_SIZE_MAX_RATIO=39, //def: 0.03       maximum ratio between symbol side length and captured width/height
        SAME_COLOR_DIST_THRES=40, //def: 30         the maximum difference per channel between the colors of the symbols of a same color (B/W).
        DIFF_COLOR_DIST_THRES=41, //def: 60         the minimum difference per channel between the colors of the symbols of a different color (B&W).
        XY_DISTORTION_THRES=42, //def: 2.0          let it be x, then the ratio between the vertical side and horizontal side of a symbol is within (1/x, x)
        INTRINSIC_COLOR_DIFF_THRES=43, //def:40     the intrinsic color minimum difference per channel
        CONTIGUOUS_DISTORTION_THRES=44, //def:0.3   let it be x, then the ratio between two contiguous symbol side lengths (spatially or temporally) is within (1-x, 1+x)
        TRACK_SEARCH_RATIO=45, //def: 5.0           the ratio between the search ratio in pixel and the last symbol size in pixel.

        //category #3: recognition related
        //range 64 to 95

        GUESS_BLOCK_SIZE_PROB_THRES=64, //def:0.01  the threshold of the multiply of all 8 probabilities of guessing a certain block size.
        LOCATOR_LIKELIHOOD_RANGE=65, //def:0.5      let it be x, then the ratio of the maximum and minimum locator lilelihood is no less than x.

        //category #4: painter related
        //range 96 to 127
        MARGIN_LEFT_BLOCK_RATIO=96,
        MARGIN_TOP_BLOCK_RATIO=97
    };

    static std::unique_ptr<Constants>& current(){
        static std::unique_ptr<Constants> current_=NULL;
        if(current_)return current_;
        else{
            current_.reset(new Constants());
            const char* path=CONST_PATH;
            if(__exists(path)) {
                FILE* f=fopen(path, "rb");
                fread(current_->table_,sizeof(int64_t),1024,f);
                fclose(f);
                return current_;
            }
            else{
                //initialize the table
                current_->get<int>(SCREEN_PAINTER_QUEUE_SIZE)=1;

                current_->get<int>(INITIAL_BLACK_WHITE_THRES)=100;
                current_->get<double>(WHITE_DISTORTION_THRES_MIN)=0.9;
                current_->get<double>(WHITE_DISTORTION_THRES_MAX)=1.4;
                current_->get<double>(BLACK_DISTORTION_THRES_MIN)=0.7;
                current_->get<double>(BLACK_DISTORTION_THRES_MAX)=1.1;
                current_->get<double>(SAME_DISTORTION_THRES)=0.2;
                current_->get<int>(SYMBOL_SIZE_MIN_PIXEL)=2;
                current_->get<double>(SYMBOL_SIZE_MAX_RATIO)=0.03;
                current_->get<int>(SAME_COLOR_DIST_THRES)=30;
                current_->get<int>(DIFF_COLOR_DIST_THRES)=60;
                current_->get<double>(XY_DISTORTION_THRES)=2.0;
                current_->get<int>(INTRINSIC_COLOR_DIFF_THRES)=40;
                current_->get<double>(CONTIGUOUS_DISTORTION_THRES)=0.3;
                current_->get<double>(TRACK_SEARCH_RATIO)=5.0;

                current_->get<double>(GUESS_BLOCK_SIZE_PROB_THRES)=0.01;
                current_->get<double>(LOCATOR_LIKELIHOOD_RANGE)=0.5;

                current_->get<double>(MARGIN_LEFT_BLOCK_RATIO)=0.5;
                current_->get<double>(MARGIN_TOP_BLOCK_RATIO)=0.5;

                FILE* f=fopen(path,"wb");
                fwrite(current_->table_,sizeof(int64_t),1024,f);
                fclose(f);
                return current_;
            }
        }
    }

    template<typename T>
    T& get(int name){
        assert(name<1024);
        int64_t* ptr=table_+name;
        return *((T*)ptr);
    }
};

#define CONST(x,type) (Constants::current()->get<type>(Constants::x))
#define LOCALCONST(x,type) static const type x=CONST(x,type)

#endif //DCODE_CONSTANTS_H
