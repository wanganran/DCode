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
        //minimum block idx
        int start_from_buffer_idx;
        //maximum block idx
        int top_buffer_idx;

        NACK_buffer():start_from_buffer_idx(0), top_buffer_idx(0){}
    };

    NACK_buffer NACK_buffer_[R];
    int NACK_buffer_peak_;
    int NACK_buffer_tail_;

    //init a new NACK.
    void _shift_NACK_buffer(){
        int old_peak=NACK_buffer_peak_;
        NACK_buffer_peak_=(NACK_buffer_peak_+1)%R;
        if(NACK_buffer_tail_==NACK_buffer_peak_)NACK_buffer_tail_=(NACK_buffer_tail_+1)%R;
        _clear_NACK(NACK_buffer_peak_,NACK_buffer_[old_peak].top_buffer_idx);
    }

    //clear a certain NACK
    void _clear_NACK(int NACK_idx, int last_top_buffer_idx){
        NACK_buffer_[NACK_idx].negative_blocks.clear();
        NACK_buffer_[NACK_idx].start_from_buffer_idx=_next(last_top_buffer_idx);
        NACK_buffer_[NACK_idx].top_buffer_idx=last_top_buffer_idx;
    }

    //init the pointers and clear the first NACK.
    void _init_NACK(int updated_id){
        assert(NACK_buffer_peak_==0);
        NACK_buffer_tail_=NACK_buffer_peak_; //length=1
        Block_ref& block=buffer_[updated_id];
        assert(!block.is_vacancy());
        _clear_NACK(NACK_buffer_peak_, block.segment->get_full_id(size_per_frame_));
    }

    //insert a new NACK entry to current peak NACK
    bool _update_NACK(int updated_id) {
        //only care D frames before updated_id
        int till_block_id = ((updated_id / size_per_frame_ - D + BUFFER_SIZE) * size_per_frame_ + size_per_frame_ - 1) %
                            buffer_size_;
        NACK_buffer &nack_buffer = NACK_buffer_[NACK_buffer_peak_];
        auto from_block_id=_next(nack_buffer.top_buffer_idx);
        if (!_cross(from_block_id, till_block_id, head_idx_))
            _foreach(from_block_id, till_block_id, [&](int id) {
                if (buffer_[id].is_vacancy()) {
                    nack_buffer.negative_blocks.push_back(id);
                    if (!_cross(nack_buffer.start_from_buffer_idx, nack_buffer.top_buffer_idx, id))
                        nack_buffer.top_buffer_idx = id;
                }
            });
    }

    ///end NACK buffer

    //begin Block_ref

    struct Block_ref{
        bool is_start_of_packet(){if(!segment) return false; return segment->metadata.start_of_packet;}
        bool is_end_of_packet(){if(!segment)return false; return segment->metadata.end_of_packet;}
        bool is_start_or_end_of_packet(){return is_start_of_packet() || is_end_of_packet();}
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
            is_functional_=false;
        }
        bool reset(){
            if(segment)Segment_pool::shared().free(segment);
            segment= nullptr;
            is_functional_=false;
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
            _remove_flag_range(idx_begin, *std::prev(flags_.end()));
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

    //find x if x==idx
    Option<int> _query_flag(int idx){return flags_.find(idx)==flags_.end()?Some(idx):None<int>();}

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

        //-1 indicates the buffer is empty.
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

    //previous and next one position within the buffer
    int _prev(int x){
        return (x+buffer_size_-1)%R;
    }
    int _next(int x){
        return (x+1)%R;
    }

    //begin, end are indexes within [0,buffer_size_].
    template<typename F>
    void _foreach(int begin, int end, F func){
        if(begin<=end)
            for(int i=begin;i<=end;i++)
                func(i);
        else{
            _foreach(begin,buffer_size_,func);
            _foreach(0,end,func);
        }
    }

    //check whether a block complete a packet. return false if cannot complete
    //this method must be FAST. Constant time complexity O(1) is necessary.
    bool _check_packet_fast(int id){
        const int SEARCH_RANGE=3;
        int from=id-SEARCH_RANGE;
        int to=id+SEARCH_RANGE;
        if(to<from)to+=buffer_size_;
        bool flag=true;
        for(int i=id-1;i>=from;i--){
            Block_ref& buffer=buffer_[(i+buffer_size_)%buffer_size_];
            if(buffer.is_start_of_packet() || buffer.is_end_of_packet()){
                break;
            }
            else if(buffer.is_vacancy()){
                flag=false;
                break;
            }
        }
        if(flag){
            for(int i=id+1;i<=to;i++){
                Block_ref& buffer=buffer_[i%buffer_size_];
                if(buffer.is_start_of_packet() || buffer.is_end_of_packet()){
                    break;
                }
                else if(buffer.is_vacancy()){
                    flag=false;
                    break;
                }
            }
        }
        return flag;
    }

    //check which packet a block belongs to. returns whether the packet exists.
    //it won't be a standalone retransmission block. It should be proceeded prior to calling this.
    //out_left/right_flag is the beginning and ending of the packet or the speculated packet.
    //out_vacancy_num is the detected vacancy blocks (include functional blocks) within the packet.
    bool _check_packet(int id, int& out_left_flag, int& out_right_flag, int& out_vacancy_num){
        auto right_flag=_query_flag_after(_prev(id));
        auto left_flag=_query_flag_before(_next(id));

        //first, are they valid?
        //it is a standalone packet
        if(buffer_[id].is_start_of_packet() && buffer_[id].is_end_of_packet()) {
            out_left_flag = id;
            out_right_flag = id;
            out_vacancy_num = 0;
            return true;
        }
        else if(buffer_[id].is_start_of_packet()){
            //find afterwards
            left_flag=Some(id);
        }
        else if(buffer_[id].is_end_of_packet()){
            right_flag=Some(id);
        }
        //otherwise, left to right is the packet.

        //inside a detected packet
        if(right_flag.empty() || left_flag.empty() ||
                right_flag==left_flag || //only one flag exists
                _cross(left_flag.get_reference(),right_flag.get_reference(),head_idx_)) //the right flag is an old flag
            return false;
        //the detected packet's length is valid
        if(_distance(left_flag.get_reference(),right_flag.get_reference())>MAX_BLOCK_PER_PACKET)return false;


        //second, the detected packet has a begining and ending

        //assert the flag is valid
        out_left_flag=left_flag.get_reference();
        out_right_flag=right_flag.get_reference();
        //once flag inserted, the buffer should not be empty.
        assert(buffer_[out_left_flag].is_start_or_end_of_packet() && buffer_[out_right_flag].is_start_or_end_of_packet());
        if(!buffer_[out_left_flag].is_start_of_packet()){
            //then it is the end of the packet. its following block is the beginning
            out_left_flag=_next(out_left_flag);
        }

        if(!buffer_[out_right_flag].is_end_of_packet()) {
            //then it is the start of the packet. if its previous block is a data block, then it is the end block. otherwise the one before its previous is the end block.
            if (buffer_[out_right_flag].segment->metadata.last_is_data)
                out_right_flag=_prev(out_right_flag);
            else out_right_flag =_prev(_prev(out_right_flag));
        }

        //check again the validity
        assert(_cross(out_left_flag, out_right_flag, id));
        assert(!_cross(out_left_flag, out_right_flag, head_idx_));

        //third, traverse
        out_vacancy_num=0;

        _foreach(out_left_flag,out_right_flag,[&](int i){
            if(buffer_[i].is_vacancy())out_vacancy_num++;
        });

        return true;
    }

    //| ret_count: 1 byte | ret_1_content_length: 1 byte | ret_1_fid: 1 byte |
    //| ret_1_bid: 1 byte | last_is_data: 1 bit | start_of_packet: 1 bit | end_of_packet: 1 bit | padding |
    //| content: n bytes |...
    int _fill_retransmission_packet(Packet* ret_packet, Packet* out_packets_ptr){
        assert(ret_packet->type==Packet_type::COMBINED_RETRANSMISSION);
        //parse the packet
        int ret_count=ret_packet->data[0];
        int t=1;
        int out_count=0;
        for(int i=0;i<ret_count;i++){
            uint8_t* ptr=ret_packet->data+t;
            Rx_segment* seg=Segment_pool::shared().alloc();
            seg->init(ptr[0]);
            seg->metadata.FID=ptr[1];
            seg->block_id=ptr[2];
            seg->metadata.last_is_data=(ptr[3]&128)>0;
            seg->metadata.start_of_packet=(ptr[3]&64)>0;
            seg->metadata.end_of_packet=(ptr[3]&32)>0;
            //fake a single retransmission
            seg->metadata.type=Packet_type::SINGLE_BLOCK_RETRANSMISSION;

            memcpy(seg->data,ptr+4,ptr[0]);
            t+=ptr[0]+4;

            out_count+=receive(seg,out_packets_ptr);
        }
        return out_count;
    }

    //this won't shift the head
    //ret doesn't need to be freed after this call.
    bool _fill_retransmission(Rx_segment* ret){
        int id=ret->get_full_id(size_per_frame_);
        assert(!buffer_[id].is_functional_block());
        bool replace=!buffer_[id].is_vacancy();
        buffer_[id].init_as_segment(ret);
        if(!ret->metadata.last_is_data)
            buffer_[_prev(id)].init_as_functional();

        if(buffer_[id].is_start_or_end_of_packet())
            _insert_flag(id);

        return !replace;
    }

    //return if the segment is already received.
    //seg is allocated by the caller. After call, it doesn't need to be freed by the caller
    bool _insert(Rx_segment* seg) {
        int id = seg->get_full_id(size_per_frame_);

        //indicates if the id is already occupied, including the old one is expired.
        bool overrided = false;
        if (!buffer_[id].is_vacancy()) {
            //replace it, no matter whether the original one is expired
            //Segment_pool::shared().free(buffer_[id].segment);
            buffer_[id].init_as_segment(seg);
            overrided = true;
        }
        else {
            //insert
            buffer_[id].init_as_segment(seg);
        }

        if (!seg->metadata.last_is_data) {
            //mark functional packet
            buffer_[_prev(id)].init_as_functional();
        }

        //modify the header pos
        int old_head_idx = head_idx_;
        if (head_idx_ == -1) { //this is the first received block
            head_idx_ = id;
            _init_NACK(id);
        }
        else if (!_cross(((head_idx_ / size_per_frame_ - D + 1 + BUFFER_SIZE) * size_per_frame_) % buffer_size_,
                         head_idx_, id)) {
            //the newly arrived one is only permitted to be D frames prior to the head frame
            // otherwise shift the head
            _foreach(_next(head_idx_), _prev(id), [&](int i){
                buffer_[i].reset();
            });
            head_idx_ = id;
        }
        else {
            //it is placed before the head, no need to shift
        }

        //update flags
        if (head_idx_ != old_head_idx) {
            //shifted
            _remove_flag_range(old_head_idx, head_idx_);
        }
        else if (overrided) {
            //override a received block. If received, the flags must already been inserted.
            // return false directly
            assert(!_query_flag(id).empty());
            return false;
        }

        //finally, insert the flags
        if (buffer_[id].is_start_or_end_of_packet()) {
            //insert flag
            _insert_flag(id);
        }
        return true;
    }


    ///end buffer manipulations
public:
    //the segment doesn't need to be freed after the call.
    int receive(Rx_segment* segment, Packet* out_packets_arr){
        //deferred: update NACK
        auto _deferred=defer([&](){
            _update_NACK(segment->get_full_id(size_per_frame_));
        });

        //first, check if the segment is a standalone retransmission
        bool is_retrans_block=false;
        if(segment->metadata.type==Packet_type::SINGLE_BLOCK_RETRANSMISSION){
            is_retrans_block=true;
            segment->metadata.type=Packet_type::DATA;
        }

        //first, insert the segment
        //prerequisite: segment is a newly arrived one.
        if(is_retrans_block)
            if(!_fill_retransmission(segment))return 0;
        else
            if(!_insert(segment))return 0;

        //second, check it complete a packet, or it is a combined retransmission block
        int left_flag, right_flag, vacancies;
        if(_check_packet_fast(segment->get_full_id(size_per_frame_)) //first a fast pass
           && _check_packet(segment->get_full_id(size_per_frame_),left_flag,right_flag,vacancies)){
            //a packet is formed. Check if it is complete
            if(vacancies==0){
                //form a packet
                assert(buffer_[left_flag].is_start_of_packet() && buffer_[right_flag].is_end_of_packet());
                //calculate the length
                int length=0;
                Packet_type type=buffer_[left_flag].segment->metadata.type;
                _foreach(left_flag,right_flag,[&](int i){
                    if(buffer_[i].segment)
                        length+=buffer_[i].segment->data_len;
                    if(i!=left_flag)
                        assert(buffer_[i].segment->metadata.type==type);
                });

                out_packets_arr->init(type,length);

                int offset=0;
                _foreach(left_flag,right_flag,[&](int i){
                    memcpy(out_packets_arr->data+offset, buffer_[i].segment->data,buffer_[i].segment->data_len);
                    offset+=buffer_[i].segment->data_len;
                });

                //check if the formed packet is a retransmission
                if(type==Packet_type::COMBINED_RETRANSMISSION){
                    return _fill_retransmission_packet(out_packets_arr, out_packets_arr);
                }
                else return 1;
            }
        }
        else
            return 0;
    }
};

#endif //DCODE_RX_BUFFER_H
