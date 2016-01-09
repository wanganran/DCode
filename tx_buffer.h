//
// Created by 王安然 on 16/1/8.
//

#ifndef DCODE_TX_BUFFER_H
#define DCODE_TX_BUFFER_H

class Tx_buffer{
private:
    static const int ID_RANGE=256;
    static const int R=3; //repeat NACK
    static const int NR=30; //maximum frames per RTT
    static const int BUFFER_SIZE=NR+K*R;
    static const int K=5; //if this frame is K frames later than last sent NACK, or current negative blocks are exceeding block size, and there exist nagative blocks since last sent NACK, then send a new NACK.
    static const int D=2; //regard the most recent D received frames as non-complete frames
    static const int MAX_BLOCK_PER_PACKET=64;
};

#endif //DCODE_TX_BUFFER_H
