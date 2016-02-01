//
// Created by 王安然 on 16/1/12.
//
#include "rx_buffer.h"

Ack Rx_buffer::_parse_ack(Packet* pkt) {
    uint8_t* ptr=pkt->data;
    Ack res;
    while(ptr<pkt->data+pkt->length){
        int fid=ptr[0];
        bool flipped=(ptr[1]>=128);
        int num=ptr[1]&127;
        ptr+=2;

        std::vector<int> dest;
        for(int i=0;i<num;i++){
            if(ptr[0]<128){
                dest.push_back(fid*size_per_frame_+(ptr[0]&127));
                ptr++;
            }
            else{
                int start=(ptr[0]&127);
                int end=(ptr[1]&127);
                for(int i=start;i<=end;i++){
                    dest.push_back(fid*size_per_frame_+i);
                }
                ptr+=2;
            }
        }
        if(flipped){
            std::vector<int> flip;
            int t=0;
            for(int i=0;i<size_per_frame_;i++) {
                if (t<dest.size() && dest[t] == i) {
                    t++;
                }
                else
                    flip.push_back(i);
            }

            res.blocks_to_acked.insert(res.blocks_to_acked.end(),flip.begin(),flip.end());
        }
        else
            res.blocks_to_acked.insert(res.blocks_to_acked.end(),dest.begin(),dest.end());
    }

    return res;
}

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
                Packet copy_current(*out_packets_arr);
                auto res=_fill_retransmission_packet(&copy_current, left_flag, out_packets_arr);
                return res;
            }
            else if(type==Packet_type::ACK){
                Ack_buffer::shared().push_retrans(_parse_ack(out_packets_arr));
                return 0;
            }
            else return 1;
        }
    }
    else
        return 0;
}


void Rx_buffer::reset(){
    _init_buffer();
    for(int i=0;i<buffer_size_;i++)buffer_[i].reset();
}