//
// Created by Anran on 15/6/6.
// First version finished on 15/6/10.
//

#ifndef DCODE_BLOCKING_QUEUE_H
#define DCODE_BLOCKING_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class Blocking_queue{
private:
    T* array_;
    volatile int head_, tail_;
    volatile int count_;
    int capacity_;
    std::mutex lock_;
    std::condition_variable cv_full_,cv_empty_;
public:
    Blocking_queue(int cap):array_(new T[cap]),head_(0),tail_(0),count_(0),capacity_(cap) {}
    ~Blocking_queue(){if(array_)delete[] array_;}
    T pop(){
        std::unique_lock<std::mutex> lock(lock_);
        while(count_==0)cv_empty_.wait(lock);
        T res=array_[head_];
        head_=(head_+1)%capacity_;
        count_--;
        cv_full_.notify_one();
        return res;
    }
    T head(){
        std::unique_lock<std::mutex> lock(lock_);
        while(count_==0)cv_empty_.wait(lock);
        return array_[head_];
    }
    bool is_empty(){
        return count_==0;
    }
    void push(const T val){
        std::unique_lock<std::mutex> lock(lock_);
        while(count_==capacity_)cv_full_.wait(lock);
        int old_tail=tail_;
        tail_=(tail_+1)%capacity_;
        count_++;
        array_[old_tail]=val;
        cv_empty_.notify_one();
    }
};
#endif //DCODE_BLOCKING_QUEUE_H
