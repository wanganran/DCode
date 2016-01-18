//
// Created by 王安然 on 16/1/12.
//

#include <mutex>

#include "tx_buffer.h"
#include "config.h"
#include "ack_buffer.h"

static const int MAX_ACK_PACKET_SIZE=100;

static void Tx_buffer::worker_thread_func(Tx_buffer* buffer, Screen_fetcher* fetcher){
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
    Tx_frame frame(Config::current().barcode_config.vertical_block_count,Config::current().barcode_config.horizontal_block_count)

    bool last_is_data=true;

    int current_FID=0;

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
            auto pkt=buffer->_form_retrans_packet(newly_retrans);
            buffer->urgent_queue_.push(In_ref(pkt));
        }

        frame.init();

        int current_block_id=0;
        int total_block_id=frame.horizontal_count*frame.vertical_count;

        #define GET_BLOCK(id) (frame.get_block_ref((id)%frame.horizontal_count, (id)/frame.horizontal_count))

        //every loop generate one frame
        while(current_block_id!=total_block_id) {
            //load a new packet here
            std::unique_lock<std::mutex> lock(buffer->queue_mutex_);
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
                            current_block_id++;
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::ACTION);
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
                            current_block_id++;
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::PROBE);
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
                            lock.unlock();
                        }
                        break;
                    case In_ref::ACTION:
                        if (last_is_data) {
                            modulator.modulate_action(ref.workload.in_action, GET_BLOCK(current_block_id));
                            current_block_id++;
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::ACTION);
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
                            current_block_id++;
                            buffer->_update_block_ref(current_FID, current_block_id, Block_type::PROBE);
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
    }
}

Tx_buffer::Tx_buffer(Screen_fetcher *fetcher):fetcher_(fetcher){
    worker_thread_=std::thread(Tx_buffer::worker_thread_func,this,fetcher);
}

std::future<bool> Tx_buffer::push_packet(uint8_t *source, int length) {
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

void Tx_buffer::reset(Screen_fetcher* fetcher) {

}
