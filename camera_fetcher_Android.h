//
// Created by Anran on 15/6/6.
// First version finished on 15/6/10.
//

#ifndef DCODE_CAMERA_FETCHER_ANDROID_H
#define DCODE_CAMERA_FETCHER_ANDROID_H

#include <stdint.h>
#include <map>
#include "utils/blocking_queue.h"

class Camera_fetcher_Android {
private:
    uint8_t** arrays_;
    int capacity_;
    Blocking_queue<int> buffer_;
    Blocking_queue<uint8_t*> queue_;
public:
    Camera_fetcher_Android(int cap);

    //to JNI
    void assign(uint8_t** arrays,int count);
    int lock_one();
    void ready(int id);

    //to Physical
    uint8_t* get_frame();
    void release_frame(uint8_t*);
};

#endif //DCODE_CAMERA_FETCHER_ANDROID_H
