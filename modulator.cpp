//
// Created by 王安然 on 15/8/7.
//

#include "modulator.h"
using namespace std;

inline void _push_primary_with_escape(Escaper& escaper, unique_ptr<Tx_block::Tx_block_helper>& helper, uint8_t byte){
    uint8_t escaped[3];
    escaper.escape(byte, escaped);
    helper->push_primary(escaped[0]);
    helper->push_primary(escaped[1]);
    helper->push_primary(escaped[2]);
}
inline void _fill_rest_block(unique_ptr<Tx_block::Tx_block_helper>& helper){
    int rest=helper->get_total_symbol_count()%3;
    //safe symbol: 1
    while(rest--)helper->push_primary(1);
}

void Modulator::modulate_idle(Tx_block& dest){
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::IDLE, 0, parameters.parity);
    auto helper=dest.get_helper();

    int count=helper->get_total_symbol_count();
    //it is not necessary to encode secondary channel for IDLE blocks
    //BUT escape still exists.
    for(int i=0;i<count-3;i+=3){
        _push_primary_with_escape(escaper_, helper, rand_256());
    }
    _fill_rest_block(helper);
};


//define error correction level
int redundancy_primary(FEC_level level, int total_in_byte){
    //percentage
    static const int ratios[]={90,70,60};
    return ratios[(int)level]*total_in_byte/100;
}
int redundancy_secondary(FEC_level level, int total_in_byte){
    static const double ratios[]={80,65,50};
    return ratios[(int)]
}

//Data packet Link layer format:
//| 1 bit: last_is_data | 6 bits: FID |
//| 1 bit: start of packet | 1 bit: end of packet | 2 bits: packet type (data/ack/retransmission) |
//| 5 bits: reserverd |
int Modulator::modulate_data(const uint8_t *source_ptr, Tx_block &dest,
                             bool last_block_is_data, int FID, bool is_start_of_packet, bool is_end_of_packet, Packet_type packet_type,
                             int max_size) {
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::DATA, 0, parameters.parity);
    auto helper=dest.get_helper();

    if(max_size==-1)max_size=MAX_INT;

    //max block size = symbol number/3
    //max symbol number = 24*24 = 576
    static int MAX_BLOCK_SIZE=200;

    uint8_t buffer[MAX_BLOCK_SIZE];


    //first: encode header

    return 0;
}

void Modulator::modulate_probe(const Tx_PHY_probe &probe, Tx_block &dest) {

}

void Modulator::modulate_action(const Tx_PHY_action &action, Tx_block &dest) {

}
