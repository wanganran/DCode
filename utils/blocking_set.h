//
// Created by Anran on 15/6/6.
// First version finished on 15/6/10.
//

#ifndef DCODE_BLOCKING_SET_H
#define DCODE_BLOCKING_SET_H
#include <mutex>
template<typename T>
class Blocking_set{
private:
    int capacity_;
    T* set_;
    bool* set_available_;
    int count_;
    std::mutex mutex_;
    std::condition_variable cv_full,cv_empty;
public:
    Blocking_set(int cap):capacity_(cap),set_(new T[capacity_]), set_available_(new bool[capacity_]),count_(0){
        for(int i=0;i<cap;+i++)set_available_[i]=false;
    }
    ~Blocking_set(){delete[] set_;delete[] set_available_;}
    T pop(){
        std::unique_lock<std::mutex> lock(mutex_);
        while(count_==0)cv_empty.wait(lock);
        T res;
        for(int i=0;i<capacity_;i++)
            if(set_available_[i]){
                set_available_[i]=false;
                res=set_[i];
                break;
            }
        count_--;
        cv_full.notify_one();
        return res;
    }
    void push(T val){
        std::unique_lock<std::mutex> lock(mutex_);
        while(count_==capacity_)cv_full.wait(lock);
        for(int i=0;i<capacity_;i++)
            if(!set_available_[i]){
                set_available_[i]=true;
                set_[i]=val;
                break;
            }
        count_++;
        cv_empty.notify_one();
    }
};
#endif //DCODE_BLOCKING_SET_H
