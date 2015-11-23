//
// Created by 王安然 on 15/11/23.
//

#ifndef DCODE_SEGMENT_POOL_H
#define DCODE_SEGMENT_POOL_H

#include "utils/utils.h"
#include "structures.h"

//not thread safe
class Segment_pool: Noncopyable{
private:
    static const int MAX_SEGMENT=3000;

    Rx_segment segments[MAX_SEGMENT];
    bool is_dynamic_alloc[MAX_SEGMENT];
    uint16_t stack_alloc[MAX_SEGMENT];
    int ptr_alloc;
    uint16_t stack_nonalloc[MAX_SEGMENT];
    int ptr_nonalloc;

    Segment_pool(){
        memset(is_dynamic_alloc,0,sizeof(bool)*MAX_SEGMENT);
        memset(stack_alloc,0,sizeof(uint16_t)*MAX_SEGMENT);

        for(uint16_t i=0;i<MAX_SEGMENT;i++)
            stack_nonalloc[i]=i;
        ptr_alloc=0;
        ptr_nonalloc=0;
    }

public:
    static Segment_pool& shared(){
        static Segment_pool pool;
        return pool;
    }

    Rx_segment* alloc(){
        if(ptr_nonalloc!=0){
            Rx_segment* res=segments+stack_nonalloc[--ptr_nonalloc];
            res->reset();
            return res;
        }
        else
            return new Rx_segment;
    }

    void free(Rx_segment* seg){
        if(seg>=segments && seg<segments+MAX_SEGMENT){
            stack_alloc[ptr_alloc++]=(uint16_t)(seg-segments);
        }
        else
            delete seg;
    }
};

#endif //DCODE_SEGMENT_POOL_H
