//
// Created by Anran on 15/6/6.
// First version finished on 15/6/10.
//
#include <string.h>
#include "screen_fetcher_Android.h"
#include "utils/constants.h"
Screen_fetcher_Android::Screen_fetcher_Android():
        array_size_(Config::current()->hardware_config.tx_height*Config::current()->hardware_config.tx_width*3),
        QUEUE_SIZE(CONST(SCREEN_PAINTER_QUEUE_SIZE,int)),
        queue_(QUEUE_SIZE),
        buffer_(QUEUE_SIZE),
        buffer_array_(new uint8_t*[QUEUE_SIZE])
{
    for(int i=0;i<QUEUE_SIZE;i++) {
        buffer_array_[i] = new uint8_t[array_size_];
        buffer_.push(buffer_array_[i]);
    }
}
Screen_fetcher_Android::~Screen_fetcher_Android() {
    for(int i=0;i<QUEUE_SIZE;i++)
        delete[] buffer_array_[i];
    delete[] buffer_array_;
}
uint8_t* Screen_fetcher_Android::fetch_buffer() {
    return buffer_.pop();
}

void Screen_fetcher_Android::push_buffer(uint8_t *t) {
    queue_.push(t);
}

void Screen_fetcher_Android::paint(uint8_t *destination) {
    uint8_t* ptr=queue_.pop();
    memcpy(destination,ptr,array_size_*sizeof(uint8_t));
    buffer_.push(ptr);
}
