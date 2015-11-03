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


class Modulator{
public:
    void modulate_idle(Block& dest);
    std::pair<int, int> modulate_data(const uint8_t* source_pri, const uint8_t& source_sec, Block& dest);

    //Tx->Rx
    void modulate_probe(const PHY_probe& probe, Block& dest);

    //Rx->Tx
    void modulate_action(const PHY_action& action, Block& dest);
};

class Demodulator{
public:

    Option<Block_type> demodulate(Symbol_scanner::Block_content& src,
                                  uint8_t* data_dest_pri, int& out_len_pri,
                                  uint8_t* data_dest_sec, int& out_len_sec,
                                  PHY_probe_result& probe_dest,
                                  PHY_action& action_dest
    );
};

class Modulator_factory{

private:
    Palette* palette_;
public:
    Modulator_factory(const char* config_file_path);

    Modulator* get_modulator(PHY_config);

};
#endif //DCODE_DEMODULATOR_H
