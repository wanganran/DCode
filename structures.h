//
// Created by 王安然 on 15/10/20.
//

#ifndef DCODE_STRUCTURES_H
#define DCODE_STRUCTURES_H

#include <vector>
#include "utils/utils.h"

enum class Block_type:uint8_t {
    IDLE=0,
    DATA=1,
    PROBE=2,
    ACTION=3
};

enum class Error_estimate{
    HIGH,
    MID,
    LOW
};

enum class FEC_level{
    //HIGH means less redundancy.
    HIGH=0,
    MID=1,
    LOW=2
};

enum class Packet_type{
    DATA,
    ACK,
    SINGLE_BLOCK_RETRANSMISSION,
    COMBINED_RETRANSMISSION
};

struct Block_meta{
    bool last_is_data;
    bool start_of_packet;
    bool end_of_packet;
    int FID;
    Packet_type type;
};
// Transmitter related structures

/*
 * PHY Block format:
 * | Anchor: 3 sym | Syn: 1 sym | Block_type: 1 sym | Data with anchor and flag | Block type: 1 sym | Syn: 1 sym | Anchor: 2 sym |
 */

struct Tx_block : public Noncopyable{
private:
    RGB* content_;
    bool inited_;
public:
    int sidelength;

    Tx_block():sidelength(-1),content_(nullptr), inited_(false){}
    void init(int sidelength, Block_type type, uint8_t syn, bool para_parity){
        if(this->sidelength!=sidelength) {
            this->sidelength = sidelength;
            if(content_)delete[] content_;
            content_ = new RGB[sidelength * sidelength];
        }
        static const RGB COLOR_BLACK=RGB(255,255,255);
        static const RGB COLOR_WHITE=RGB(0,0,0);

        set(0,0,COLOR_BLACK);
        set(1,0,COLOR_WHITE);
        set(2,0,COLOR_BLACK);
        set(0,1,COLOR_WHITE);
        set(0,2,COLOR_BLACK);
        set(1,1,COLOR_WHITE);
        set(1,2,COLOR_BLACK);
        set(2,1,COLOR_BLACK);

        set(sidelength-1,sidelength-1,COLOR_WHITE);
        set(sidelength-1,sidelength-2,COLOR_BLACK);
        set(sidelength-2,sidelength-1,COLOR_BLACK);

        set(0,sidelength-1,COLOR_WHITE);
        set(1,sidelength-1,COLOR_WHITE);
        set(0,sidelength-2,COLOR_BLACK);
        set(1,sidelength-2,COLOR_BLACK);
        set(2,sidelength-1,COLOR_BLACK);

        set(sidelength-1,0,COLOR_WHITE);
        set(sidelength-2,0,COLOR_BLACK);
        set(sidelength-1,1,COLOR_WHITE);
        set(sidelength-1,2,COLOR_BLACK);
        set(sidelength-2,1,COLOR_BLACK);

        set(3,0,RGB(syn));
        set(sidelength-3,sidelength-1,RGB(syn));

        set(4,0,PAR2(((uint8_t)type)<<1));
        set(sidelength-4,sidelength-1,PAR2(((uint8_t)type)<<1));

        int mid=sidelength/2;
        set(0,mid,RGB(para_parity?2:5));
        set(mid,0,RGB(para_parity?2:5));

        inited_=true;
    }
    void reset(){
        inited_=false;
    }
    bool inited(){
        return inited_;
    }
    //no check for efficiency
    void set(unsigned int x, unsigned int y, RGB color){
        content_[x+y*sidelength]=color;
    }
    void set(unsigned int pos, RGB color){
        set(pos%sidelength,pos/sidelength,color);
    }
    RGB& get(int x, int y){
        return content_[x+y*sidelength];
    }
    RGB& get(unsigned int pos){
        return get(pos%sidelength,pos/sidelength);
    }
};

struct Tx_frame : public Noncopyable{
private:
    Tx_block* content_;
    bool inited_;
public:
    const int vertical_count;
    const int horizontal_count;

    Tx_frame(int v_count,int h_count):vertical_count(v_count),horizontal_count(h_count), content_(nullptr){}
    void init(){
        inited_=true;
        if(!content_)content_=new Tx_block[vertical_count*horizontal_count];
        for(int i=0;i<vertical_count*horizontal_count;i++)
            content_[i].reset();
    }
    bool inited(){return inited_;}
    void reset(){
        inited_=false;
    }
    Tx_block& get_block_ref(int x, int y){
        return content_[x+y*horizontal_count];
    }
    int get_total_symbol_vertical_count_with_border(int sidelength){
        return vertical_count*sidelength+5;
    }
    int get_total_symbol_horizontal_count_with_border(int sidelength){
        return horizontal_count*sidelength+5;
    }
    //assume the corresponded block is inited
    const RGB& get_symbol_at_with_border(int x, int y, int sidelength){
        static const int BORDERS_LT[][3]={{0,1,0},{1,1,0},{0,0,0}};
        static const int BORDERS_RB[][2]={{0,0},{0,1}};
        static const int BORDERS_RT[][2]={{0,1},{0,1},{0,0}};
        static const int BORDERS_LB[][3]={{0,0,0},{1,1,0}};

        static const RGB COLOR_BLACK=RGB(255,255,255);
        static const RGB COLOR_WHITE=RGB(0,0,0);

        int rx=x-2;
        int ry=y-2;

        if(rx>=0 && ry>=0){
            int bx=rx/sidelength;
            int by=ry/sidelength;
            if(bx<horizontal_count && by<vertical_count)return get_block_ref(bx,by).get(rx%sidelength,ry%sidelength);
            else if(bx>=horizontal_count){
                int resid=ry%sidelength;
                if(resid<3)return BORDERS_LT[resid][rx%sidelength]==0?COLOR_BLACK:COLOR_WHITE;
                else if(resid<sidelength-2)return COLOR_WHITE;
                else return BORDERS_LB[resid+3-sidelength][rx%sidelength]==0?COLOR_BLACK:COLOR_WHITE;
            }
            else{
                int resid=rx%sidelength;
                if(resid<3)return BORDERS_LT[ry%sidelength][resid]==0?COLOR_BLACK:COLOR_WHITE;
                else if(resid<sidelength-2)return COLOR_WHITE;
                else return BORDERS_RT[ry%sidelength][resid+3-sidelength]==0?COLOR_BLACK:COLOR_WHITE;
            }
        }
        else if(rx<0) {
            int dx = rx + 2;
            int resid = (ry + sidelength) % sidelength;
            if (resid < 3)return BORDERS_RT[resid][dx] == 0 ? COLOR_BLACK : COLOR_WHITE;
            else if (resid < sidelength - 2)return COLOR_WHITE;
            else return BORDERS_RB[resid + 3 - sidelength][dx] == 0 ? COLOR_BLACK : COLOR_WHITE;
        }
        else {
            int dy = ry + 2;
            int resid = (rx + sidelength) % sidelength;
            if (resid < 3)return BORDERS_LB[dy][resid] == 0 ? COLOR_BLACK : COLOR_WHITE;
            else if (resid < sidelength - 2)return COLOR_WHITE;
            else return BORDERS_RB[dy][resid + 3 - sidelength] == 0 ? COLOR_BLACK : COLOR_WHITE;
        }
    }
};

struct Tx_PHY_probe{
//currently nothing here
};

struct Tx_PHY_action{
    bool next_expected_parity;
    int next_sidelength;
    uint8_t color_sec_mask;
    FEC_level FEC_level_pri, FEC_level_sec;
    int self_FPS;
};

//Receiver related structures

struct Rx_PHY_probe_result{
    RGB received_probe_colors[64];
    enum class Edge_sharpness{
        SHARP,
        MEDIUM,
        BLUR
    } sharpness;
    Error_estimate error_estimate_pri, error_estimate_sec[3];
};

struct Rx_PHY_action_result: public Tx_PHY_action{
};

struct Rx_segment : public Noncopyable{
private:

    bool inited_;
public:

    int frame_id;
    int block_id;
    bool is_start_of_packet;
    bool is_end_of_packet;
    uint8_t* data;
    int data_len;


    Rx_segment():inited_(false),data(nullptr),data_len(0){}

    void init(int len){
        if(data_len!=len){
            if(data)delete[] data;
            data=new uint8_t[len];
            data_len=len;
        }
        inited_=true;
    }
    void reset(){
        inited_=false;
    }
    bool inited(){return inited_;}
    ~Segment(){
        if(data)delete[] data;
    }
};

struct Rx_frame: public Noncopyable{

    std::vector<Rx_segment> segments;
    std::vector<Block_type> block_types;

    Rx_frame(int vcount, int hcount):segments((unsigned)(vcount*hcount)),block_types((unsigned)(vcount*hcount)){}
    void reset(){
        for(int i=0;i<segments.size();i++){
            segments[i].reset();
            block_types[i]=Block_type::IDLE;
        }
    }
};
struct Rx_window{
    const static int WINDOW_SIZE=64;
private:
    std::vector<Rx_frame> frames;
public:
    int top;
    int tail;

    Rx_frame& activate(int frameId, bool& replaced);

    Rx_frame& get_frame(int frameId);

    //frames which id<=till_frameId will be removed.
    bool truncate_frames(int till_frameId);
};

#endif //DCODE_STRUCTURES_H
