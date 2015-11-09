//
// Created by 王安然 on 15/8/7.
//

#include "modulator.h"
#include "utils/hamming128.h"

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
static int _redundancy(FEC_level level, int total_in_byte){
    //percentage
    static const int ratios[]={90,70,60};
    return ratios[(int)level]*total_in_byte/100;
}
static int _calc_msg_size(FEC_level level_pri, FEC_level level_sec, int total_symbol_count, int sec_mask,
                          int& out_redund_size){
    //primary
    int pri_size=total_symbol_count/3;
    int bit_per_symbol=(sec_mask&1)+((sec_mask>>1)&1)+((sec_mask>>2)&1);
    int bits=total_symbol_count*bit_per_symbol;
    //no inner FEC, or inner 12,8 code
    int sec_size=(level_sec==FEC_level::HIGH?(bits/8):(bits/12));

    out_redund_size=_redundancy(level_pri, pri_size+sec_size);
    return pri_size+sec_size;
}

//Data packet Link layer format:
//| 1 bit: last_is_data | 6 bits: FID |
//| 1 bit: start of packet | 1 bit: end of packet | 2 bits: packet type (data/ack/retransmission) |
//| 5 bits: reserved |
int Modulator::modulate_data(const uint8_t *source_ptr, Tx_block &dest,
                             bool last_block_is_data, int FID, bool is_start_of_packet, bool is_end_of_packet, Packet_type packet_type,
                             int max_size) {
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::DATA, 0, parameters.parity);
    auto helper=dest.get_helper();

    if(max_size==-1)max_size=MAX_INT;

    //max block size in both channels = symbol number/3 + symbol number*3/8
    //max symbol number = 24*24 = 576
    //message limit: 256
    static int MAX_BLOCK_SIZE=256;

    uint8_t buffer[MAX_BLOCK_SIZE];


    //first: encode header
    buffer[0]=(uint8_t)((last_block_is_data?128:0) | ((FID&63)<<1) | (is_start_of_packet?1:0));
    buffer[1]=(uint8_t)((is_end_of_packet?128:0) | (((int)packet_type)<<5));

    //second: calculate
    int k;
    int n=_calc_msg_size(parameters.FEC_strength_primary, parameters.FEC_strength_secondary, helper->get_total_symbol_count(),parameters.color_sec_mask, k);
    auto& coder=coder_buffered_.get_coder(n,k);

    //third: encode
    unsigned int length=(unsigned)min(max_size, n-k-2);
    memcpy(buffer+2, source_ptr, length);
    coder->encode(buffer);

    //fourth: modulate
    auto& hamming=Hamming128::get_shared();
    for(int i=0;i<helper->get_total_symbol_count();i++){
        helper->push_primary(buffer[i]);
    }
    bool sec_buffer[12];
    if(parameters.FEC_strength_secondary==FEC_level::HIGH){
        for(int j=0;)
    }
    return length;
}

void Modulator::modulate_probe(const Tx_PHY_probe &probe, Tx_block &dest) {

}

void Modulator::modulate_action(const Tx_PHY_action &action, Tx_block &dest) {

}
