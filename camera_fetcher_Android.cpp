//
// Created by Anran on 15/6/7.
// First version finished on 15/6/10.
//

#include <assert.h>
#include "camera_fetcher_Android.h"

Camera_fetcher_Android::Camera_fetcher_Android(int cap):queue_(cap),buffer_(cap),arrays_(NULL),capacity_(0) { }

void Camera_fetcher_Android::assign(uint8_t **arrays, int count) {
    arrays_=arrays;
    capacity_=count;
    for(int i=0;i<count;i++){
        buffer_.push(i);
    }
}

int Camera_fetcher_Android::lock_one(){
    return buffer_.pop();
}

void Camera_fetcher_Android::ready(int id) {
    queue_.push(arrays_[id]);
}

unsigned char *Camera_fetcher_Android::get_frame() {
    return queue_.pop();
}

void Camera_fetcher_Android::release_frame(uint8_t * t) {
    for(int i=0;i<capacity_;i++)
        if(arrays_[i]==t){
            buffer_.push(i);
            return;
        }
}
