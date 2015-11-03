//
// Created by 王安然 on 15/10/23.
//

#ifndef DCODE_PACKET_H
#define DCODE_PACKET_H

#include "structures.h"

enum class Packet_type{
    DATA,
    PACKED_RETRANS,
    SINGLE_RETRANS
};

class packet{
public:
    Packet_type type;
    uint8_t* data;
    int len;

    bool transform_to_blocks(Block* dest, int max_len);
    bool from_captured_blocks(Segment* segs, int len);
};

#endif //DCODE_PACKET_H
