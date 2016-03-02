//
// Created by 王安然 on 16/2/1.
//

#include "adaptation.h"
#include "palette.h"

//probe isn't guaranteed correct, so it can only be used for:
//noise: analyzing the mean and variance of colors -> palette, modulation depth
//blurring -> block size
//rough error rate. -> secondary modulation
void Adaptation_unit::input_probe(const Rx_PHY_probe_result &probe) {

    //judge for the rough error rates
    auto pri_e = probe.error_estimate_pri == Error_estimate::HIGH ? 1 : 0;

    if(pri_e){
        //this probe must be broken. ignore it.
        return;
    }
    //analyze the secondary channels
    sec_err_estimate_[0].push_back(probe.error_estimate_sec[0]);
    sec_err_estimate_[1].push_back(probe.error_estimate_sec[1]);
    sec_err_estimate_[2].push_back(probe.error_estimate_sec[2]);

    //try to adjust the colors
    for(int i=0;i<64;i++)
        for(int j=0;j<9;j++)
            current_analyzer_.add(i,probe.received_probe_colors[i][j]);

}

void Adaptation_unit::input_availability(const Rx_frame_availability &avail) {
    //TODO: currently not yet implemented
}

//this method is to analyze the error rates of different blocks.
//to further make a reasonable FEC rate -> primary&secondary FEC rate
void Adaptation_unit::input_error_analysis(const Rx_frame_error_rate &err) {
    accu_frame_err_rate_.total_raw_frames_received+=err.total_raw_frames_received;
    int w=Config::current().barcode_config.horizontal_block_count;
    int h=Config::current().barcode_config.vertical_block_count;
    for(int i=0;i<w;i++)
        for(int j=0;j<h;j++) {
            auto& arr = accu_frame_err_rate_.get_BER(i, j);
            auto& to_insert = err.get_BER(i, j);
            arr.insert(arr.end(), to_insert.begin(), to_insert.end());
        }
}

//determine data is enough for decision
bool Adaptation_unit::fetch_action(Tx_PHY_action &out_action) {
    return false;
}

//this method is to analyze the actual errors of a block
//it will randomly skip some blocks for efficiency.
//it will analyze which color is more likely to be erroneous,
//and adjust the actual palette colors
void Adaptation_unit::input_valid_data_block(const Rx_block &block) {

}
