//
// Created by 王安然 on 15/8/7.
//

#include "modulator.h"
#include "utils/hamming128.h"

using namespace std;

void Modulator::_push_primary_with_escape(Escaper& escaper, Tx_block_helper& helper, uint8_t byte){
    uint8_t escaped[3];
    escaper.escape(byte, escaped);
    helper.push_primary(escaped[0]);
    helper.push_primary(escaped[1]);
    helper.push_primary(escaped[2]);
}
static uint8_t _transform_by_mask(uint8_t mask, uint8_t data) {
    switch (mask) {
        case 0:
            return 0;
        case 1:
            return data & 1;
        case 2:
            return (data & 1) << 1;
        case 3:
            return data & 3;
        case 4:
            return (data & 1) << 2;
        case 5:
            return ((data & 2) << 1) | (data & 1);
        case 6:
            return (data & 3) << 1;
        case 7:
            return data & 7;
    }
}

inline static uint8_t to_uint8(bool b){return b?1:0;}

static uint8_t _transform_by_mask(uint8_t mask, bool* data){
    switch (mask) {
        case 0:
            return 0;
        case 1:
            return to_uint8(data[0]);
        case 2:
            return (to_uint8(data[0])) << 1;
        case 3:
            return to_uint8(data[0])|(to_uint8(data[1])<<1);
        case 4:
            return (to_uint8(data[0])) << 2;
        case 5:
            return (to_uint8(data[1]) << 2) | (to_uint8(data[0]));
        case 6:
            return (to_uint8(data[1]) << 2) | (to_uint8(data[0]) << 1);
        case 7:
            return (to_uint8(data[2]) << 2) | (to_uint8(data[1]) << 1) | to_uint8(data[0]);
    }
}
static uint8_t _transform_back_by_mask(uint8_t mask, uint8_t data){
    switch (mask) {
        case 0:
            return 0;
        case 1:
            return data & 1;
        case 2:
            return (data & 2) >> 1;
        case 3:
            return data & 3;
        case 4:
            return (data & 4) >> 2;
        case 5:
            return ((data & 4) >> 1) | (data & 1);
        case 6:
            return (data & 6) >> 1;
        case 7:
            return data & 7;
    }
}

void Modulator::_fill_rest_block(Modulator::Tx_block_helper& helper){
    int rest=helper.get_total_symbol_count()%3;
    while(rest--)helper.push_primary(rest&7);
}

void Modulator::modulate_idle(Tx_block& dest){
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::IDLE, 0, parameters.parity);
    Tx_block_helper helper(dest);

    int count=helper.get_total_symbol_count();
    //it is not necessary to encode secondary channel for IDLE blocks
    //BUT escape still exists.
    for(int i=0;i<count-3;i+=3){
        _push_primary_with_escape(escaper_, helper, rand_256());
    }
    _fill_rest_block(helper);
};


//define error correction level
static int _redundancy(FEC_level level, int total_in_byte){
    //percentage
    static const int ratios[]={90,70,60};
    return ratios[(int)level]*total_in_byte/100;
}
static int _calc_msg_size(FEC_level level_pri, FEC_level level_sec, int total_symbol_count, int sec_mask,
                          int& out_redund_size){
    //primary
    int pri_size=total_symbol_count/3;
    int bit_per_symbol=(sec_mask&1)+((sec_mask>>1)&1)+((sec_mask>>2)&1);
    int bits=total_symbol_count*bit_per_symbol;
    //no inner FEC, or inner 12,8 code
    int sec_size=(level_sec==FEC_level::HIGH?(bits/8):(bits/12));

    out_redund_size=_redundancy(level_pri, pri_size+sec_size);
    return pri_size+sec_size;
}

//Data packet Link layer format:
//| 1 bit: last_is_data | 6 bits: FID |
//| 1 bit: start of packet | 1 bit: end of packet | 2 bits: packet type (data/ack/retransmission) |
//| 5 bits: reserved |
int Modulator::modulate_data(const uint8_t *source_ptr, Tx_block &dest,
                             bool last_block_is_data, int FID, bool is_start_of_packet, bool is_end_of_packet, Packet_type packet_type,
                             int max_size) {
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::DATA, 0, parameters.parity);
    Tx_block_helper helper(dest);

    if(max_size==-1)max_size=MAX_INT;

    //max block size in both channels = symbol number/3 + symbol number*3/8
    //max symbol number = 24*24 = 576
    //message limit: 256
    static int MAX_BLOCK_SIZE=256;

    uint8_t buffer[MAX_BLOCK_SIZE];


    //first: encode header
    buffer[0]=(uint8_t)((last_block_is_data?128:0) | ((FID&63)<<1) | (is_start_of_packet?1:0));
    buffer[1]=(uint8_t)((is_end_of_packet?128:0) | (((int)packet_type)<<5));

    //second: calculate
    int k;
    int n=_calc_msg_size(parameters.FEC_strength_primary, parameters.FEC_strength_secondary, helper.get_total_symbol_count(),parameters.color_sec_mask, k);
    auto& coder=coder_buffered_.get_coder(n,k);

    //third: encode
    unsigned int length=(unsigned)min(max_size, n-k-2);
    memcpy(buffer+2, source_ptr, length);
    coder->encode(buffer);

    //fourth: modulate
    auto& hamming=Hamming128::get_shared();
    Escaper escaper;
    uint8_t escaped[3];
    for(int i=0;i<helper.get_total_symbol_count()/3;i++){
        _push_primary_with_escape(escaper,helper, buffer[i]);
    }

    if(parameters.color_sec_mask!=0) {
        bool sec_buffer[12];
        uint8_t sec_mask = parameters.color_sec_mask;
        int bit_per_symbol = (sec_mask & 1) + ((sec_mask >> 1) & 1) + ((sec_mask >> 2) & 1);
        if (parameters.FEC_strength_secondary == FEC_level::HIGH) { // no redundancy
            uint8_t last = 0;
            int last_cnt = 0;
            for (int i = helper.get_total_symbol_count() / 3; i < n; i++) {
                int data=(buffer[i]|(last<<8));
                for (int j = 0; j < (8+last_cnt) / bit_per_symbol; j++) {
                    uint8_t s_data = (data >> (8+last_cnt-bit_per_symbol-j * bit_per_symbol)) & ((1 << bit_per_symbol) - 1);
                    helper.push_secondary(sec_mask, _transform_by_mask(sec_mask, s_data));
                }
                last_cnt=(8+last_cnt)%bit_per_symbol;
                last=data&((1<<last_cnt)-1);
            }
            if (last_cnt != 0)
                helper.push_secondary(sec_mask, _transform_by_mask(sec_mask, last));
        }
        else { //use 12,8 hamming
            for (int i = helper.get_total_symbol_count() / 3; i < n; i++) {
                hamming.encode(buffer[i], sec_buffer);
                //assume 12 can divide bit_per_symbol
                for (int j = 0; j < 12 / bit_per_symbol; j++) {
                    helper.push_secondary(sec_mask, _transform_by_mask(sec_mask, sec_buffer+j*bit_per_symbol));
                }
            }
        }
    }
    return length;
}

void Modulator::modulate_probe(const Tx_PHY_probe &probe, Tx_block &dest) {
    auto& parameters=Tx_adaptive_parameters::current();

    dest.init(parameters.block_sidelength, Block_type::PROBE, 0, parameters.parity);
    Tx_block_helper helper(dest);
    //first is palette
    for(int i=0;i<8;i++)
        for(int j=0;j<8;j++) {
            helper.push_primary(i);
            helper.push_secondary(7, j);
        }

    //then patterns to detect error rate and sharpness
    for(int i=0;i<helper.get_total_symbol_count()-64;i++){
        int line=helper.get_actual_pos(helper.seek_primary())/dest.sidelength;
        if(line%2)
            helper.push_primary(i%8);
        else helper.push_primary(7^(i%8));

        helper.push_secondary(7,i%2==0?0:7);
    }
}

//Action packet format:
// | 8 bit: next_sidelength | 1 bit: next_expected_parity | 3 bit: color_sec_mask |
// | 2 bit: FEC_level_pri | 2 bit: FEC_level_sec | 8 bit: self_FPS | 5 bytes: reserved
// a low rate FEC by default (16,8)
void Modulator::modulate_action(const Tx_PHY_action &action, Tx_block &dest) {
    static Reed_solomon_code low_rate_rs_code(16,8);

    auto& parameters=Tx_adaptive_parameters::current();
    dest.init(parameters.block_sidelength, Block_type::PROBE, 0, parameters.parity);

    uint8_t buffer[16];
    buffer[0]=action.next_sidelength;
    buffer[1]=(action.next_expected_parity?128:0) | (action.color_sec_mask << 4) | (((uint8_t)(action.FEC_level_pri))<<2) | ((uint8_t)(action.FEC_level_sec));
    buffer[2]=action.self_FPS;

    low_rate_rs_code.encode(buffer);

    Tx_block_helper helper(dest);
    Escaper escaper;
    for(int i=0;i<16;i++){
        _push_primary_with_escape(escaper, helper, buffer[i]);
    }
}


Demodulator::Block_content Demodulator::_get_block_content(Pixel_reader* reader_, int sidelength, Symbol_scanner::Block_anchor& anchor, bool parity) {
    auto left_top = anchor.left_top;
    auto right_top = anchor.right_top;
    auto left_bottom = anchor.left_bottom;
    auto right_bottom = anchor.right_bottom;
    auto fun_locate = [sidelength, left_top, right_top, right_bottom, left_bottom](int x, int y) {
        int fmx = sidelength;
        int fmy = sidelength;
        int fzx1 = x;
        int fzy1 = y;
        int fzx = fmx - x;
        int fzy = fmy - y;

        auto px = (fzx * fzy * left_top->center_x +
                   fzx1 * fzy * right_top->center_x +
                   fzx * fzy1 * left_bottom->center_x +
                   fzx1 * fzy1 * right_bottom->center_x) / (fmx * fmy);
        auto py = (fzx * fzy * left_top->center_y +
                   fzx1 * fzy * right_top->center_y +
                   fzx * fzy1 * left_bottom->center_y +
                   fzx1 * fzy1 * right_bottom->center_y) / (fmx * fmy);

        return Point(px, py);
    };
    return Block_content(sidelength, [reader_, fun_locate](int x, int y) {
                             auto p = fun_locate(x, y);
                             return reader_->get_RGB(p.x, p.y);
                         },
                         [reader_, fun_locate](int x, int y) {
                             auto p = fun_locate(x, y);
                             return reader_->get_smoothed_RGB(p.x, p.y);
                         },parity);
}

//101 means even (false), 010 means odd (true)
Option<Block_type> Demodulator::get_block_type(Block_content &src) {
    auto c1=src.get_smoothed_color(4,0);
    auto c2=src.get_smoothed_color(src.sidelength-4, src.sidelength-1);
    bool newest;
    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(src.parameter_parity, newest);
    auto matcher=parameters.palette_matcher;
    auto c1m=matcher->match_primary(c1);
    auto c2m=matcher->match_primary(c2);

    auto c1c=CHECK_PAR2(c1m)?Option((Block_type)(c1m>>1)):None<Block_type>();
    auto c2c=CHECK_PAR2(c2m)?Option((Block_type)(c2m>.1)):None<Block_type>();
    if(c1c.empty() && c2c.empty())return None<Block_type>();
    else if(c1c.empty())return c2c;
    else if(c2c.empty())return c1c;
    else if(c1c.get_reference()==c2c.get_reference())return c1c;
    else return None<Block_type>();
}

bool Demodulator::demodulate_data(Block_content &src, uint8_t *data_dest, int &out_len,
                                  int &missed_len) {

}

bool Demodulator::demodulate_probe(Block_content &src, Rx_PHY_probe_result &probe_dest) {
    return false;
}

bool Demodulator::demodulate_action(Block_content &src, Rx_PHY_action_result &action_dest) {
    return false;
}

Option<Block_content> Demodulator::get_block_content(Symbol_scanner::Block_anchor &src, Pixel_reader* reader,
                                                             bool &out_parameter_parity) {

    Point c1p((src.left_top->center_x+src.right_top->center_x)/2, (src.left_top->center_y+src.right_top->center_y)/2);
    Point c2p((src.left_top->center_x+src.left_bottom->center_x)/2, (src.left_top->center_y+src.left_bottom->center_y)/2);

    auto c1=reader->get_smoothed_RGB(c1p.x,c1p.y);
    auto c2=reader->get_smoothed_RGB(c2p.x,c2p.y);

    bool newest_parity;
    auto& last_parameter=Rx_adaptive_parameters::get_newest(newest_parity);
    auto c1m=last_parameter.palette_matcher->match_primary(c1);
    auto c2m=last_parameter.palette_matcher->match_primary(c2);
    c1m=(c1m==5?0:(c1m==2?1:-1));
    c2m=(c2m==5?0:(c2m==2?1:-1));
    int parity;
    if(c1m==-1 && c2m==-1)return None<Block_content>();
    else if(c1m==-1)parity=c2m;
    else if(c2m==-1)parity=c1m;
    else if(c1m!=c2m)return None<Block_content>();
    else parity=c1m;

    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(parity, newest_parity);
    return Some(_get_block_content(reader, parameters.block_sidelength,src,parity));
}

Demodulator::Block_content_helper::Block_content_helper(Demodulator::Block_content &content):
        pos_(0),escape_pos_(0), content_(&content),escape_buffer({
        0,1,2,3,4, content_->sidelength/2, content_->sidelength-2,content_->sidelength-1, //8
        content_->sidelength+0,content_->sidelength+1,content_->sidelength+2,content_->sidelength*2-2,content_->sidelength*2-1, //5
        content_->sidelength*2+0,content_->sidelength*2+1,content_->sidelength*3-1, //3
        (content_->sidelength/2)*content_->sidelength, //1
        content_->sidelength*(content_->sidelength-2)+0,content_->sidelength*(content_->sidelength-2)+1,content_->sidelength*(content_->sidelength-1)-1, //3
        content_->sidelength*(content_->sidelength-1)+0,content_->sidelength*(content_->sidelength-1)+1,content_->sidelength*(content_->sidelength-1)+2, //3
        content_->sidelength*content_->sidelength-4,content_->sidelength*content_->sidelength-3,content_->sidelength*content_->sidelength-2,content_->sidelength*content_->sidelength-1}){} //2

//same as Tx_block_helper::get_total_symbol_count
int Demodulator::Block_content_helper::get_total_symbol_count() {
    return content_->sidelength*content_->sidelength
            -8-3-5-5-4-2;
}

bool Demodulator::Block_content_helper::pull_symbol(RGB& out_color) {
    while(pos_<ARRSIZE(escape_buffer) && pos_<content_->sidelength*content_->sidelength && escape_buffer[escape_pos_]==pos_) {
        pos_++;
        escape_pos_++;
    }
    if(pos_>=content_->sidelength*content_->sidelength)return false;
    out_color=content_->get_center_color(pos_%content_->sidelength,pos_/content_->sidelength);
    return true;
}
