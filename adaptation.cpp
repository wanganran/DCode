//
// Created by 王安然 on 16/2/1.
//

#include "adaptation.h"
#include "palette.h"

//probe isn't guaranteed correct, so it can only be used for noise est. etc.
void Adaptation_unit::input_probe(const Rx_PHY_probe_result &probe) {

}

void Adaptation_unit::input_availability(const Rx_frame_availability &avail) {

}

void Adaptation_unit::input_error_analysis(const Rx_frame_error_rate &err) {

}

bool Adaptation_unit::fetch_action(Tx_PHY_action &out_action) {
    return false;
}

void Adaptation_unit::input_valid_data_block(const Rx_block &block) {

}
