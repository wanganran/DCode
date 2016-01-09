//
// Created by 王安然 on 16/1/8.
//

#ifndef DCODE_TX_BUFFER_H
#define DCODE_TX_BUFFER_H

#include "structures.h"

class Tx_buffer{
private:
    static const int ID_RANGE=256;
    static const int R=3; //repeat NACK
    static const int NR=30; //maximum frames per RTT

    constexpr int _SHIFT2(int x){
        int shift=0;
        while(x>0){
            x>>=1;
            shift++;
        }
        return 1<<shift;
    }
    static const int BUFFER_SIZE=_SHIFT2(NR+K*(R+1)+D);
    static const int K=5; //if this frame is K frames later than last sent NACK, or current negative blocks are exceeding block size, and there exist nagative blocks since last sent NACK, then send a new NACK.
    static const int D=2; //regard the most recent D received frames as non-complete frames
    static const int MAX_BLOCK_PER_PACKET=64;

    struct Packet_ref{
        std::shared_ptr<Packet> pkt;
        int pos;
        int len;
        Packet_ref(std::shared_ptr<Packet> _pkt, int _pos, int _len):pkt(_pkt), pos(_pos),len(_len){}
        Packet_ref(): pkt(nullptr), pos(0),len(0){}
    };

    struct Block_ref{
    public:
        Block_type block_type;
        Packet_type packet_type;
        Packet_ref in_packet;
    };
};

#endif //DCODE_TX_BUFFER_H
