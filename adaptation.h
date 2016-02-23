//
// Created by 王安然 on 16/2/1.
//

#include "structures.h"
#include "palette.h"

#ifndef DCODE_ADAPTATION_H
#define DCODE_ADAPTATION_H

#endif //DCODE_ADAPTATION_H

//this class is to receive probes, error stats, block availabilities, etc.
//and output actions.
class Adaptation_unit{
private:
    int last_action_id_;
    std::unique_ptr<Palette::Analyzer> current_analyzer_;

    Adaptation_unit():last_action_id_(-1){}
public:
    void input_probe(const Rx_PHY_probe_result& probe);
    void input_valid_data_block(const Rx_block& block);
    void input_availability(const Rx_frame_availability& avail);
    void input_error_analysis(const Rx_frame_error_rate& err);

    bool fetch_action(Tx_PHY_action& out_action);

    static Adaptation_unit& shared(){
        static Adaptation_unit singleton;
        return singleton;
    }
};