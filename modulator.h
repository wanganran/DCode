//
// Created by 王安然 on 15/8/7.
//

#ifndef DCODE_MODULATOR_H
#define DCODE_MODULATOR_H

#include <memory>
#include "palette.h"
#include "utils/utils.h"
#include "structures.h"
#include "symbol_scanner.h"
#include "structures.h"
#include "adaptive_parameters.h"
#include "utils/escaper.h"
#include "utils/reed_solomon_code.h"

class Modulator: public Noncopyable {
private:
    Escaper escaper_;
    Reed_solomon_code_buffered coder_buffered_;
public:
    void modulate_idle(Tx_block& dest);

    //need some extra info: last block is data? frame ID? is start/end of packet? current packet type?
    int modulate_data(const uint8_t* source_ptr, Tx_block& dest,
                      bool last_block_is_data, int FID, bool is_start_of_packet, bool is_end_of_packet, Packet_type packet_type,
                      int max_size=-1);

    //Tx->Rx
    void modulate_probe(const Tx_PHY_probe& probe, Tx_block& dest);

    //Rx->Tx
    void modulate_action(const Tx_PHY_action& action, Tx_block& dest);
};

class Demodulator: public Noncopyable {
public:
    Option<Block_type> get_block_type(Symbol_scanner::Block_content& src);
    bool demodulate_data(Symbol_scanner::Block_content& src,
                         uint8_t* data_dest, int& out_len, int& missed_len);
    bool demodulate_probe(Symbol_scanner::Block_content& src,
                          Rx_PHY_probe_result& probe_dest);
    bool demodulate_action(Symbol_scanner::Block_content& src,
                           Rx_PHY_action_result& action_dest);
};

class Modulator_factory{
public:
    static Modulator& get_modulator(){
        static Modulator singleton;
        return singleton;
    }
    static Demodulator& get_demodulator(){
        static Demodulator singleton;
        return singleton;
    }

};
#endif //DCODE_DEMODULATOR_H
