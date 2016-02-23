//
// Created by Anran on 15/6/15.
//

#include <math.h>
#include <stdlib.h>
#include <queue>
#include "locator_scanner.h"
#include "utils/constants.h"

using namespace std;

Locator_scanner::Locator_scanner(Pixel_reader* reader):reader_(reader) {
    _update_matcher();
}

inline static int _limit(const int& x, int min, int max){return x<min?min:(x>max?max:x);}
bool Locator_scanner::locate_single_primary(Locator_scanner_result& out_result, int center_x, int center_y, const Locator_scanner_result& reference) {
    auto width = reader_->get_width();
    auto height = reader_->get_height();

    int t = reference.center_y - (int) (reference.estimated_symbol_size_y * 3);
    int b = reference.center_y + (int) (reference.estimated_symbol_size_y * 3);
    int l = reference.center_x - (int) (reference.estimated_symbol_size_x * 5);
    int r = reference.center_x + (int) (reference.estimated_symbol_size_x * 5);
    _limit(l, 0, width);
    _limit(r, 0, width);
    _limit(t, 0, height);
    _limit(b, 0, height);

    return locate_single_primary(out_result, l, r, t, b);

}

bool Locator_scanner::locate_single_primary(Locator_scanner_result& out_result, int l, int t, int r, int b) {
    //color_b_w_b_w_b_color
    auto width=reader_->get_width();
    auto height=reader_->get_height();
    _limit(l,0,width);
    _limit(r,l,width);
    _limit(t,0,height);
    _limit(b,t,height);

    Locator_scanner_result curr;
    double max_likelihood=-1;
    for(int i=t;i<b;i+=2){
        int _l=l;
        if((_l=_do_one_direction_primary(i,_l,r,false,curr,nullptr))!=r+1){
            if(curr.likelihood>max_likelihood){
                out_result=curr;
                max_likelihood=curr.likelihood;
            }
        }
    }
    if(max_likelihood>=0)return true;
    else return false;

}

bool Locator_scanner::track_single_primary(Locator_scanner_result & ref_result) {
    //assuming the latest center point is still inside the locator range
    auto width=reader_->get_width();
    auto height=reader_->get_height();

    int t=ref_result.center_y-(int)(ref_result.estimated_symbol_size_y*3);
    int b=ref_result.center_y+(int)(ref_result.estimated_symbol_size_y*3);
    int l=ref_result.center_x-(int)(ref_result.estimated_symbol_size_x*5);
    int r=ref_result.center_x+(int)(ref_result.estimated_symbol_size_x*5);
    _limit(l,0,width);
    _limit(r,0,width);
    _limit(t,0,height);
    _limit(b,0,height);

    return locate_single_primary(ref_result,l,r,t,b);
}

bool Locator_scanner::locate_single_secondary(Locator_scanner_result& out_result, int approx_x, int approx_y, const Locator_scanner_result& reference){
    auto width=reader_->get_width();
    auto height=reader_->get_height();

    _limit(approx_x,0,width);
    _limit(approx_y,0,height);

    bool fast=_y_is_black_or_white(reader_->get_brightness(approx_x,approx_y));

    int tb_scale=fast?1:3;
    int lr_scale=fast?2:3;

    int t=approx_y-(int)(reference.estimated_symbol_size_y*tb_scale);
    int b=approx_y+(int)(reference.estimated_symbol_size_y*tb_scale);
    int l=approx_x-(int)(reference.estimated_symbol_size_x*lr_scale);
    int r=approx_x+(int)(reference.estimated_symbol_size_x*lr_scale);
    _limit(l,0,width);
    _limit(r,0,width);
    _limit(t,0,height);
    _limit(b,0,height);

    double max_likelihood=-1;
    Locator_scanner_result curr;
    for(int i=t;i<b;i+=2){
        int _l=l;
        if((_l=_do_one_direction_secondary(i,_l,r,false,curr,reference))!=r+1){
            if(curr.likelihood>max_likelihood){
                out_result=curr;
                max_likelihood=curr.likelihood;
            }
        }
    }
    if(max_likelihood>=0)return true;
    else return false;
}

int Locator_scanner::locate_multiple_primary(
        Locator_scanner::Locator_scanner_result *result, int maximum_count) {
    return locate_multiple_primary(result,maximum_count,0,0,reader_->get_width(),reader_->get_height());
}

int Locator_scanner::locate_multiple_primary(
        Locator_scanner::Locator_scanner_result *result, int maximum_count, int l, int t, int r, int b) {
    int k=0;
    for(int i=t;i<b;i+=2){
        int s=l;
        while(k<maximum_count&&((s=_do_one_direction_primary(i,s,r,false,result[k],nullptr))!=r+1)){
            //check duplicate
            int j;
            bool flag=false;
            for(j=0;j<k;j++){
                if(abs(result[j].center_x-result[k].center_x)+abs(result[j].center_y-result[k].center_y)<result->estimated_symbol_size_x+result->estimated_symbol_size_y) {
                    //check likelihood
                    if(result[k].likelihood>result[j].likelihood){
                        //replace
                        result[j]=result[k];
                    }
                    flag = true;
                    break;
                }
            }
            if(!flag)k++;
            else return false;
        }
        if(k==maximum_count)return k;
    }
    return k;
}
template<int L>
inline static void shift_2(std::array<int, L> data){
    for(int i=0;i<L-2;i++)
        data[i]=data[i+2];
}

static inline int manhattan_dist(RGB c1, RGB c2){
    return abs(c1.R-c2.R)+abs(c1.G-c2.G)+abs(c1.B-c2.B);
}
static inline RGB avg_RGB(RGB c1, RGB c2){
    return RGB((uint8_t)((c1.R+c2.R)/2),(uint8_t)((c1.G+c2.G)/2),(uint8_t)((c1.B+c2.B)/2));
}
static inline RGB avg_RGB(RGB c1,RGB c2,RGB c3){
    return RGB((uint8_t)((c1.R+c2.R+c3.R)/3),(uint8_t)((c1.G+c2.G+c3.G)/3),(uint8_t)((c1.B+c2.B+c3.B)/3));
}


bool Locator_scanner::_check_corner_scan_primary(int k, const Scanned_boundaries_primary_& boundaries, bool k_is_x, float& likelihood,
                                         const Locator_scanner_result* reference,
                                         RGB& out_black, RGB& out_white) {
    //first check the distances
    LOCALCONST(WHITE_DISTORTION_THRES_MIN, double);
    LOCALCONST(WHITE_DISTORTION_THRES_MAX, double);
    LOCALCONST(BLACK_DISTORTION_THRES_MIN, double);
    LOCALCONST(BLACK_DISTORTION_THRES_MAX, double);
    LOCALCONST(SAME_DISTORTION_THRES, double);
    LOCALCONST(CONTIGUOUS_DISTORTION_THRES, double);

    static const int SYMBOL_SIZE_MAX=(int)(min(reader_->get_width(),reader_->get_height())*CONST(SYMBOL_SIZE_MAX_RATIO,double));
    static const int SYMBOL_SIZE_MIN=CONST(SYMBOL_SIZE_MIN_PIXEL,int);

    auto from=boundaries;

    //constraint 1: not too long or short
    double avg=_symbol_size_from_boundary(boundaries);
    if(reference){
        double ref_len=k_is_x?reference->estimated_symbol_size_y:reference->estimated_symbol_size_x;
        if(avg>=(1+CONTIGUOUS_DISTORTION_THRES)*ref_len||avg<(1-CONTIGUOUS_DISTORTION_THRES)*ref_len)return false;
    }
    else
        if(avg<SYMBOL_SIZE_MIN||avg>SYMBOL_SIZE_MAX)return false;


    //constraint 2: ratio should be reasonable
    int len1=from[1]-from[0];
    int len2=from[2]-from[1];
    int len3=from[3]-from[2];
    if(len1<WHITE_DISTORTION_THRES_MIN*avg||
       len1>WHITE_DISTORTION_THRES_MAX*avg||
       len2<BLACK_DISTORTION_THRES_MIN*avg||
       len2>BLACK_DISTORTION_THRES_MAX*avg||
       len3<WHITE_DISTORTION_THRES_MIN*avg||
       len3>WHITE_DISTORTION_THRES_MAX*avg){
        return false;
    }

    //constraint 3: same color should have similar length
    if(abs(len1-len3)>SAME_DISTORTION_THRES*(len1+len3)/2.0)return false;

    //update likelihood: ratio balance
    if(!reference) {
        int var_len = abs(len1 - avg) + abs(len2 - avg) + abs(len3 - avg);
        likelihood *= (1 - var_len / avg / 3);
    }
    else {
        auto ll_rat = _symbol_size_from_boundary(boundaries) /
                      (k_is_x ? reference->estimated_symbol_size_y : reference->estimated_symbol_size_x);
        if (ll_rat > 1)ll_rat = 1 / ll_rat;
        likelihood *= ll_rat;
    }

    //then check color
    LOCALCONST(SAME_COLOR_DIST_THRES,int);

    RGB sample1=k_is_x?reader_->get_RGB(k,from[0]+len1/2):reader_->get_RGB(from[0]+len1/2,k);
    RGB sample2=k_is_x?reader_->get_RGB(k,from[1]+len2/2):reader_->get_RGB(from[1]+len2/2,k);
    RGB sample3=k_is_x?reader_->get_RGB(k,from[2]+len3/2):reader_->get_RGB(from[2]+len3/2,k);
    int inferred_0=(int)(from[0]-avg);
    int inferred_4=(int)(from[3]+avg);
    if(inferred_0<0)return false;
    if(inferred_4>=(k_is_x?reader_->get_height():reader_->get_width()))return false;
    RGB sample0=k_is_x?reader_->get_RGB(k,inferred_0+(int)(avg/2)):reader_->get_RGB(inferred_0+(int)(avg/2),k);
    RGB sample4=k_is_x?reader_->get_RGB(k,inferred_4-(int)(avg/2)):reader_->get_RGB(inferred_4-(int)(avg/2),k);

    //constraint 1: black/white should be similar
    RGB avg_black=avg_RGB(sample0,sample2,sample4);
    auto dist0=manhattan_dist(sample0,avg_black);
    auto dist2=manhattan_dist(sample2,avg_black);
    auto dist4=manhattan_dist(sample4,avg_black);
    if(dist0>SAME_COLOR_DIST_THRES||
       dist2>SAME_COLOR_DIST_THRES||
       dist4>SAME_COLOR_DIST_THRES||(
            reference!=NULL&&manhattan_dist(reference->color_black,avg_black)>SAME_COLOR_DIST_THRES
    ))return false;


    RGB avg_white=avg_RGB(sample1,sample3);
    auto dist1=manhattan_dist(sample1,avg_white);
    auto dist3=manhattan_dist(sample3,avg_white);
    if(dist1>SAME_COLOR_DIST_THRES||
       dist3>SAME_COLOR_DIST_THRES||(
            reference!=NULL&&manhattan_dist(reference->color_white,avg_white)>SAME_COLOR_DIST_THRES
    ))return false;

    //constraint 2: they are black and white
    if(_palette_is_black_or_white(sample0)
        .transform([&sample1](bool bw){
            if(bw) return _palette_is_black_or_white(sample1);
            else return None<bool>();
        })
        .transform([&sample2](bool bw){
            if(!bw) return _palette_is_black_or_white(sample2);
            else return None<bool>();
        })
        .transform([&sample3](bool bw){
            if(bw) return _palette_is_black_or_white(sample3);
            else return None<bool>();
        })
        .transform([&sample4](bool bw){
            if(!bw) return _palette_is_black_or_white(sample4);
            else return None<bool>();
        })
        .transform([](bool bw){
            if(bw) return Some(true);
            else return None<bool>();
        })
        .empty())return false;

    //update likelihood: color similarity and difference
    auto var_color=dist0+dist1+dist2+dist3+dist4;
    likelihood*=(1-var_color/255.0/5);

    //update out_black and out_white
    out_black=avg_black;
    out_white=avg_white;
    return true;
}

bool Locator_scanner::_check_corner_scan_secondary(int k, const Scanned_boundaries_secondary_& boundaries, bool k_is_x, float& likelihood,
                                                 const Locator_scanner_result& reference, RGB& out_black, RGB& out_white){
    //first check the distances
    LOCALCONST(BLACK_DISTORTION_THRES_MIN, double);
    LOCALCONST(BLACK_DISTORTION_THRES_MAX, double);
    LOCALCONST(CONTIGUOUS_DISTORTION_THRES, double);

    double symbol_size_min_x=reference.black_symbol_size_x*(1+CONTIGUOUS_DISTORTION_THRES);
    double symbol_size_max_x=reference.black_symbol_size_x*(1-CONTIGUOUS_DISTORTION_THRES);
    double symbol_size_min_y=reference.black_symbol_size_y*(1+CONTIGUOUS_DISTORTION_THRES);
    double symbol_size_max_y=reference.black_symbol_size_y*(1-CONTIGUOUS_DISTORTION_THRES);


    //constraint 1: not too long or short
    int avg=boundaries[1]-boundaries[0];
    if((k_is_x && !IN(avg,symbol_size_min_y,symbol_size_max_y))||
       (!k_is_x && !IN(avg,symbol_size_min_x,symbol_size_max_x)))
        return false;

    //update likelihood
    auto ll_rat=(boundaries[1]-boundaries[0])/(k_is_x?reference.black_symbol_size_y:reference.black_symbol_size_x);
    if(ll_rat>1)ll_rat=1/ll_rat;
    likelihood*=ll_rat;

    //then check color
    LOCALCONST(SAME_COLOR_DIST_THRES,int);

    RGB sample_black=k_is_x?
                     reader_->get_RGB(k,(boundaries[0]+boundaries[1])/2):
                     reader_->get_RGB((boundaries[0]+boundaries[1])/2,k);
    int inferred_0=boundaries[0]-avg/2;
    int inferred_4=boundaries[1]+avg/2;
    if(inferred_0<0)return false;
    if(inferred_4>=(k_is_x?reader_->get_height():reader_->get_width()))return false;
    RGB sample_white_0=k_is_x?reader_->get_RGB(k,inferred_0+avg/2):reader_->get_RGB(inferred_0+avg/2,k);
    RGB sample_white_1=k_is_x?reader_->get_RGB(k,inferred_4-avg/2):reader_->get_RGB(inferred_4-avg/2,k);

    //constraint 1: black/white should be similar
    auto dist_black=manhattan_dist(sample_black,reference.color_black);
    if(dist_black>SAME_COLOR_DIST_THRES)
       return false;


    RGB avg_white=avg_RGB(sample_white_0,sample_white_1);
    auto dist_sim1=manhattan_dist(sample_white_0,avg_white);
    auto dist_sim2=manhattan_dist(sample_white_1,avg_white);
    auto dist_white=manhattan_dist(avg_white,reference.color_white);
    if(dist_sim1>SAME_COLOR_DIST_THRES||
       dist_sim2>SAME_COLOR_DIST_THRES||
       dist_white>SAME_COLOR_DIST_THRES)
        return false;

    //constraint 2: they are black and white
    if(_palette_is_black_or_white(sample_white_0)
        .transform([&sample_black](bool bw){
            if(!bw) return _palette_is_black_or_white(sample_black);
            else return None<bool>();
        })
        .transform([&sample_white_1](bool bw){
            if(bw) return _palette_is_black_or_white(sample_white_1);
            else return None<bool>();
        })
        .transform([](bool bw){
            if(!bw) return Some(true);
            else return None<bool>();
        })
        .empty())return false;

    //update likelihood: color similarity and difference
    auto var_color=dist_black+dist_white+dist_sim1+dist_sim2;
    likelihood*=(1-var_color/255.0/4);

    out_black=sample_black;
    out_white=avg_white;
    return true;
}
bool Locator_scanner::_find_other_primary(int k, int y, bool k_is_x,
                                  int estimated_symbol_size_min,int estimated_symbol_size_max,
                                  int& out_center, double& out_symbol_size, double& out_black_symbol_size, float& likelihood
) {
    Scanned_boundaries_primary_ from={0};
    std::array<int, from.size()-1> color={0};
    auto current=from.size()/2-1;
    color[current]=k_is_x?reader_->get_brightness(k,y):reader_->get_brightness(y,k);
    int state=_y_is_black_or_white(color[current])?0:1;
    for(int i=y-1;i>=0;i--){
        int Y=k_is_x?reader_->get_brightness(k,i):reader_->get_brightness(i,k);
        bool bw=_y_is_black_or_white(Y);
        if(state==0&&bw) //same color: black
            color[current]=min(color[current],Y);

        else if(state==1&&!bw) //same color: white
            color[current]=max(color[current],Y);

        else{
            state=1-state;
            from[current]=i+1;
            if(current!=0) {
                color[current - 1] = Y;
                current--;
            }
            else{
                //it is the start
                break;
            }
        }
    }
    current=from.size()/2-1;
    int limit=k_is_x?reader_->get_height():reader_->get_width();
    state=_y_is_black_or_white(color[current])?0:1;
    for(int i=y+1;i<limit;i++){
        int Y=k_is_x?reader_->get_brightness(k,i):reader_->get_brightness(i,k);
        bool bw=_y_is_black_or_white(Y);
        if(state==0&&bw) //same color: black
            color[current]=min(color[current],Y);

        else if(state==1&&!bw) //same color: white
            color[current]=max(color[current],Y);

        else{
            state=1-state;
            current++;
            from[current]=i+1;
            if(current!=from.size()-1) {
                color[current] = Y;
            }
            else{
                //it is the start
                break;
            }
        }
    }
    double symbol_size=(from[from.size()-1]-from[0])/(from.size()-1.0);
    RGB out_black, out_white;
    if(estimated_symbol_size_max>=symbol_size&&
       estimated_symbol_size_min<=symbol_size&&
       _check_corner_scan_primary(k,from,k_is_x,likelihood, NULL, out_black, out_white)) {
        out_center=(from[from.size()/2]-from[from.size()/2-1])/2;
        out_symbol_size=_symbol_size_from_boundary(from);
        out_black_symbol_size=from[2]-from[1];
        return true;
    }
    return false;
}
bool Locator_scanner::_find_other_secondary(int k, int y, bool k_is_x,
                             int& out_center, double& out_symbol_size, float& likelihood, const Locator_scanner_result& reference){

    Scanned_boundaries_secondary_ from={0};

    double max_symbol_size=k_is_x?reference.estimated_symbol_size_y*2:reference.estimated_symbol_size_x*2;
    from[0]=y-1; from[1]=y+1;

    int succ=0;
    while(from[0]>=0 && y-from[0]<max_symbol_size){
        auto Y=k_is_x?reader_->get_brightness(k,from[0]):reader_->get_brightness(from[0],k);
        if(!_y_is_black_or_white(Y)){
            succ++;
            break;
        }
        from[0]--;
    }

    int limit=k_is_x?reader_->get_height():reader_->get_width();
    while(from[1]<limit && from[1]-y<max_symbol_size){
        auto Y=k_is_x?reader_->get_brightness(k,from[1]):reader_->get_brightness(from[1],k);
        if(!_y_is_black_or_white(Y)){
            succ++;
            break;
        }
        from[1]++;
    }

    if(succ==2){
        RGB out_white, out_black;
        if(_check_corner_scan_secondary(k,from,k_is_x,likelihood,reference, out_black,out_white)){
            out_center=(from[1]+from[0])/2;
            out_symbol_size=(from[1]-from[0]);
            return true;
        }
    }
    return false;
}


#if 0
static inline bool check_intrinsic_color(RGB color, int& result){
    LOCALCONST(INTRINSIC_COLOR_DIFF_THRES,int);
    if(color.R-color.G>=INTRINSIC_COLOR_DIFF_THRES&&color.B-color.G>=INTRINSIC_COLOR_DIFF_THRES){
        result = 1;
        return true;
    }
    if(color.G-color.R>=INTRINSIC_COLOR_DIFF_THRES&&color.G-color.B>=INTRINSIC_COLOR_DIFF_THRES) {
        result = 0;
        return true;
    }
    return false;
}

bool Locator_scanner::_get_intrinsic_info(int center_x, int center_y, float symbol_size_x, float symbol_size_y,
                                          Locator_scanner_result::intrinsic_t &info) {
    int x_offset=(int)(symbol_size_x*3);
    int y_offset=(int)(symbol_size_y*3);
    RGB left=reader_->get_RGB(center_x-x_offset,center_y);
    RGB right=reader_->get_RGB(center_x+x_offset,center_y);
    RGB top=reader_->get_RGB(center_x,center_y-y_offset);
    RGB bottom=reader_->get_RGB(center_x,center_y+y_offset);

    //the total XOR should be zero
    //RGB can only be 101 (true) and 010 (false)
    //totally 3bit data.

    int leftv,rightv,topv,bottomv;
    if(check_intrinsic_color(left,leftv)&&
       check_intrinsic_color(right,rightv)&&
       check_intrinsic_color(top,topv)&&
       check_intrinsic_color(bottom,bottomv)){
        if(leftv+rightv+topv+bottomv==3||leftv+rightv+topv+bottomv==1){
            info.available=true;
            info.flipped=(leftv+rightv+topv+bottomv==3);

            int flipped_int=info.flipped?0:1;
            if(leftv==flipped_int)info.direction=Locator_scanner_result::intrinsic_t::LEFT;
            if(rightv==flipped_int)info.direction=Locator_scanner_result::intrinsic_t::RIGHT;
            if(topv==flipped_int)info.direction=Locator_scanner_result::intrinsic_t::TOP;
            if(bottomv==flipped_int)info.direction=Locator_scanner_result::intrinsic_t::BOTTOM;

            return true;
        }
    }
    return false;
}
#endif


int Locator_scanner::_do_one_direction_secondary(int k, int f, int e, bool k_is_x, Locator_scanner_result& result, const Locator_scanner_result& reference) {

    Scanned_boundaries_secondary_ from = {0};

    int _y;
    while(f<e&&_y_is_black_or_white(_y = (k_is_x ? reader_->get_brightness(k, f) : reader_->get_brightness(f, k))))f+=2;

    bool current_black=false;

    for (int i = f + 1; i < e; i+=2) {

        int Y = k_is_x ? reader_->get_brightness(k, i) : reader_->get_brightness(i, k);
        int bw = _y_is_black_or_white(Y);
        if (current_black && bw)  //same color: black
            _y = min(_y, Y);
        if ((current_black && !bw) || (!current_black && bw)) {//black->white || white->black
            int y_pre= k_is_x ? reader_->get_brightness(k, i-1) : reader_->get_brightness(i-1, k);
            bool bw_pre=_y_is_black_or_white(y_pre);
            if(bw==bw_pre)i--;

            current_black=!current_black;
            if (current_black) { //just meet
                from[0] = i;
                _y=Y;
            }
            else{
                from[1]=i;
                float likelihood = 1.0;
                int center_y;
                RGB out_black, out_white;
                double symbol_size_x=from[1]-from[0];
                double symbol_size_y;

                int center_x=(from[1]+from[0])/2;
                //just compare to old
                if (_check_corner_scan_secondary(k, from, k_is_x, likelihood, reference, out_black, out_white) &&
                    _find_other_secondary(center_x,
                                k,
                                !k_is_x,
                                center_y,
                                symbol_size_y,
                                likelihood,
                                reference

                    )) {
                    //success
                    if (k_is_x){
                        swap(center_x, center_y);
                        swap(symbol_size_x,symbol_size_y);
                    }

                    result.is_primary=false;
                    result.center_x = center_x;
                    result.center_y = center_y;
                    result.estimated_symbol_size_x = reference.estimated_symbol_size_x*symbol_size_x/reference.black_symbol_size_x;
                    result.estimated_symbol_size_y = reference.estimated_symbol_size_y*symbol_size_y/reference.black_symbol_size_y;
                    result.black_symbol_size_x=symbol_size_x;
                    result.black_symbol_size_y=symbol_size_y;
                    result.likelihood = likelihood;
                    result.color_black = out_black;
                    result.color_white = out_white;

                    return i + (from[1]-from[0])*2+1;

                }
                _y=Y;
            }
        }
    }
    return e + 1;
}



//assuming the legality of k, f, e, result has been checked
int Locator_scanner::_do_one_direction_primary(int k, int f, int e, bool k_is_x, Locator_scanner_result& result, const Locator_scanner_result* reference) {
    LOCALCONST(CONTIGUOUS_DISTORTION_THRES, double);

    Scanned_boundaries_primary_ from = {0};
    int from_id = 0;

    std::array<int, from.size()-1> color = {0};
    int color_id = 0;

#define __PUSH_COLOR(y) (color[color_id++]=y)
#define __COLOR_FULL() (color_id==from.size()-1)
#define __PUSH_FROM(x) (from[from_id++]=x)

    int _y;
    while (f<e&&_y_is_black_or_white(_y = (k_is_x ? reader_->get_brightness(k, f) : reader_->get_brightness(f, k))))f+=2;
    int state = 1;
    __PUSH_FROM(f);

    for (int i = f + 1; i < e; i+=2) {

        int Y = k_is_x ? reader_->get_brightness(k, i) : reader_->get_brightness(i, k);
        int bw = _y_is_black_or_white(Y);
        if (state == 0 && bw)  //same color: black
            _y = min(_y, Y);
        if (state == 1 && !bw) //same color: white
            _y = max(_y, Y);
        if ((state == 0 && !bw) || (state == 1 && bw)) {//black->white || white->black
            int y_pre= k_is_x ? reader_->get_brightness(k, i-1) : reader_->get_brightness(i-1, k);
            int bw_pre=_y_is_black_or_white(y_pre);
            if(bw==bw_pre)i--;

            state = 1 - state;
            __PUSH_COLOR(_y);
            __PUSH_FROM(i);
            if (__COLOR_FULL()) {
                float likelihood = 1.0;
                bool check = false;
                int out_center;
                double out_symbol_size,out_black_symbol_size;
                RGB out_black, out_white;

                double symbol_size_x=_symbol_size_from_boundary(from);
                double estimated_symbol_size = reference ?
                                               (k_is_x ? reference->estimated_symbol_size_x
                                                       : reference->estimated_symbol_size_y) :
                                               symbol_size_x;

                //just compare to old
                if (_check_corner_scan_primary(k, from, k_is_x, likelihood, reference, out_black, out_white) &&
                    _find_other_primary((from[from.size()/2-1] + from[from.size()/2]) / 2, //PATTERN_CHANGE must be even
                                k,
                                !k_is_x,
                                (int) (estimated_symbol_size * (1 - CONTIGUOUS_DISTORTION_THRES)),
                                (int) (estimated_symbol_size * (1 + CONTIGUOUS_DISTORTION_THRES)),
                                out_center,
                                out_symbol_size,
                                        out_black_symbol_size,
                                likelihood
                    )) {
                    //success
                    int center_x = (from[from.size()/2-1] + from[from.size()/2]) / 2;
                    int center_y = out_center;
                    if (k_is_x)swap(center_x, center_y);
                    double symbol_size_y = out_symbol_size;
                    if(k_is_x)swap(symbol_size_x,symbol_size_y);

                    result.is_primary=true;
                    result.center_x = center_x;
                    result.center_y = center_y;
                    result.estimated_symbol_size_x = symbol_size_x;
                    result.estimated_symbol_size_y = symbol_size_y;

                    result.black_symbol_size_x=from[2]-from[1];
                    result.black_symbol_size_y=out_black_symbol_size;
                    if(k_is_x)swap(result.black_symbol_size_x,result.black_symbol_size_y);

                    result.likelihood = likelihood;
                    result.color_black = out_black;
                    result.color_white = out_white;

                    return _limit((int)(i + symbol_size_x*5+1),f,e);

                }
                shift_2(color);
                shift_2(from);

                color_id -= 2;
                from_id -= 2;

                _y=Y;
            }
        }
    }
    return e + 1;
}

const int MAX_LOCATOR_BUFFER=20;

bool Locator_scanner::_guess_and_locate(const Locator_scanner::Locator_scanner_result& p1, const Locator_scanner::Locator_scanner_result& p2,
                        Option<Locator_scanner::Locator_scanner_result>& result, bool is_primary) {
    //first estimate the position
    Locator_scanner::Locator_scanner_result res;
    res.center_x=p2.center_x*2-p1.center_x;
    res.center_y=p2.center_y*2-p1.center_y;
    //second do the track
    if((!is_primary && locate_single_secondary(res, res.center_x,res.center_y, p2)) ||
            (is_primary && track_single_primary(res))){
        result=Some(std::move(res));
        return true;
    }
    else return false;
}
static bool is_primary_(int x, int y, int w, int h){
    return (x==w/2||x==w/2-1)&&(y==h/2||y==h/2-1);
}
void Locator_scanner::_find_adj(Mat<Option<Locator_scanner::Locator_scanner_result>>& mat, int x, int y, std::queue<Point>& queue) {
    if (!mat[x][y].empty()) {
        //fail safe
        Locator_scanner::Locator_scanner_result wont_used;

        #define CHECK_AND_PUSH(x1,y1,x2,y2,x3,y3) \
        if (_guess_and_locate(mat[x1][y1].get_reference_or(wont_used), mat[x2][y2].get_reference_or(wont_used),\
        mat[x3][y3], is_primary_(x3,y3,mat.get_width(),mat.get_height())))\
        queue.push(Point(x + 1, y));

        //right one
        if (x > 0 && !mat[x - 1][y].empty() && x < mat.get_width() - 1 && mat[x + 1][y].empty()) {
            CHECK_AND_PUSH(x-1,y,x,y,x+1,y)
        }
        //left one
        if (x > 0 && mat[x - 1][y].empty() && x < mat.get_width() - 1 && !mat[x + 1][y].empty()) {
            CHECK_AND_PUSH(x+1,y,x,y,x-1,y)
        }
        //bottom one
        if (y > 0 && !mat[x][y - 1].empty() && y < mat.get_height() - 1 && mat[x][y + 1].empty()) {
            CHECK_AND_PUSH(x,y-1,x,y,x,y+1)
        }
        //top one
        if (y > 0 && mat[x][y - 1].empty() && y < mat.get_height() - 1 && !mat[x][y + 1].empty()) {
            CHECK_AND_PUSH(x,y+1,x,y,x,y-1)
        }
        //right-bottom one
        if (y > 0 && x > 0 && !mat[x - 1][y - 1].empty() && y < mat.get_height() - 1 && x < mat.get_width() - 1 &&
            mat[x + 1][y + 1].empty()) {
            CHECK_AND_PUSH(x-1,y-1,x,y,x+1,y+1)
        }
        //left-top one
        if (y > 0 && x > 0 && mat[x - 1][y - 1].empty() && y < mat.get_height() - 1 && x < mat.get_width() - 1 &&
            !mat[x + 1][y + 1].empty()) {
            CHECK_AND_PUSH(x+1,y+1,x,y,x-1,y-1)
        }
        //right-top one
        if (y > 0 && x < mat.get_width() - 1 && mat[x + 1][y - 1].empty() && y < mat.get_height() - 1 && x < 0 &&
            !mat[x - 1][y + 1].empty()) {
            CHECK_AND_PUSH(x-1,y+1,x,y,x+1,y-1)
        }
        //left-bottom one
        if (y > 0 && x < mat.get_width() - 1 && !mat[x + 1][y - 1].empty() && y < mat.get_height() - 1 && x < 0 &&
            mat[x - 1][y + 1].empty()) {
            CHECK_AND_PUSH(x+1,y-1,x,y,x-1,y+1)
        }

        //next left one
        if (x > 1 && !mat[x - 1][y].empty() && mat[x - 2][y].empty()) {
            CHECK_AND_PUSH(x,y,x-1,y,x-2,y)
        }
        //next right one
        if (x < mat.get_width() - 2 && mat[x + 2][y].empty() && !mat[x + 1][y].empty()) {
            CHECK_AND_PUSH(x,y,x+1,y,x+2,y)
        }
        //next top one
        if (y > 1 && !mat[x][y - 1].empty() && mat[x][y - 2].empty()) {
            CHECK_AND_PUSH(x,y,x,y-1,x,y-2)
        }
        //next bottom one
        if (y < mat.get_height() - 2 && mat[x][y + 2].empty() && !mat[x][y + 1].empty()) {
            CHECK_AND_PUSH(x,y,x,y+1,x,y+2)
        }
        //next left-top one
        if (y > 1 && x > 1 && !mat[x - 1][y - 1].empty() &&
            mat[x - 2][y - 2].empty()) {
            CHECK_AND_PUSH(x,y,x-1,y-1,x-2,y-2)
        }
        //next right-bottom one
        if (y < mat.get_height() - 2 && x < mat.get_width() - 2 && mat[x + 2][y + 2].empty() &&
                                                                   !mat[x + 1][y + 1].empty()) {
            CHECK_AND_PUSH(x,y,x+1,y+1,x+2,y+2)
        }
        //next left-bottom one
        if (y < mat.get_height() - 2 && x > 1 && mat[x - 2][y + 2].empty() &&
            !mat[x - 1][y + 1].empty()) {
            CHECK_AND_PUSH(x,y,x-1,y+1,x-2,y+2)
        }
        //next right-top one
        if (y > 1 && x < mat.get_width() - 2 && !mat[x + 1][y - 1].empty() &&
            mat[x + 2][y - 2].empty()) {
            CHECK_AND_PUSH(x,y,x+1,y-1,x+2,y-2)
        }
    }
}


Option<Mat<Option<Locator_scanner::Locator_scanner_result>>> Locator_scanner::get_locator_net(int vcnt, int hcnt){

    //first: locate primary
    std::array<Locator_scanner::Locator_scanner_result, MAX_LOCATOR_BUFFER> buffer{};

    //limited to center 2/4*2/4 area
    int l=reader_->get_width()/4;
    int r=reader_->get_width()-l;
    int t=reader_->get_height()/4;
    int b=reader_->get_height()-t;

    auto len=locate_multiple_primary(buffer.data(),(int)(buffer.size()),l,t,r,b);

    //second: number <2, (3, 4, more)
    Mat<Option<Locator_scanner::Locator_scanner_result>> locator_map(hcnt+1,vcnt+1,[]{return None<Locator_scanner::Locator_scanner_result>();});
    auto none=None<Mat<Option<Locator_scanner_result>>>();

    if(len<=2)return none;
    //sort by likelihood
    std::sort(buffer.begin(),buffer.begin()+len, [](const Locator_scanner::Locator_scanner_result& a, const Locator_scanner::Locator_scanner_result& b){
        return a.likelihood>b.likelihood;
    });
    //drop the least ones
    LOCALCONST(LOCATOR_LIKELIHOOD_RANGE, double);
    std::queue<Point> queue_;

    auto end=buffer.end()-1;
    auto min_ll=buffer.begin()->likelihood*LOCATOR_LIKELIHOOD_RANGE;

    while(end->likelihood<min_ll)end--;
    len=(int)(end-buffer.begin()+1);

    if(len<=2||len>4)return none;
    if(len==3){
        //lack one, guess which it is
        //sort by x, then sort by y
        std::sort(buffer.begin(),buffer.begin()+3,[](auto x, auto y){
            return x.center_x<y.center_x;
        });

        bool is_right=buffer[1].center_x-buffer[0].center_x<buffer[2].center_x-buffer[1].center_x;
        auto& standalone_x=buffer[2];

        std::sort(buffer.begin(),buffer.begin()+3,[](auto x, auto y){
            return x.center_y<y.center_y;
        });

        bool is_bottom=buffer[1].center_y-buffer[0].center_y<buffer[2].center_y-buffer[1].center_y;
        auto& standalone_y=buffer[2];

        if(standalone_x==standalone_y)return none;

        Point p1(is_right?(hcnt/2+1):(hcnt/2),is_bottom?(vcnt/2):(vcnt/2+1));
        Point p2(is_right?(hcnt/2):(hcnt/2+1),is_bottom?(vcnt/2+1):(vcnt/2));
        locator_map[p1.x][p1.y]=Some(standalone_x);
        locator_map[p2.x][p2.y]=Some(standalone_y);
        queue_.push(p1);
        queue_.push(p2);

        auto& lastone=buffer[1]==standalone_x?buffer[0]:buffer[1];
        Point p3(is_right?(hcnt/2):(hcnt/2+1),is_bottom?(vcnt/2):(vcnt/2+1));
        locator_map[p3.x][p3.y]=Some(lastone);
        queue_.push(p3);
    }
    else if(len==4){
        //normal case, sort by x and y
        std::sort(buffer.begin(),buffer.begin()+4,[](auto x, auto y){
            return x.center_x<y.center_x;
        });
        if(buffer[0].center_y<buffer[1].center_y)swap(buffer[0],buffer[1]);
        if(buffer[2].center_y<buffer[3].center_y)swap(buffer[1],buffer[2]);
        for(int i=0;i<4;i++) {
            Point p((hcnt / 2 + i / 2), vcnt / 2 + i % 2);
            locator_map[p.x][p.y] = Some(buffer[i]);
            queue_.push(p);
        }
    }
    //four: iterating, get all possible
    while(!queue_.empty()){
        auto p=queue_.front();
        queue_.pop();
        _find_adj(locator_map,p.x,p.y,queue_);
    }
    //five: return
    return Some(locator_map);
}

Option<Mat<Option<Locator_scanner::Locator_scanner_result>>> Locator_scanner::update_locator_net(Mat<Option<Locator_scanner_result>> prev){
    LOCALCONST(LOCATOR_LIKELIHOOD_RANGE,double);
    //first: track everything
    Locator_scanner_result wont_use;
    Point wont_use_p;
    std::queue<Point> queue_;
    auto res=prev.transform([&prev](const Option<Locator_scanner_result> & oloc){
            return oloc.transform([](const Locator_scanner_result& loc){
                Locator_scanner_result res=loc;
                if (res.is_primary) {
                    if (track_single_primary(res)) {
                        auto rat = res.likelihood / loc.likelihood;
                        if (rat > 1)rat = 1 / rat;
                        if (rat > LOCATOR_LIKELIHOOD_RANGE)
                            return Some(res);
                        else
                            return None<Locator_scanner_result>();
                    }
                    else
                        return None<Locator_scanner_result>();
                }
                else {
                    if (track_single_secondary(res)) {
                        auto rat = res.likelihood / loc.likelihood;
                        if (rat > 1)rat = 1 / rat;
                        if (rat > LOCATOR_LIKELIHOOD_RANGE)
                            return Some(res);
                        else
                            return None<Locator_scanner_result>();
                    }
                    else
                        return None<Locator_scanner_result>();
                }
            });
        });
    //second: for those which cannot be tracked or originally not recognized, re-recognize them
    for(int i=0;i<res.get_width();i++)
        for(int j=0;j<res.get_height();j++)
            if(!res[i][j].empty())
                queue_.push(Point(i,j));
    while(!queue_.empty()) {
        auto t=queue_.front();
        queue_.pop();
        _find_adj(prev,t.x,t.y,queue_);
    }
    if(queue_.size()<4)return None<Mat<Option<Locator_scanner_result>>>();
    //finally, return
    else return Some(res);
}

#if(0)
static inline bool locator_ptr_cmp_x(Locator_scanner::Locator_scanner_result& r1, Locator_scanner::Locator_scanner_result& r2){
    return r1.center_x>r2.center_x;
}
static inline bool locator_ptr_cmp_y(Locator_scanner::Locator_scanner_result& r1, Locator_scanner::Locator_scanner_result& r2){
    return r1.center_y>r2.center_y;
}


static bool _direction_judge(Locator_scanner::Locator_scanner_result* ori, Locator_scanner::Locator_scanner_result* to_judge, int block_size, bool is_x){
    //first: length shouldn't be too small
    double len=sqrt((ori->center_x-to_judge->center_x)*(ori->center_x-to_judge->center_x)+(ori->center_y-to_judge->center_y)*(ori->center_y*to_judge->center_y));
    double len_adj_should_be=(is_x?(ori->center_x+to_judge->center_x):(ori->center_y+to_judge->center_y))/2*block_size;
    double len_diag_should_be=sqrt(pow((is_x?(ori->estimated_symbol_size_x+to_judge->estimated_symbol_size_x):(ori->estimated_symbol_size_y+to_judge->estimated_symbol_size_y))*block_size,2)+
                                   pow((is_x?(ori->estimated_symbol_size_y+to_judge->estimated_symbol_size_y):(ori->estimated_symbol_size_x+to_judge->estimated_symbol_size_x))/2*block_size,2));
    double ratio_yx=(ori->center_y-to_judge->center_y)/(ori->center_x-to_judge->center_x);
    double abs_ratio_yx=abs(ratio_yx);
    if((abs_ratio_yx>1&&is_x)||(abs_ratio_yx<1&&!is_x))return false;
    if(len<len_adj_should_be/2)return false;
    if(len>(len_adj_should_be+len_diag_should_be)/2)return false;
    return true;
}
#endif

