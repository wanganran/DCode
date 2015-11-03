//
// Created by 王安然 on 15/8/3.
//

#include "physical_test.h"
#include "physical.h"
#include "../locator_scanner.h"
#include <iostream>

using namespace std;

void physical_test(){
    Config* config=Config::current();
    Camera_fetcher* fetcher=new Camera_fetcher(3);
    Physical* phy=new Physical(config,nullptr,fetcher);

    auto YUV_path="/Users/wanganran/test.yuv";
    uint8_t* arr=new uint8_t[config->hardware_config.rx_height*config->hardware_config.rx_width*3/2];
    auto f=fopen(YUV_path,"rb");
    fread(arr,sizeof(uint8_t),config->hardware_config.rx_height*config->hardware_config.rx_width*3/2,f);
    fclose(f);
    fetcher->assign(&arr,1);
    fetcher->ready(fetcher->lock_one());

    Pixel_reader* reader=phy->fetch_from_camera();
    cout<<"..."<<endl;
    Locator_scanner* scanner=new Locator_scanner(reader,new Palette());
    Locator_scanner::Locator_scanner_result results[200];

    long long ts=chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    scanner->locate_multiple(results,200);

    cout<<chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()-ts<<endl;


    ts=chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    for(int i=0;i<200;i++)
        if(results[i].success){
            results[i]=scanner->track_single(results[i],7);
        }

    cout<<chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()-ts<<endl;

    cout<<"====\n";
    for(int i=0;i<200;i++){
        if(results[i].success){
            cout<<results[i].center_x<<", "<<results[i].center_y<<":\t"
                <<results[i].color_black.to_string()<<", "<<results[i].color_white.to_string()<<";\t"
                <<results[i].estimated_symbol_size_x<<", "<<results[i].estimated_symbol_size_y<<";\t"
                <<results[i].intrinsic<<", "<<results[i].likelihood<<endl;
        }
    }
}