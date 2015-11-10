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
    MID_HIGH,
    MID_LOW,
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

// Transmitter related structures

/*
 * PHY Block format:
 * | Anchor: 3 sym | Syn: 1 sym | Block_type: 1 sym | Data with anchor and flag | Block type: 1 sym | Syn: 1 sym | Anchor: 3 sym |
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
        set(sidelength-4,sidelength-1,RGB(syn));

        set(4,0,PAR2(((uint8_t)type)<<1));
        set(sidelength-5,sidelength-1,PAR2(((uint8_t)type)<<1));

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

    class Tx_block_helper{
    friend class Tx_block;
    private:
        Tx_block* block_;
        unsigned int pos_pri_;
        unsigned int pos_sec_;
        const int max_pos_;
        int escape_ptr_pri_;
        int escape_ptr_sec_;
        int escape_buffer[27];

        Tx_block_helper(Tx_block* block):block_(block), pos_pri_(0), pos_sec_(0), max_pos_(block->sidelength*block->sidelength), escape_ptr_pri_(0),escape_ptr_sec_(0), escape_buffer({
                0,1,2,3,4, block_->sidelength/2, block_->sidelength-2,block_->sidelength-1, //8
                block_->sidelength+0,block_->sidelength+1,block_->sidelength+2,block_->sidelength*2-2,block_->sidelength*2-1, //5
                block_->sidelength*2+0,block_->sidelength*2+1,block_->sidelength*3-1, //3
                (block_->sidelength/2)*block_->sidelength, //1
                block_->sidelength*(block_->sidelength-2)+0,block_->sidelength*(block_->sidelength-2)+1,block_->sidelength*(block_->sidelength-1)-1, //3
                block_->sidelength*(block_->sidelength-1)+0,block_->sidelength*(block_->sidelength-1)+1,block_->sidelength*(block_->sidelength-1)+2, //3
                block_->sidelength*block_->sidelength-4,block_->sidelength*block_->sidelength-3,block_->sidelength*block_->sidelength-2,block_->sidelength*block_->sidelength-1}){} //2

    public:
        unsigned int get_actual_pos(unsigned int pos){
            unsigned int actual_pos=pos+5;
            if(actual_pos>=block_->sidelength/2)actual_pos++;
            if(actual_pos>=block_->sidelength-2)actual_pos+=5;
            if(actual_pos>=block_->sidelength*2-2)actual_pos+=4;
            if(actual_pos>=block_->sidelength*3-1)actual_pos++;
            if(actual_pos>=block_->sidelength*block_->sidelength/2)actual_pos++;
            if(actual_pos>=block_->sidelength*(block_->sidelength-2))actual_pos+=2;
            if(actual_pos>=block_->sidelength*(block_->sidelength-1)-1)actual_pos+=4;
            return actual_pos;
        }
        int get_total_symbol_count(){
            return block_->sidelength*block_->sidelength
                   -8-3-5-5 //anchor
                    -4-2; //header & footer
        }
        bool push_primary(uint8_t data){
            while(escape_ptr_pri_< ARRSIZE(escape_buffer) && pos_pri_< max_pos_ && escape_buffer[escape_ptr_pri_]==pos_pri_){
                pos_pri_++;
                escape_ptr_pri_++;
            }
            if(pos_pri_>=max_pos_)return false;
            block_->set(pos_pri_,RGB(data));
            pos_pri_++;
            return true;
        }
        int seek_primary(int place=-1){
            if(place<0){
                int p=bsearch_less(escape_buffer,ARRSIZE(escape_buffer),pos_pri_);
                return pos_pri_-p-1;
            }
            if(place>=get_total_symbol_count())place=get_total_symbol_count();
            pos_pri_=get_actual_pos(place);

        }
        bool push_secondary(uint8_t mask, uint8_t data){
            while(escape_ptr_sec_< ARRSIZE(escape_buffer) && pos_sec_< max_pos_ && escape_buffer[escape_ptr_sec_]==pos_sec_){
                pos_sec_++;
                escape_ptr_sec_++;
            }
            if(pos_sec_>=max_pos_)return false;
            block_->get(pos_sec_).set_secondary(mask,data);
            pos_sec_++;
            return true;
        }

        bool skip_secondary(){
            while(escape_ptr_sec_< ARRSIZE(escape_buffer) && pos_sec_< max_pos_ && escape_buffer[escape_ptr_sec_]==pos_sec_){
                pos_sec_++;
                escape_ptr_sec_++;
            }
            if(pos_sec_>=max_pos_)return false;
            pos_sec_++;
            return true;
        }

        int seek_secondary(int place=-1){
            if(place<0){
                int p=bsearch_less(escape_buffer,ARRSIZE(escape_buffer),pos_sec_);
                return pos_sec_-p-1;
            }
            if(place>=get_total_symbol_count())place=get_total_symbol_count();
            pos_sec_=get_actual_pos(place);
        }
    };

    //return empty ptr when not initialized
    std::unique_ptr<Tx_block_helper> get_helper(){
        if(inited())return std::unique_ptr<Tx_block_helper>(this);
        else return std::unique_ptr<Tx_block_helper>(nullptr);
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
    bool fill_block(Tx_block& dest) const {
        if(!dest.inited())return false;
        auto helper=dest.get_helper();
        //first is palette
        for(int i=0;i<8;i++)
        for(int j=0;j<8;j++) {
            helper->push_primary(i);
            helper->push_secondary(7, j);
        }

        //then patterns to detect error rate and sharpness
        for(int i=0;i<helper->get_total_symbol_count()-64;i++){
            int line=helper->get_actual_pos(helper->seek_primary())/dest.sidelength;
            if(line%2)
                helper->push_primary(i%8);
            else helper->push_primary(7^(i%8));

            helper->push_secondary(7,i%2==0?0:7);
        }
    };
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
    std::vector<RGB> received_probe_colors[64];
    enum class Edge_sharpness{
        SHARP,
        MEDIUM,
        BLUR
    } sharpness;
    Error_estimate error_estimate_pri, error_estimate_sec;
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
