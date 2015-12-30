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
    static const int MAX_BLOCK_PER_PACKET=64;


    ///begin NACK buffer

    struct NACK_buffer{
        //stores the block idx of vacancy blocks
        std::vector<int> negative_blocks;
        int start_from_buffer_idx;
        int top_buffer_idx;

        NACK_buffer():start_from_buffer_idx(0), top_buffer_idx(0){}
    };

    NACK_buffer NACK_buffer_[R];
    int NACK_buffer_peak_;
    int NACK_buffer_tail_;

    ///end NACK buffer

    //begin Block_ref

    struct Block_ref{
        bool is_start_of_packet(){if(!segment) return false; return segment->metadata.start_of_packet;}
        bool is_end_of_packet(){if(!segment)return false; return segment->metadata.end_of_packet;}
        bool is_vacancy(){return segment==nullptr && !is_functional_;} //if vacancy, segment and node_ref must be nullptr
        bool is_functional_block(){return is_functional_;}

        bool init_as_functional(){
            is_functional_=true;
            if(segment){
                Segment_pool::shared().free(segment);
                segment= nullptr;
            }
        }
        bool init_as_segment(Rx_segment* segment_){
            if(segment)Segment_pool::shared().free(segment);
            segment=segment_;
        }
        bool reset(){
            if(segment)Segment_pool::shared().free(segment);
            segment= nullptr;
        }

        Rx_segment* segment;

        Block_ref():segment(nullptr), is_functional_(false){}
    private:
        bool is_functional_;
    };

    ///end Block_ref

    ///begin buffer flags

    //flag list
    std::set<int> flags_;

    //must ensure that flag_idx is before head_idx_
    void _insert_flag(int flag_idx){
        flags_.insert(flag_idx);
    }

    //[idx_begin, idx_end]
    void _remove_flag_range(int idx_begin, int idx_end){
        if(idx_begin<=idx_end) {
            auto it = flags_.lower_bound(idx_begin);
            while (it!=flags_.end() && *it <= idx_end) {
                auto it_ = it;
                it++;
                flags_.erase(it_);
            }
        }
        else {
            _remove_flag_range(idx_begin, *std::prev(flags_.end()) + 1);
            _remove_flag_range(0, idx_end);
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

    ///end buffer flags

    ///begin buffer properties

    int buffer_size_;
    int size_per_frame_;
    Block_ref* buffer_;

    int head_idx_;

    ///end buffer properties

    ///begin buffer manipulations

    bool _init_buffer(){
        auto config=Config::current();

        size_per_frame_=config->barcode_config.horizontal_block_count*config->barcode_config.vertical_block_count;
        buffer_size_=size_per_frame_*BUFFER_SIZE;
        buffer_=new Block_ref[buffer_size_];

        head_idx_=-1;
    }

    //to check whether mid is in [left, right]
    bool _cross(int left, int right, int mid){
        if(left<=right){
            return left<=mid && mid<=right;
        }
        else
            return _cross(left, buffer_size_,mid) || _cross(0, right,mid);
    }

    int _distance(int left, int right){
        return left<=right?right-left:right+buffer_size_-left;
    }

    template<typename F>
    void _foreach(int begin, int end, F func){
        if(begin<=end)
            for(int i=begin;i<=end;i++)
                func(i);
    }

    //check which packet a block belongs to. returns whether the packet exists.
    //out_left/right_flag is the beginning and ending of the packet or the speculated packet.
    //out_vacancy_num is the detected vacancy blocks (include functional blocks) within the packet.
    bool _check_packet(int id, int& out_left_flag, int& out_right_flag, int& out_vacancy_num){
        auto right_flag=_query_flag_after(id);
        auto left_flag=_query_flag_before(id);

        //if they are valid?

        //inside a detected packet
        if(right_flag.empty() || left_flag.empty() || _cross(left_flag.get_reference(),right_flag.get_reference(),head_idx_))return false;
        //the detected packet's length is valid
        if(_distance(left_flag.get_reference(),right_flag.get_reference())>MAX_BLOCK_PER_PACKET)return false;
        //the detected packet has a begining and ending

        //assert the flag is valid

        out_left_flag=left_flag.get_reference();
        out_right_flag=right_flag.get_reference();
        assert(buffer_[out_left_flag].segment!= nullptr && buffer_[out_right_flag].segment!= nullptr);
        if(!buffer_[out_left_flag].is_start_of_packet()){
            //then it is the end of the packet. its following block is the beginning
            out_left_flag++;
        }

        if(!buffer_[out_right_flag].is_end_of_packet()) {
            //then it is the start of the packet. if its previous block is a data block, then it is the end block. otherwise the one before its previous is the end block.
            if (buffer_[out_right_flag].segment->metadata.last_is_data)
                out_right_flag--;
            else out_right_flag -= 2;
        }

        //check again the validity
        if(_cross(out_left_flag,out_right_flag,head_idx_))return false;

        //traverse
        out_vacancy_num=0;

        _foreach(out_left_flag,out_right_flag,[&](int i){
            if(buffer_[i].is_vacancy())out_vacancy_num++;
        });

        return true;

    }


    //return if the segment is already received.
    //seg is allocated by the caller. After call, it doesn't need to be freed by the caller
    bool _insert(Rx_segment* seg){
        int id=seg->metadata.FID*size_per_frame_+seg->block_id;

        bool overrided=false;
        if(!buffer_[id].is_vacancy()){//replace it, no matter whether the original one is expired
            Segment_pool::shared().free(buffer_[id].segment);
            buffer_[id].init_as_segment(seg);
            overrided=true;
        }
        else {
            //insert
            buffer_[id].init_as_segment(seg);
        }
        if(!seg->metadata.last_is_data){
            //mark functional packet
            buffer_[(id-1+buffer_size_)%buffer_size_].init_as_functional();
        }
        //modify the header pos
        int old_head_idx=head_idx_;
        if(head_idx_==-1){ //this is the first received block
            head_idx_=id;
            _init_NACK(id);
        }
        else if(head_idx_>id && id+buffer_size_-head_idx_<D*size_per_frame_){
                //shift id in another circle
                head_idx_=id;
        }
        else{
            //it is placed before the head, no need to shift
        }

        //update flags
        if(head_idx_!=old_head_idx){
            //shifted
            _remove_flag_range(old_head_idx,head_idx_);
        }
        else if(overrided)//override a received block, return false directly
            return false;

        if (buffer_[id].is_end_of_packet() || buffer_[id].is_start_of_packet()){
            //insert flag
            _insert_flag(id);
        }
        return true;
    }

    void _clear_NACK(int NACK_idx, int last_top_buffer_idx){
        NACK_buffer_[NACK_idx].negative_blocks.clear();
        NACK_buffer_[NACK_idx].start_from_buffer_idx=(last_top_buffer_idx+1)% buffer_size_;
        NACK_buffer_[NACK_idx].top_buffer_idx=last_top_buffer_idx;
    }

    void _init_NACK(int updated_id){
        assert(NACK_buffer_peak_==0);
        NACK_buffer_tail_=NACK_buffer_peak_;
        Block_ref& block=buffer_[updated_id];
        assert(!block.is_vacancy());
        _clear_NACK(NACK_buffer_peak_, block.segment->get_full_id(size_per_frame_));
    }
    bool _update_NACK(int updated_id) {
        //only care D frames before updated_id
        int till_block_id = ((updated_id / size_per_frame_ - D + BUFFER_SIZE) * size_per_frame_ + size_per_frame_ - 1) %
                            buffer_size_;
        NACK_buffer &nack_buffer = NACK_buffer_[NACK_buffer_peak_];
        if (!_cross((nack_buffer.top_buffer_idx + 1) % buffer_size_, till_block_id, head_idx_))
            _foreach((nack_buffer.top_buffer_idx + 1) % buffer_size_, till_block_id, [&](int id) {
                if (buffer_[id].is_vacancy()) {
                    nack_buffer.negative_blocks.push_back(id);
                    if (!_cross(nack_buffer.start_from_buffer_idx, nack_buffer.top_buffer_idx, id))
                        nack_buffer.top_buffer_idx = id;
                }
            });
    }

    ///end buffer manipulations
public:
    int receive(Rx_segment* segment, Packet* out_packets_arr){
        //prerequisite: segment is a newly arrived one.
        if(!_insert(segment))return 0;

        //deferred: update NACK
        auto _deferred=defer([&](){
            _update_NACK(segment->get_full_id(size_per_frame_));
        });

        //first, insert the segment
        //second, check it it complete a packet, or it is an independent retransmission block
        //third, if so, check if it is a retransmission packet
        // if so, complete its corresponded block by recursion
        //otherwise, fill out_packets_arr and return.
    }
};

#endif //DCODE_RX_BUFFER_H
