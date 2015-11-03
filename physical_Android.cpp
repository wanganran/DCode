//
// Created by Anran on 15/5/30.
//First version finished on 15/6/10.
//

#include "physical.h"
#include "screen_fetcher_Android.h"
#include "camera_fetcher_Android.h"
#include "pixel_reader_Android.h"
#include "screen_painter_Android.h"

Physical::Physical(Config& config, Screen_fetcher* s_fetcher, Camera_fetcher* c_fetcher):
rx_width_(config.hardware_config.rx_width),
rx_height_(config.hardware_config.rx_height),
tx_width_(config.hardware_config.tx_width),
tx_height_(config.hardware_config.tx_height),
s_fetcher_(s_fetcher),
c_fetcher_(c_fetcher)
{
}

Pixel_reader *Physical::fetch_from_camera() {
    if(!c_fetcher_)return nullptr;
    return new Pixel_reader(rx_width_,rx_height_,c_fetcher_->get_frame());
}

void Physical::free_to_camera(Pixel_reader *reader) {
    if(!reader)return;
    if(c_fetcher_)
        c_fetcher_->release_frame(reader->data_);
    delete reader;
}

Screen_painter *Physical::get_screen_painter(){
    if(!s_fetcher_)return nullptr;
    return new Screen_painter(s_fetcher_->fetch_buffer(),tx_width_,tx_height_);
}

void Physical::submit_screen_painter(Screen_painter *mem) {
    if(!mem)return;
    if(s_fetcher_)
        s_fetcher_->push_buffer(mem->ptr_);
    delete mem;
}
