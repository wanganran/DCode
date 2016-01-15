//
// Created by 王安然 on 16/1/12.
//

#include "tx_buffer.h"
#include "config.h"

static const int MAX_ACK_PACKET_SIZE=100;

static void Tx_buffer::worker_thread_func(Tx_buffer* buffer, Screen_fetcher* fetcher){
    auto& parameters=Tx_adaptive_parameters::current();
    auto& modulator=Modulator_factory::get_modulator();
    std::shared_ptr<Packet> last_packet;
    int last_packet_offset;
    while(buffer->worker_thread_flag_){
        if(parameters.shift_next()); //currently do nothing

        //destination frame
        Tx_frame frame(Config::current().barcode_config.vertical_block_count,Config::current().barcode_config.horizontal_block_count)
        //first last packet
        if(last_packet){
            if(last_packet->length<=last_packet_offset)last_packet=nullptr;
            else {
                modulator.modulate_data(last_packet->data+last_packet_offset, dest);
            }
        }
        //every loop generate one frame
        if(!buffer->urgent_queue_.empty()){

        }
    }
}

Tx_buffer::Tx_buffer(Screen_fetcher *fetcher):fetcher_(fetcher){
    worker_thread_=std::thread(Tx_buffer::worker_thread_func,this,fetcher);
}

void Tx_buffer::push_packet(uint8_t *source, int length) {
    std::shared_ptr<Packet> packet=std::make_shared<Packet>();
    packet->init(Packet_type::DATA,length);
    memcpy(packet->data,source, length);
}

void Tx_buffer::push_ack(const Ack &ack) {
    uint8_t packet_buffer[MAX_ACK_PACKET_SIZE];

}

void Tx_buffer::push_action_block(const Tx_PHY_action &action) {

}

void Tx_buffer::push_probe_block(const Tx_PHY_probe &probe) {

}

void Tx_buffer::reset(Screen_fetcher* fetcher) {

}
