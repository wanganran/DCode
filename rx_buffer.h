//
// Created by 王安然 on 15/11/21.
//

#ifndef DCODE_RX_BUFFER_H
#define DCODE_RX_BUFFER_H
#include <cstdint>
#include <set>
#include "structures.h"
#include "segment_pool.h"

class Rx_buffer{
private:

    static const int BUFFER_SIZE=256;
    static const int R=3; //repeat NACK
    static const int NR=30; //maximum frames per RTT
    static const int K=5; //if this frame is K frames later than last sent NACK, or current negative blocks are exceeding block size, and there exist nagative blocks since last sent NACK, then send a new NACK.
    static const int D=2; //regard the most recent D received frames as non-complete frames

    struct NACK_buffer{
        std::vector<std::pair<int,int>> negative_blocks;
        int until_buffer_idx;

        NACK_buffer():until_buffer_idx(0){}
    };

    NACK_buffer NACK_buffer_[R];
    int NACK_buffer_peak_;

    struct Block_ref{
        bool is_start_of_packet(){if(!segment) return false; return segment->metadata.start_of_packet;}
        bool is_end_of_packet(){if(!segment)return false; return segment->metadata.end_of_packet;}
        bool is_vacancy(){return segment==nullptr && !is_functional_;} //if vacancy, segment and node_ref must be nullptr
        bool is_functional_block(){return is_functional_;}

        bool init_as_functional(){is_functional_=true;}
        bool init_as_segment(Rx_segment* segment_){segment=segment_;}

        Rx_segment* segment;

        Block_ref():segment(nullptr), is_functional_(false){}
    private:
        bool is_functional_;
    };

    //flag list
    std::set<int> flags_;

    //must ensure that flag_idx is before head_idx_
    void _insert_flag(int flag_idx){
        flags_.insert(flag_idx);
    }

    //[idx_begin, idx_end)
    void _remove_flag_range(int idx_begin, int idx_end){
        auto it=flags_.lower_bound(idx_begin);
        if(it==flags_.end())return;
        while(*it<idx_end){
            auto it_=it;
            it++;
            flags_.erase(it_);
        }
    }

    //find maximal x s.t. x<=idx, circularly
    Option<int> _query_flag_before(int idx){
        if(flags_.size()==0)return None<int>();
        else if(flags_.size()==1)return Some(*(flags_.begin()));

        auto it=flags_.upper_bound(idx);
        if(it==flags_.begin())
            return Some(*(std::prev(flags_.end())));
        else return Some(*(std::prev(it)));
    }

    //find minimal x s.t. x>=idx, circularly
    Option<int> _query_flag_after(int idx){
        if(flags_.size()==0)return None<int>();
        else if(flags_.size()==1)return Some(*(flags_.begin()));

        auto it=flags_.lower_bound(idx);
        if(it==flags_.end())return Some(*(flags_.begin()));
        else return Some(*it);
    }

    int buffer_size_;
    int size_per_frame_;
    Block_ref* buffer_;

    int head_idx_;


    bool _init_buffer(){
        auto config=Config::current();

        size_per_frame_=config->barcode_config.horizontal_block_count*config->barcode_config.vertical_block_count;
        buffer_size_=size_per_frame_*BUFFER_SIZE;
        buffer_=new Block_ref[buffer_size_];

        head_idx_=-1;
    }

    bool _insert(Rx_segment* seg){
        int id=seg->metadata.FID*size_per_frame_+seg->block_id;
        if(!buffer_[id].is_vacancy()){//replace it
            Segment_pool::shared().free(buffer_[id].segment);
        }

        buffer_[id].segment = seg;
        if (buffer_[id].is_end_of_packet() || buffer_[id].is_start_of_packet());
    }
public:

};

#endif //DCODE_RX_BUFFER_H
