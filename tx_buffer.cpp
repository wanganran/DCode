//
// Created by 王安然 on 16/1/12.
//

#include <mutex>
#include <map>

#include "tx_buffer.h"
#include "config.h"
#include "ack_buffer.h"
#include "frame_painter.h"

static const int MAX_ACK_PACKET_SIZE=100;

static void Tx_buffer::worker_thread_func(Tx_buffer* buffer, Physical* physical){
    auto& parameters=Tx_adaptive_parameters::current();
    auto& modulator=Modulator_factory::get_modulator();

    std::promise<bool> last_promise;
    std::shared_ptr<Packet> last_packet;
    int last_packet_offset;

    int64_t last_ack_tick=-1;
    int64_t last_retrans_tick=-1;

    Ack newly_ack;
    Ack newly_retrans;

    //destination frame
    Tx_frame frame(Config::current().barcode_config.vertical_block_count,Config::current().barcode_config.horizontal_block_count);

    bool last_is_data=true;

    int current_FID=0;

    std::unique_lock<std::mutex> lock(buffer->queue_mutex_);

    //each while iteration, output one frame.
    while(buffer->worker_thread_flag_){
        if(parameters.shift_next()); //currently do nothing

        auto& ack_buffer=Ack_buffer::shared();
        auto ack_tick=ack_buffer.pop_ack(newly_ack);
        if(ack_tick!=last_ack_tick){
            //update ack
            last_ack_tick=ack_tick;
            auto pkt=buffer->_form_ack_packet(newly_ack);
            buffer->urgent_queue_.push(In_ref(pkt));
        }

        auto retrans_tick=ack_buffer.pop_retrans(newly_retrans);
        if(retrans_tick!=last_retrans_tick){
            //update retrans
            last_retrans_tick=retrans_tick;
            auto pkts=buffer->_form_retrans_packet(newly_retrans);
            for(auto& pkt:pkts)
                buffer->urgent_queue_.push(In_ref(pkt));
        }

        frame.init();

        int current_block_id=0;
        int total_block_id=frame.horizontal_count*frame.vertical_count;

        #define GET_BLOCK(id) (frame.get_block_ref((id)%frame.horizontal_count, (id)/frame.horizontal_count))


        //every loop generate one frame
        while(current_block_id!=total_block_id) {
            //load a new packet here
            lock.lock();
            if(!buffer->urgent_queue_.empty()) {
                auto &ref = buffer->urgent_queue_.front();
                switch (ref.in_type) {
                    case In_ref::PACKET:
                        if (last_packet==nullptr) {
                            last_packet = ref.workload.in_packet;
                            last_promise= std::move(ref.sent);
                            last_packet_offset = 0;
                            buffer->urgent_queue_.pop();
                        }
                        break;
                    case In_ref::ACTION:
                        if (last_is_data) {
                            modulator.modulate_action(ref.workload.in_action, GET_BLOCK(current_block_id));
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::ACTION);
                            current_block_id++;
                            ref.sent.set_value(true);
                            buffer->urgent_queue_.pop();
                            last_is_data = false;
                            lock.unlock();
                            continue;
                        }
                        break;
                    case In_ref::PROBE:
                        if (last_is_data) {
                            modulator.modulate_probe(ref.workload.in_probe, GET_BLOCK(current_block_id));
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::PROBE);
                            current_block_id++;
                            ref.sent.set_value(true);
                            buffer->urgent_queue_.pop();
                            last_is_data = false;
                            lock.unlock();
                            continue;
                        }
                        break;
                    default:break;
                }
            }
            else if(!buffer->regular_queue_.empty()) {
                auto& ref=buffer->regular_queue_.front();
                switch (ref.in_type) {
                    case In_ref::PACKET:
                        if (last_packet==nullptr) {
                            last_packet = ref.workload.in_packet;
                            last_promise=std::move(ref.sent);
                            last_packet_offset = 0;
                            buffer->regular_queue_.pop();
                        }
                        break;
                    case In_ref::ACTION:
                        if (last_is_data) {
                            modulator.modulate_action(ref.workload.in_action, GET_BLOCK(current_block_id));
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::ACTION);
                            current_block_id++;
                            ref.sent.set_value(true);
                            buffer->regular_queue_.pop();
                            last_is_data = false;
                            lock.unlock();
                            continue;
                        }
                        break;
                    case In_ref::PROBE:
                        if (last_is_data) {
                            modulator.modulate_probe(ref.workload.in_probe, GET_BLOCK(current_block_id));
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::PROBE);
                            current_block_id++;
                            ref.sent.set_value(true);
                            buffer->urgent_queue_.pop();
                            last_is_data = false;
                            lock.unlock();
                            continue;
                        }
                        break;
                    default:break;
                }
            }
            lock.unlock();

            if(last_packet==nullptr){
                modulator.modulate_idle(GET_BLOCK(current_block_id));
                buffer->_update_block_ref(current_FID,current_block_id,Block_type::IDLE);
                current_block_id++;
                last_is_data=false;
            }
            else if (last_packet->length <= last_packet_offset)last_packet = nullptr;
            else {
                Block_meta block_meta;
                block_meta.start_of_packet=last_packet_offset==0;
                block_meta.FID=current_FID;
                block_meta.last_is_data=last_is_data;
                block_meta.type=last_packet->type;

                int size=modulator.modulate_data(last_packet->data + last_packet_offset, GET_BLOCK(current_block_id), block_meta, last_packet->length-last_packet_offset, true);
                buffer->_update_block_ref(current_FID, current_block_id, Packet_ref(last_packet,last_packet_offset,size));
                last_packet_offset+=size;

                if(last_packet->length <= last_packet_offset){
                    last_packet=nullptr;
                    last_packet_offset=0;
                    last_promise.set_value(true);
                }

                current_block_id++;
                last_is_data=true;
            }

        }

        //now that a frame is constructed
        auto painter=physical->get_screen_painter();
        Frame_painter::paint_to_screen(painter,frame);
        physical->submit_screen_painter(painter);
        current_FID++;
    }
    if(last_packet)
        last_promise.set_value(false);

    lock.lock();
    while(!buffer->urgent_queue_.empty()){
        buffer->urgent_queue_.front().sent.set_value(false);
        buffer->urgent_queue_.pop();
    }
    while(!buffer->regular_queue_.empty()){
        buffer->regular_queue_.front().sent.set_value(false);
        buffer->regular_queue_.pop();
    }
}

Tx_buffer::Tx_buffer(Physical* phy){
    auto& config=Config::current();
    block_per_frame_=config.barcode_config.horizontal_block_count*config.barcode_config.vertical_block_count;
    total_block_ref_count_=block_per_frame_*BUFFER_SIZE;
    block_buffer_=new Block_ref[total_block_ref_count_];

    worker_thread_=std::thread(Tx_buffer::worker_thread_func,this,phy);
}

Tx_buffer::~Tx_buffer(){
    if(block_buffer_)delete[] block_buffer_;
    if(worker_thread_flag_) {
        worker_thread_flag_ = false;
        worker_thread_.join();
    }
}

std::future<bool> Tx_buffer::push_packet(uint8_t *source, unsigned int length) {
    std::shared_ptr<Packet> packet=std::make_shared<Packet>();
    packet->init(Packet_type::DATA,length);
    memcpy(packet->data,source, length);
}

std::future<bool> Tx_buffer::push_ack(const Ack &ack) {
    uint8_t packet_buffer[MAX_ACK_PACKET_SIZE];

}

std::future<bool> Tx_buffer::push_action_block(const Tx_PHY_action &action) {

}

std::future<bool> Tx_buffer::push_probe_block(const Tx_PHY_probe &probe) {

}

void Tx_buffer::reset(Physical* phy) {

}

void Tx_buffer::_update_block_ref(int fid, int block_id, const Tx_buffer::Packet_ref &ref) {
    int related_id=fid%BUFFER_SIZE;
    block_buffer_[related_id].clear();
    block_buffer_[related_id].block_type=Block_type::DATA;
    block_buffer_[related_id].in_packet=ref;
    block_buffer_[related_id].full_id=fid*;
}

void Tx_buffer::_update_block_ref(int fid, int block_id, Block_type type) {

}


std::vector<std::shared_ptr<Packet>> Tx_buffer::_form_retrans_packet(const Ack &ack) {
    //judge whether a block should be retransmitted
    std::map<std::shared_ptr<Packet>, std::vector<int>> missed_map;

    for(auto id : ack.blocks_to_acked){
        auto block_ref=_get_block_ref(id);
        if(block_ref && block_ref->block_type==Block_type::DATA){
            //mark the portion of that packet
            auto& pkt=block_ref->in_packet.pkt;
            auto ptr=missed_map.find(pkt);
            if(ptr==missed_map.end()) {
                missed_map.insert(std::pair<std::shared_ptr<Packet>, std::vector<int>>(
                        pkt, std::vector<int>{id}));
            }
            else
                ptr.second.push_back(id);
        }
    }
    int total_blocks_num=0;
    int total_size=0;

    int block_per_frame=Config::current().barcode_config.horizontal_block_count*Config::current().barcode_config.vertical_block_count;

    //if only one block, or no more than half of the packet is missed, retransmit them
    for(auto pair : missed_map){
        int missed_len=0;
        for(auto ref:pair.second)missed_len+=_get_block_ref(ref)->in_packet.len;
        if(pair.second.size()!=1 && missed_len<=pair.first->length/2){
            Packet pkt;
            pkt.init(Packet_type::RETRANSMISSION, (int)(1+missed_len+pair.second.size()*4));

            pkt.data[0]=(uint8_t)(pair.second.size());
            uint8_t *ptr=pkt.data+1;
            for(auto ref_id:pair.second){
                auto ref=_get_block_ref(ref_id);
                ptr[0]=(uint8_t)(ref->in_packet.len);
                ptr[1]=(uint8_t)(ref_id/block_per_frame);
                ptr[2]=(uint8_t)(ref_id%block_per_frame);
                ptr[3]=((ref->in_packet.pos==0?128:0)|
                        (ref->in_packet.pos+ref->in_packet.len>=ref->in_packet.pkt->length?64:0)|
                        ((uint8_t)ref->in_packet.pkt->type)|
                        _get_block_ref((ref_id-1))

            }
        }
        else{
            total_blocks_num+=pair.second->missed_block_num;
            total_size+=pair.second->missed_part;
        }
    }

    //form the packet
    for(auto id:ack.blocks_to_acked){
        auto block_ref=_get_block_ref(id);
        if(block_ref && block_ref->block_type==Block_type::DATA && block_ref->in_packet.pkt->missed_block_num!=0)

    }
}

std::shared_ptr<Packet> Tx_buffer::_form_ack_packet(const Ack &ack) {
    return __1::shared_ptr<Packet>();
}
