//
// Created by 王安然 on 15/11/5.
//

#ifndef DCODE_HAMMING128_H
#define DCODE_HAMMING128_H

#include <stdint.h>
#include <stdio.h>
#include "utils.h"

class Hamming128:public Noncopyable{
private:
    uint8_t table[4096];
    int code[256];
public:
    Hamming128(const char* path_table, const char* path_code){
        FILE* fp=fopen(path_table, "r");
        for(int i=0;i<4096;i++) {
            int t;
            fscanf(fp, "%d", &t);
            table[i]=(uint8_t)t;
        }
        fclose(fp);

        fp=fopen(path_code, "r");
        for(int i=0;i<256;i++) {
            int t;
            fscanf(fp, "%d", &t);
            code[i]=t;
        }
        fclose(fp);
    }

    uint8_t decode(bool* ptr) const{
        int idx=0;
        for(int i=0;i<12;i++)
            idx|=((ptr[i]?1:0)>>i);
        return table[idx];
    }

    void encode(uint8_t src, bool* dest) const{
        int res=code[src];
        for(int i=0;i<12;i++)
            dest[i]=((res>>i)&1)>0;
    }

    static Hamming128& get_shared(){
        static Hamming128 singleton("hamming128_table.txt","hamming128_code.txt");
        return singleton;
    }
};

#endif //DCODE_HAMMING128_H
