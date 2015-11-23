//
// Created by 王安然 on 15/11/21.
//

#ifndef DCODE_RX_BUFFER_H
#define DCODE_RX_BUFFER_H
#include <cstdint>
#include "structures.h"
#include "segment_pool.h"

class Rx_buffer{
private:

    static const int BUFFER_SIZE=256;
    static const int MAX_FPS=30;

    static const int TIMEOUT=BUFFER_SIZE*500/MAX_FPS;
    struct Node{
        Node* pre;
        bool delivered;
        int64_t retransmission;
        int block_ref_idx;

        Node():pre(nullptr),delivered(false),retransmission(-1){

        }

        ~Node(){
            if(pre)
                delete pre;
        }
    };
    Node* FID_to_node_[BUFFER_SIZE];

    struct Block_ref{
        bool is_start_of_frame;
        bool is_end_of_frame;
        bool is_vacancy; //if vacancy, segment and node_ref must be nullptr

        Node* node_ref;

        Rx_segment* segment;

        Block_ref():is_start_of_frame(false),is_end_of_frame(false),is_vacancy(true),node_ref(nullptr),segment(nullptr){}
    };

    int buffer_size_;
    int size_per_frame_;
    Block_ref* buffer_;

    void submit_packet_and_clear_timeout(int current_idx, Packet* dest, int& out_pkt_number, Ack& out_ack){
        int current_fid=current_idx/size_per_frame_;
        auto ref=buffer_[current_idx];
    }


    Block_ref& _get_block_ref(int id){
        Block_ref ref=buffer_[id];
        auto current_time=get_current_millis();
        if(ref.is_vacancy)return ref;
        if(ref.segment->rec_time_mil<current_time-TIMEOUT){ //timeout
            if(ref.node_ref) {
            }
        }
    }
    bool _init_buffer(){
        auto config=Config::current();

        size_per_frame_=config->barcode_config.horizontal_block_count*config->barcode_config.vertical_block_count;
        buffer_size_=size_per_frame_*BUFFER_SIZE;
        buffer_=new Block_ref[buffer_size_];
    }

    bool _insert(Rx_segment* seg){
        int id=seg->metadata.FID*size_per_frame_+seg->block_id;
        //if(buffer_[id].is_vacancy)
    }
public:

};

#endif //DCODE_RX_BUFFER_H
