//
// Created by 王安然 on 16/2/1.
//

#include "structures.h"
#include "tx_buffer.h"

#ifndef DCODE_ADAPTATION_H
#define DCODE_ADAPTATION_H

#endif //DCODE_ADAPTATION_H

class Adaptation_unit{
public:
    void input_probe(const Rx_PHY_probe_result& probe);
    void input_availability(const Rx_frame_availability& avail);
    void input_error_analysis(const Rx_frame_error_rate& err);

    Adaptation_unit(Tx_buffer* tx);
};