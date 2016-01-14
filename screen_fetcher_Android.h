//
// Created by Anran on 15/6/4.
// First version finished on 15/6/10.
//

#ifndef DCODE_SCREEN_FETCHER_ANDROID_H
#define DCODE_SCREEN_FETCHER_ANDROID_H

#include <stdint.h>
#include "utils/blocking_queue.h"
#include "config.h"
#include "utils/RGB.h"


class Screen_fetcher_Android {
private:
    const int QUEUE_SIZE;
    int array_size_;
    int width_;
    int height_;

    Blocking_queue<uint8_t*> queue_;
    uint8_t** buffer_array_;
    Blocking_queue<uint8_t*> buffer_;
public:

    Screen_fetcher_Android();
    ~Screen_fetcher_Android();

    //to JNI
    bool is_flushed();
    void paint(uint8_t* destination);

    //to Physical
    uint8_t * fetch_buffer();
    void push_buffer(uint8_t *);
};

#endif //DCODE_SCREEN_FETCHER_ANDROID_H
