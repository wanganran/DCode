//
// Created by 王安然 on 15/10/20.
//

#ifndef DCODE_STRUCTURES_H
#define DCODE_STRUCTURES_H

#include <vector>
#include "utils/utils.h"
#include "utils/RGB.h"
#include "config.h"

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

//this is inside a packet, which indicates it consists of data block
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


class Packet{
public:
    Packet_type type;
    uint8_t* data;
    int length;
    Packet():length(-1),data(nullptr),type(Packet_type::DATA){}
    bool inited(){return length>=0;};
    void init(Packet_type pkttype, int len){
        type=pkttype;
        if(data)delete[] data;
        data=new uint8_t[len];
        length=len;
    }

    ~Packet() {
        if(data)
            delete[] data;
    }
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

        set((unsigned int)(sidelength-1),(unsigned int)(sidelength-1),COLOR_WHITE);
        set((unsigned int)(sidelength-1),(unsigned int)(sidelength-2),COLOR_BLACK);
        set((unsigned int)(sidelength-2),(unsigned int)(sidelength-1),COLOR_BLACK);

        set(0,(unsigned int)(sidelength-1),COLOR_WHITE);
        set(1,(unsigned int)(sidelength-1),COLOR_WHITE);
        set(0,(unsigned int)(sidelength-2),COLOR_BLACK);
        set(1,(unsigned int)(sidelength-2),COLOR_BLACK);
        set(2,(unsigned int)(sidelength-1),COLOR_BLACK);

        set((unsigned int)(sidelength-1),0,COLOR_WHITE);
        set((unsigned int)(sidelength-2),0,COLOR_BLACK);
        set((unsigned int)(sidelength-1),1,COLOR_WHITE);
        set((unsigned int)(sidelength-1),2,COLOR_BLACK);
        set((unsigned int)(sidelength-2),1,COLOR_BLACK);

        set(3,0,RGB(syn));
        set((unsigned int)(sidelength-3),(unsigned int)(sidelength-1),RGB(syn));

        set(4,0,PAR2(((uint8_t)type)<<1));
        set((unsigned int)(sidelength-4),(unsigned int)(sidelength-1),PAR2(((uint8_t)type)<<1));

        unsigned int mid=(unsigned int)(sidelength/2);
        set(0,mid,RGB((uint8_t)(para_parity?2:5)));
        set(mid,0,RGB((uint8_t)(para_parity?2:5)));

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
    Tx_block& get_block_ref(int x, int y) const{
        return content_[x+y*horizontal_count];
    }
    int get_total_symbol_vertical_count_with_border(int sidelength) const{
        return vertical_count*sidelength+5;
    }
    int get_total_symbol_horizontal_count_with_border(int sidelength) const{
        return horizontal_count*sidelength+5;
    }
    //assume the corresponded block is inited
    const RGB& get_symbol_at_with_border(int x, int y, int sidelength) const{
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

struct Ack {
    static const int MAX_ACK_COUNT=8;
    //(FID,BID,is_fully_damaged) pairs
    Tp<int, int, bool> blocks_to_acked[MAX_ACK_COUNT];
    int count;

    Ack():count(0){}

    bool operator ==(const Ack& rhs){return this->count==0 && rhs.count==0;}
    bool operator !=(const Ack& rhs){return !(*this==rhs);}

    static Ack& empty() {
        static Ack singleton;
        return singleton;
    }
};

//Receiver related structures

class Rx_block {
public:
    bool parameter_parity;
    Lazy_mat<RGB> centered_;
    Lazy_mat<RGB> smoothed_;
    std::function<Point(int,int)> locator_;
    int sidelength;
    RGB get_center_color(int x_id, int y_id){return centered_[x_id][y_id];}
    RGB get_smoothed_color(int x_id, int y_id){return smoothed_[x_id][y_id];}
    Point get_center_point(int x_id, int y_id){return locator_(x_id,y_id);}

    Rx_block(int sidelen, std::function<Point(int,int)> locator, std::function<RGB(int,int)> func_center, std::function<RGB(int,int)> func_smoothed, bool parity):
            sidelength(sidelen), locator_(locator), centered_(Lazy_mat<RGB>(sidelen,sidelen,func_center)),smoothed_(Lazy_mat<RGB>(sidelen,sidelen,func_smoothed)), parameter_parity(parity){}
};

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

/*
struct Rx_retransmission : public Noncopyable {
private:
    int id;

}
*/

struct Rx_segment : public Noncopyable{
private:

    bool inited_;
public:
    Block_meta metadata;
    int block_id;
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

    int get_full_id(int block_per_frame) const {
        return metadata.FID * block_per_frame + block_id;
    }
};
/*
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
*/
struct Tx_window: public Noncopyable{
    const static int WINDOW_SIZE_HIGH=10;
    const static int WINDOW_SIZE_LOW=10;
private:
    Packet* frames_high_[WINDOW_SIZE_HIGH];
    Packet* frames_low_[WINDOW_SIZE_LOW];

    int top_;
    int tail_;
public:
    Tx_window():top_(0),tail_(0){

    }
    //won't block
    bool push_packet(Packet* pkt, bool high_priority);

    Packet* pull_packet();
};
/*
struct Rx_window: public Noncopyable{
    const static int WINDOW_SIZE=64;
private:
    Rx_segment* segments_;
    int segments_count_;
    int segments_per_frame_;
    int top_; //is always 1 segment beyond the most recent delivery
    int tail_; //is always the least recent delivery/not deliveried.
public:

    Rx_window():top_(0),tail_(0){
        auto& config=Config::current();
        segments_per_frame_=config->barcode_config.vertical_block_count*config->barcode_config.horizontal_block_count;
        segments_count_=segments_per_frame_*WINDOW_SIZE;
        segments_=new Rx_segment[segments_count_];
    }
    ~Rx_window(){
        delete[] segments_;
    }
    Rx_segment* tail(int& out_skip) {
        if (top_ == tail_)return nullptr;
        out_skip = 0;
        int tail = tail_, top = top_;
        if (tail > top)
            top += segments_count_;

        //one frame buffer
        int till_idx=segments_per_frame_*(top/segments_per_frame_-1);

        while (tail < till_idx) {
            if (!segments_[tail%segments_count_].inited()) {
                out_skip++;
                tail++;
            }
            else if (!segments_[tail%segments_count_].is_non_segment())
                tail++;
            else {
                tail_=tail%segments_count_;
                return &(segments_[tail%segments_count_]);
            }
        }
        return nullptr;
    }
    bool pop(Rx_segment* seg){
        int idx=(int)(seg-segments_);
        seg->reset();

        tail_=(idx+1)%segments_count_;

        return true;
    }

    //for write
    //if conflicted, return false
    bool as_non_segment(int FID, int block_id, bool& overlapped){
        int idx=FID*segments_per_frame_+block_id;

        int tail=tail_,top=top_;


        if(tail==top){
            //currently empty
            segments_[idx].init_as_non_segment();
            top_=idx+1;
            return true;
        }
        else {
            if (tail > top)top += segments_count_;

            int till_idx = segments_per_frame_ * (top / segments_per_frame_ - 1);

            //determine new idx
            int abs_idx;
            if(tail_>top_) {
                if (idx <= top_)abs_idx = idx + segments_count_;
                else abs_idx = idx;
            }
            else abs_idx=idx;

            if(till_idx<tail)till_idx=tail;

            if(abs_idx<till_idx && abs_idx>=tail) {
                overlapped = true;
                segments_[idx].init_as_non_segment();
                top_=(idx+1)%segments_count_;
                return true;
            }
            else if(abs_idx>=till_idx && abs_idx<=top)

            if (till_idx <= tail) {
                //not possible to overlap
                overlapped = false;
            }
            else {
                int till_idx_mod=till_idx%segments_per_frame_;
            }
                if(tail_<top_) {
                    if (idx <= top_ && idx >= tail_) {
                        if (segments_[idx].is_non_segment())return true;
                        else {
                            segments_[idx].init_as_non_segment();
                            return false;
                        }
                    }
                    else{
                        segments_[idx].init_as_non_segment();
                        top_=(idx+1)%segments_count_;
                        return true;
                    }
                }
                else{
                    if(idx<=top || idx>=tail){
                        if(segments_[idx].is_non_segment())return true;
                        else {
                            segments_[idx].init_as_non_segment();
                            return false;
                        }
                    }
                    else{
                        segments_[idx].init_as_non_segment();
                        top_=(idx+1)%segments_count_;
                        return true;
                    }
                }
            }
        }
    }
    //Rx_segment* as_segment(Block_meta meta, int block_id, uint8_t* data, int len, bool& replaced);

    bool push();
};
*/
#endif //DCODE_STRUCTURES_H
