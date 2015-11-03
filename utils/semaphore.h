//
// Created by Anran on 15/6/4.
// First version finished on 15/6/10.
//

#ifndef DCODE_SEMAPHORE_H
#define DCODE_SEMAPHORE_H

#include <mutex>
#include <condition_variable>

class Semaphore{
private:
    int limit_;
    std::mutex mutex_;
    std::condition_variable condition1_,condition2_;
    int curr_;
public:
    Semaphore(int limit, int curr=0):limit_(limit), curr_(curr){}
    void acquire(){
        std::unique_lock<std::mutex> lock(mutex_);
        while(curr_==0)condition1_.wait(lock);
        curr_--;
        condition2_.notify_one();
    }
    void release(){
        std::unique_lock<std::mutex> lock(mutex_);
        while(curr_==limit_)condition2_.wait(lock);
        curr_++;
        condition1_.notify_one();
    }
    int get(){
        return curr_;
    }
};

#endif //DCODE_SEMAPHORE_H
