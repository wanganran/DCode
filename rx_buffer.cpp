//
// Created by 王安然 on 16/1/12.
//
#include "rx_buffer.h"

//the segment doesn't need to be freed after the call.
int Rx_buffer::receive(Rx_segment* segment, Packet* out_packets_arr, bool is_retrans=false){
    //deferred: update NACK
    auto _deferred=defer([&](){
        _update_NACK(segment->get_full_id(size_per_frame_));
    });

    if(last_tick_==-1)last_tick_=get_current_millis();
    else if(_check_timeout())reset();
    last_tick_=get_current_millis();

    if(is_retrans)
        if(!_insert_retransmission(segment))return 0;
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
            int32_t should_len=*(int32_t *)(buffer_[left_flag].segment->data);
            Packet_type type=buffer_[left_flag].segment->metadata.type;
            _foreach(left_flag,right_flag,[&](int i){
                if(buffer_[i].segment) {
                    length += buffer_[i].segment->data_len;
                    if (i != left_flag)
                        assert(buffer_[i].segment->metadata.type == type);
                }
            });
            assert(should_len<=length-4);
            out_packets_arr->init(type,should_len);

            int offset=0;
            _foreach(left_flag,right_flag,[&](int i){
                if(offset<should_len && buffer_[i].segment) {
                    int len=std::min(should_len-offset, buffer_[i].segment->data_len);
                    memcpy(out_packets_arr->data + offset, buffer_[i].segment->data, len);
                    offset += len;
                }
            });

            //check if the formed packet is a retransmission
            if(type==Packet_type::RETRANSMISSION){
                Packet* copy_current=new Packet(*out_packets_arr);
                return _fill_retransmission_packet(out_packets_arr, left_flag, out_packets_arr);
                delete copy_current;
            }
            else return 1;
        }
    }
    else
        return 0;
}

//receive a function (buffer, size, peak, tail)->bool). Shift NACK buffer if returning true
void Rx_buffer::manipulate_and_shift_NACK(std::function<bool(NACK_buffer*, int, int, int)> func){
    if(func(NACK_buffer_, R, NACK_buffer_peak_, NACK_buffer_tail_))
        _shift_NACK_buffer();
}

void Rx_buffer::reset(){
    _init_buffer();
    for(int i=0;i<buffer_size_;i++)buffer_[i].reset();
}