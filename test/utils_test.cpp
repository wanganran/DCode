//
// Created by 王安然 on 15/8/3.
//

#include "utils_test.h"
#include "../utils/blocking_queue.h"
#include "../utils/constants.h"
#include "../utils/palette.h"
#include <thread>

void utils_test(){
    Blocking_queue<int> queue(5);
    std::thread th([&queue](){
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        int r=queue.pop();
        printf("Pop: %d\n",r);
    });
    for(int i=0;i<6;i++){
        queue.push(i);
        printf("OK: %d\n",i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    printf("----\n");

    LOCALCONST(INITIAL_BLACK_WHITE_THRES,int);

    printf("Const: %d\n", INITIAL_BLACK_WHITE_THRES);

    printf("----\n");

    Palette p;
    auto matcher=p.get_matcher();
    auto d=matcher->match(RGB(10,20,160));
    printf("Matcher: %d\n",d);

    auto adjuster=p.get_adjuster();
    adjuster->enable_secondary(0,40,180);
    int thres=adjuster->get_threshold_primary(0);
    adjuster->set_threshold_primary(0,thres+1);

    matcher=p.get_matcher();
    auto d2=matcher->match(RGB(50,20,160));
    auto d3=matcher->match_primary(RGB(50,20,160));
    printf("Adjuster: %d %d, %d %d\n",thres, adjuster->get_threshold_primary(0), d2,d3);

    auto analyzer=p.get_analyzer();
    analyzer->add(0,RGB(0,0,1));
    analyzer->add(0,RGB(10,10,2));
    analyzer->add(0,RGB(4,7,3));
    double var,mean;
    analyzer->get_stat(0,7,0,mean,var);
    printf("Analyzer: %llf %llf\n",var,mean);

    printf("----\n");

    th.join();
}