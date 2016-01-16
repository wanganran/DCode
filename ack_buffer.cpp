//
// Created by 王安然 on 16/1/16.
//

#include "ack_buffer.h"
#include "utils/utils.h"

void Ack_buffer::push_ack(const Ack &ack) {
    in_change_ack_=true;
    charge_ack_=1;
    while(read_ack_ && charge_ack_==1);
    this->ack_=ack;
    this->ack_tick_=get_current_millis();
    in_change_ack_=false;
}

void Ack_buffer::push_retrans(const Ack &ack) {
    in_change_retrans_=true;
    charge_ack_=1;
    while(read_retrans_ && charge_retrans_==1);
    this->retrans_=ack;
    this->retrans_tick_=get_current_millis();
    in_change_retrans_=false;
}

int64_t Ack_buffer::pop_ack(Ack &out_ack) {
    read_ack_=true;
    charge_ack_=0;
    while(in_change_ack_ && charge_ack_==0);
    int64_t result=this->ack_tick_;
    out_ack=this->ack_;
    read_ack_=false;

    return result;
}

int64_t Ack_buffer::pop_retrans(Ack &out_ack) {
    read_retrans_=true;
    charge_retrans_=0;
    while(in_change_retrans_ && charge_retrans_==0);
    int64_t result=this->retrans_tick_;
    out_ack=this->retrans_;
    read_retrans_=false;

    return result;
}
