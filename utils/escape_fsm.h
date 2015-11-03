//
// Created by 王安然 on 15/9/13.
//

#ifndef DCODE_ESCAPE_FSM_H
#define DCODE_ESCAPE_FSM_H
#include <stdint.h>
#include <math.h>
#include "utils.h"

class Escaper{
private:
    int count_;
    int correct_;
    int error_;

    const bool ERR_MAP[]={true, false, true, false, false, true, false, true};
public:
    Escaper():count_(0),correct_(0),error_(0){}
    void reset(){count_=0;correct_=0;error_=0;}
    double get_estimated_error_rate(){
        if(correct_<=error_)return 0.5;
        else return (1-sqrt(1-2*error_/(double)(correct_+error_)))/2;
    }

    uint8_t* input(uint8_t symbol, uint8_t* out){
        if(count_%3<2) {
            *out |= ((symbol & 7) << (5 - (count_ % 3) * 3));
            count_++;
            return out;
        }
        else {
            ERR_MAP[symbol&7]?error_++:correct_++;
            *out |= (symbol&6);
            return out+1;
        }
    }
};

/*
//escape WWWBBBWWW like
template<int mod_depth>
class Escape_FSM{
friend class Singleton_helper;
private:

    struct State_tb{
        int met[2];
    }* state_tb;

    int state_;
    int escape_state_;
    Escape_FSM(int mod_depth):state_(0),escape_state_(mod_depth*3-1){

        state_tb=new State_tb[mod_depth*3];

        for(int i=0;i<mod_depth;i++)state_tb[i].met[0]=i+1, state_tb[i].met[1]=0;
        for(int i=mod_depth;i<mod_depth*2;i++)state_tb[i].met[1]=i+1, state_tb[i].met[0]=1;
        state_tb[mod_depth].met[0]=mod_depth;
        for(int i=mod_depth*2;i<mod_depth*3;i++)state_tb[i].met[1]=0,state_tb[i].met[0]=i+1;
    }

    class Singleton_helper_{

    public:
        static const int MAX_DEPTH=6;
        static const int MIN_DEPTH=3;
        Escape_FSM* obj_[MAX_DEPTH-MIN_DEPTH+1];

        Singleton_helper_(){
            for(int i=MIN_DEPTH;i<=MAX_DEPTH;i++)
                obj_[i-MIN_DEPTH]=new Escape_FSM(i);
        }
        ~Singleton_helper_(){
            for(int i=0;i<MAX_DEPTH-MIN_DEPTH+1;i++)delete obj_[i];
        }
    };

public:
    void reset(){state_=0;}
    uint8_t* input(uint8_t in, uint8_t* out){
        state_=state_tb[state_].met[in];
        if(state_!=escape_state_){
            *out=in;
            return out+1;
        }
        else{
            *out=0;
            out[1]=in;
            return out+2;
        }
    }
    bool is_to_escaped(){
        return state_==escape_state_;
    }

    static Escape_FSM* get_FSM(int mod_depth){
        static Singleton_helper_ singleton;
        return singleton.obj_[mod_depth-Singleton_helper_::MIN_DEPTH];
    }
};
*/
#endif //DCODE_ESCAPE_FSM_H
