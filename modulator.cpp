//
// Created by 王安然 on 15/8/7.
//

#include "modulator.h"
#include "utils/hamming128.h"
#include "utils/utils.h"

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
//| 8 bits: FID |
//| 1 bit: start of packet | 1 bit: end of packet | 2 bits: packet type (data/ack/retransmission) | 1 bit: last is data
//| 4 bits: reserved |
int Modulator::modulate_data(const uint8_t *source_ptr, Tx_block &dest,
                             const Block_meta& meta, const Ack& ack,
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
    Packet_type type=(meta.type==Packet_type::DATA?(ack.count!=0?Packet_type::ACK:Packet_type::DATA):meta.type);

    buffer[0]=(uint8_t)(meta.FID);
    buffer[1]=(uint8_t)((meta.end_of_packet?128:0) | (((int)type)<<5) | (meta.last_is_data?16:0));

    //second: calculate
    int k;
    int n=_calc_msg_size(parameters.FEC_strength_primary, parameters.FEC_strength_secondary, helper.get_total_symbol_count(),parameters.color_sec_mask, k);
    auto& coder=coder_buffered_.get_coder(n,k);

    //third: encode
    unsigned int length=(unsigned)min(max_size, n-k-2);

    //check ack
    bool acked=ack.count==0;
    int offset=0;
    if(type==Packet_type::ACK){
        offset++;
        buffer[2]=(uint8_t)(ack.count);
        for(int i=0;i<ack.count;i++){
            match_Tp(ack.blocks_to_acked[i], [buffer,i, &offset](int fid, int bid, bool is_fully_damaged){
                buffer[3+i*2]=(uint8_t)(fid&255);
                buffer[4+i*2]=(uint8_t)bid;
                offset+=2;
            });
        }
    }
    memcpy(buffer+2+offset, source_ptr, length-offset);
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
                //first high, then low
                for (int j = 12/bit_per_symbol-1; j >= 0; j--) {
                    helper.push_secondary(sec_mask, _transform_by_mask(sec_mask, sec_buffer+j*bit_per_symbol));
                }
            }
        }
    }
    return length-offset;
}

static const uint8_t GRAY_CODE[8]={0,1,3,2,6,7,5,4};
static const uint8_t GRAY_POS[8]={1,2,1,4,1,2,1,4};

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
        auto pos=helper.get_actual_pos(helper.seek_primary());
        int line=pos/dest.sidelength;
        int col=pos%dest.sidelength;
        if(line%2)
            helper.push_primary(GRAY_CODE[col%8]);
        else helper.push_primary(GRAY_CODE[7^(col%8)]);

        helper.push_secondary(7,col%2==0?0:7);
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
    buffer[0]=(uint8_t)action.next_sidelength;
    buffer[1]=(uint8_t)((action.next_expected_parity?128:0) | (action.color_sec_mask << 4) | (((uint8_t)(action.FEC_level_pri))<<2) | ((uint8_t)(action.FEC_level_sec)));
    buffer[2]=(uint8_t)action.self_FPS;

    low_rate_rs_code.encode(buffer);

    Tx_block_helper helper(dest);
    Escaper escaper;
    for(int i=0;i<16;i++){
        _push_primary_with_escape(escaper, helper, buffer[i]);
    }
}


Rx_block Demodulator::_get_block_content(Pixel_reader* reader_, int sidelength, Symbol_scanner::Block_anchor& anchor, bool parity) {
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
    return Rx_block(sidelength, fun_locate, [reader_, fun_locate](int x, int y) {
                             auto p = fun_locate(x, y);
                             return reader_->get_RGB(p.x, p.y);
                         },
                         [reader_, fun_locate](int x, int y) {
                             auto p = fun_locate(x, y);
                             return reader_->get_smoothed_RGB(p.x, p.y);
                         },parity);
}

//101 means even (false), 010 means odd (true)
Option<Block_type> Demodulator::get_block_type(Rx_block &src) {
    auto c1=src.get_smoothed_color(4,0);
    auto c2=src.get_smoothed_color(src.sidelength-4, src.sidelength-1);
    bool newest;
    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(src.parameter_parity, newest);
    auto matcher=parameters.palette_matcher;
    auto c1m=matcher->match_primary(c1);
    auto c2m=matcher->match_primary(c2);

    auto c1c=CHECK_PAR2(c1m)?Option((Block_type)(c1m>>1)):None<Block_type>();
    auto c2c=CHECK_PAR2(c2m)?Option((Block_type)(c2m>>1)):None<Block_type>();
    if(c1c.empty() && c2c.empty())return None<Block_type>();
    else if(c1c.empty())return c2c;
    else if(c2c.empty())return c1c;
    else if(c1c.get_reference()==c2c.get_reference())return c1c;
    else return None<Block_type>();
}

//Data packet Link layer format:
//| 8 bits: FID |
//| 1 bit: start of packet | 1 bit: end of packet | 2 bits: packet type (data/ack/retransmission) | 1 bit: last is data
//| 5 bits: reserved |
bool Demodulator::demodulate_data(Rx_block &src, uint8_t *data_dest, int &out_len,
                                  Block_meta& out_meta) {
    uint8_t buffer_sec[576];
    uint8_t buffer_data[256];

    bool newest;
    auto &parameters = Rx_adaptive_parameters::get_global_by_parity(src.parameter_parity, newest);

    Block_content_helper helper(src);
    int len = helper.get_total_symbol_count();
    Escaper escaper;
    uint8_t *buffer_ptr = buffer_data;
    for (int i = 0; i < len; i++) {
        RGB color;
        helper.pull_symbol(color);
        auto sym = parameters.palette_matcher->match(color);
        if (parameters.color_sec_mask == 0)
            buffer_ptr = escaper.input(sym & 7, buffer_ptr);
        else {
            buffer_ptr = escaper.input(sym & 7, buffer_ptr);
            buffer_sec[i] = _transform_back_by_mask(parameters.color_sec_mask, sym >> 3);
        }
    }
    int bit_per_symbol = (parameters.color_sec_mask & 1) + ((parameters.color_sec_mask >> 1) & 1) +
                         ((parameters.color_sec_mask >> 2) & 1);

    if (parameters.FEC_strength_secondary == FEC_level::HIGH) {
        int last = 0;
        int last_cnt = 0;
        for (int i = 0; i < len; i++) {
            last = ((last << bit_per_symbol) | buffer_sec[i]);
            last_cnt += bit_per_symbol;
            if (last_cnt >= 8) {
                *(buffer_ptr++) = (uint8_t) (last >> (last_cnt - 8));
                last_cnt -= 8;
                last ^= (last >> last_cnt << last_cnt);
            }
        }
    }
    else {
        auto &hamming = Hamming128::get_shared();
        int symbol_per_code = 12 / bit_per_symbol;
        for (int i = 0; i < len; i += symbol_per_code) {
            if (i + symbol_per_code > len)break;
            int code = 0;
            for (int j = 0; j < symbol_per_code; j++)
                code = ((code << symbol_per_code) | buffer_sec[i + j]);
            *(buffer_ptr++) = hamming.decode(code);
        }
    }
    //decode
    int n = (int) (buffer_ptr - buffer_data);
    int k;
    int n_should = _calc_msg_size(parameters.FEC_strength_primary, parameters.FEC_strength_secondary,
                                  helper.get_total_symbol_count(), parameters.color_sec_mask, k);
    assert(n == n_should);
    auto &decoder = coder_buffered_.get_coder(n, k);
    if (decoder->decode(buffer_data)) {
        out_meta.FID = buffer_data[0];
        out_meta.start_of_packet = (buffer_data[0] & 1) == 1;
        out_meta.end_of_packet = (buffer_data[1] >> 7) == 1;
        out_meta.type = (Packet_type) ((buffer_data[1] >> 5) & 3);
        out_meta.last_is_data = ((buffer_data[1] >> 4) & 1) == 1;

        out_len = n - k - 2;


        memcpy(data_dest, buffer_data + 2, (size_t)(n - k - 2));
        return true;
    }
    else return false;
}

bool Demodulator::demodulate_probe(Rx_block &src, Pixel_reader* reader, Rx_PHY_probe_result &probe_dest) {
    Block_content_helper helper(src);
    //first palette
    for(int i=0;i<64;i++)
        helper.pull_symbol_smoothed(probe_dest.received_probe_colors[i]);
    //then error rates
    int len=helper.get_total_symbol_count();
    //store the correct and error symbol number of primary channel
    int correct_sym=0;
    int err_sym=0;
    //store the secondary gap of three channel
    //[i][j][k] means ith channel, jth primary data, kth secondary data. Result is R/G/B level
    int sec_stat[3][2][2];
    int sec_stat_sqr[3][2][2];
    int sec_stat_num[3][2][2];
    bool newest;
    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(src.parameter_parity, newest);
    PointF last_pt;
    auto last_col=-2;
    int gradient_fz=0;
    int gradient_fm=0;

    for(int i=0;i<len-64;i++){
        auto pos=helper.get_actual_pos();
        auto line=pos/src.sidelength;
        auto col=pos%src.sidelength;
        PointF curr_pt=src.get_center_point(col,line);
        RGB dest;
        helper.pull_symbol(dest);
        uint8_t probe=parameters.palette_matcher->match_primary(dest);
        if(line%2==0)probe^=7;

        if(probe==GRAY_CODE[col%8]) {
            correct_sym++;

            int code = GRAY_CODE[col % 8];
            sec_stat[2][code & 1][col % 2] += dest.B;
            sec_stat_sqr[2][code & 1][col % 2] += dest.B * dest.B;
            sec_stat_num[2][code & 1][col % 2]++;

            sec_stat[1][code & 2][col % 2] += dest.G;
            sec_stat_sqr[1][code & 2][col % 2] += dest.G * dest.G;
            sec_stat_num[1][code & 2][col % 2]++;

            sec_stat[0][code & 4][col % 2] += dest.R;
            sec_stat_sqr[0][code & 4][col % 2] += dest.R * dest.R;
            sec_stat_num[0][code & 4][col % 2]++;

            if(col-last_col==1){
                //gradient exists
                //sample 8 points
                RGB samples[8];

                for(int t=0;t<8;i++){
                    auto pt = last_pt*(t/8.0)+curr_pt*(1-t/8.0);
                    samples[t]=reader->get_RGB(pt.x,pt.y);
                }
                auto last_code=GRAY_CODE[last_col%8];
                int smax=-1,smin=256;
                int scount=0;
                int squat1,squat3;
                switch(GRAY_POS[last_code]) {
                    case 4: //R
                        for (int t = 0; t < 8; t++) {
                            smax = max(smax, (int)(samples[t].R));
                            smin = min(smin, (int)(samples[t].R));
                        }
                        squat1 = (smin * 3 + smax) / 4;
                        squat3 = (smax * 3 + smin) / 4;
                        for (int t = 0; t < 8; t++)
                            if (samples[t].R < squat3 && samples[t].R >= squat1)scount++;
                        break;
                    case 2: //G
                        for (int t = 0; t < 8; t++) {
                            smax = max(smax, (int)(samples[t].G));
                            smin = min(smin, (int)(samples[t].G));
                        }
                        squat1 = (smin * 3 + smax) / 4;
                        squat3 = (smax * 3 + smin) / 4;
                        for (int t = 0; t < 8; t++)
                            if (samples[t].G < squat3 && samples[t].G >= squat1)scount++;
                        break;
                    case 1: //B
                        for (int t = 0; t < 8; t++) {
                            smax = max(smax, (int)(samples[t].B));
                            smin = min(smin, (int)(samples[t].B));
                        }
                        squat1 = (smin * 3 + smax) / 4;
                        squat3 = (smax * 3 + smin) / 4;
                        for (int t = 0; t < 8; t++)
                            if (samples[t].B < squat3 && samples[t].B >= squat1)scount++;

                }
                gradient_fm+=8;
                gradient_fz+=scount;
            }
        }
        else err_sym++;

    }

    if(gradient_fz*8<=gradient_fm)probe_dest.sharpness=Rx_PHY_probe_result::Edge_sharpness::SHARP;
    else if(gradient_fz*4<=gradient_fm)probe_dest.sharpness=Rx_PHY_probe_result::Edge_sharpness::MEDIUM;
    else probe_dest.sharpness=Rx_PHY_probe_result::Edge_sharpness::BLUR;

    if(err_sym*20<correct_sym) probe_dest.error_estimate_pri=Error_estimate::LOW;
    else if(err_sym*10<correct_sym) probe_dest.error_estimate_pri=Error_estimate::MID;
    else probe_dest.error_estimate_pri=Error_estimate::HIGH;

    for(int ch=0;ch<3;ch++){
        double err_prob[2];
        for(int pr=0;pr<2;pr++){
            int mean[2],var[2];
            for(int sr=0;sr<2;sr++){
                if(sec_stat_num==0){mean[sr]=0;var[sr]=100;}
                mean[sr]=sec_stat[ch][pr][sr]/sec_stat_num[ch][pr][sr];
                var[sr]=sec_stat_sqr[ch][pr][sr]-mean[sr]*mean[sr];
            }
            //assume gaussian distribution
            auto delta=mean[1]-mean[0];
            err_prob[pr]=(1-Normal_dist::N_cul(mean[0],sqrt(var[0]),delta))+Normal_dist::N_cul(mean[1],sqrt(var[1]),-delta);
        }
        auto prob=max(err_prob[0],err_prob[1]);
        probe_dest.error_estimate_sec[ch]=(prob<0.05?Error_estimate::LOW:(prob<0.15?Error_estimate::MID:Error_estimate::HIGH));
    }

    return true;
}

bool Demodulator::demodulate_action(Rx_block &src, Rx_PHY_action_result &action_dest) {
    static Reed_solomon_code low_rate_rs_code(16,8);
    Block_content_helper helper(src);
    bool newest;
    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(src.parameter_parity,newest);
    RGB dest;
    helper.pull_symbol(dest);
    Escaper escaper;

    uint8_t buffer[16];
    uint8_t* ptr=buffer;
    while(ptr!=buffer+16)
        ptr=escaper.input(parameters.palette_matcher->match_primary(dest),ptr);

    if(low_rate_rs_code.decode(buffer)){
        action_dest.next_sidelength=buffer[0];
        action_dest.next_expected_parity=(buffer[1]&128)>0?true:false;
        action_dest.color_sec_mask=(uint8_t)((buffer[1]>>4)&7);
        action_dest.FEC_level_pri=(FEC_level)((buffer[1]>>2)&3);
        action_dest.FEC_level_sec=(FEC_level)((buffer[1])&3);
        action_dest.self_FPS=buffer[2];
        return true;
    }
    else
        return false;
}

Option<Rx_block> Demodulator::get_block_content(Symbol_scanner::Block_anchor &src, Pixel_reader* reader,
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
    if(c1m==-1 && c2m==-1)return None<Rx_block>();
    else if(c1m==-1)parity=c2m;
    else if(c2m==-1)parity=c1m;
    else if(c1m!=c2m)return None<Rx_block>();
    else parity=c1m;

    auto& parameters=Rx_adaptive_parameters::get_global_by_parity(parity, newest_parity);
    return Some(_get_block_content(reader, parameters.block_sidelength,src,parity));
}

Demodulator::Block_content_helper::Block_content_helper(Rx_block &content):
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
bool Demodulator::Block_content_helper::pull_symbol_smoothed(RGB &out_color) {
    while(pos_<ARRSIZE(escape_buffer) && pos_<content_->sidelength*content_->sidelength && escape_buffer[escape_pos_]==pos_) {
        pos_++;
        escape_pos_++;
    }
    if(pos_>=content_->sidelength*content_->sidelength)return false;
    out_color=content_->get_smoothed_color(pos_%content_->sidelength,pos_/content_->sidelength);
    return true;
}