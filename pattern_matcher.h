//
// Created by 王安然 on 15/6/15.
//

#ifndef DCODE_PATTERN_MATCHER_H
#define DCODE_PATTERN_MATCHER_H

#include "utils/RGB.h"
#include <functional>


class Pattern_matcher{
public:
    template<int width,int height>
    struct Pattern{
        std::function<bool (RGB)>* pattern_[width][height];

        bool match(int x, int y, RGB color){
            return (*pattern_[x][y])(color);
        }

        static std::function<bool (RGB)>* get_pred_range(RGB min, RGB max){
            return new std::function<bool (RGB)>([min,max](RGB color){
                return color.R<=max.R&&color.G>=min.R&&
                       color.G<=max.G&&color.G>=min.G&&
                       color.B<=max.B&&color.B>=min.B;
            });
        }
        static std::function<bool (RGB)>* get_pred_any(){
            static const std::function<bool (RGB)>* f=new std::function<bool (RGB)>([](RGB color){return true;});
            return f;
        }
    };
    template <int width, int height>
    bool match(Pattern& pat, RGB** color){
        for(int i=0;i<width;i++){
            for(int j=0;j<height;j++)
                if(!pat.match(i,j,color[i][j]))return false;
        }
        return true;
    }
    template<int width, int height>
    int hamming_dist(Pattern& pat, RGB** color){
        int res=0;
        for(int i=0;i<width;i++){
            for(int j=0;j<height;j++)
                if(!pat.match(i,j,color[i][j]))res++;
        }
        return res;

    }

};

#endif //DCODE_PATTERN_MATCHER_H
