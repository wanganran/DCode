//
// Created by 王安然 on 15/10/23.
//

#ifndef DCODE_PACKET_H
#define DCODE_PACKET_H

#include "structures.h"

class Packet_encoder{
    static int encode_data_packet(const Packet& src, Tx_block* buffer);
    static int decode_data_packet(Rx_segment* src_from, Rx_segment* src_to, Packet& dest);
};

#endif //DCODE_PACKET_H
