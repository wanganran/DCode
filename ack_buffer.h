//
// Created by 王安然 on 16/1/16.
//

#ifndef DCODE_ACK_BUFFER_H
#define DCODE_ACK_BUFFER_H
#include "structures.h"

class Ack_buffer:public Noncopyable{
private:
    //Rx analyses its window, putting blocks that need retransmission to this.
    //Tx request this and form Acks.
    Ack ack_;
    int64_t ack_tick_;
    //Rx receives Acks and putting it here.
    //Tx request this and build up retransmissions.
    Ack retrans_;
    int64_t retrans_tick_;

    volatile bool in_change_ack_;
    volatile bool read_ack_;
    volatile bool in_change_retrans_;
    volatile bool read_retrans_;
    volatile bool charge_ack_;
    volatile bool charge_retrans_;
public:
    Ack_buffer():in_change_ack_(false),read_ack_(false),in_change_retrans_(false),read_retrans_(false){}

    void push_ack(const Ack& ack);
    void push_retrans(const Ack& ack);

    int64_t pop_ack(Ack& out_ack);
    int64_t pop_retrans(Ack& out_ack);

    static Ack_buffer& shared(){
        static Ack_buffer singleton;
        return singleton;
    }
};

#endif //DCODE_ACK_BUFFER_H
