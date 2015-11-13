//
// Created by 王安然 on 15/8/7.
//

#ifndef DCODE_MODULATOR_H
#define DCODE_MODULATOR_H

#include <memory>
#include "palette.h"
#include "utils/utils.h"
#include "structures.h"
#include "symbol_scanner.h"
#include "structures.h"
#include "adaptive_parameters.h"
#include "utils/escaper.h"
#include "utils/reed_solomon_code.h"

class Modulator: public Noncopyable {
private:
    Escaper escaper_;
    Reed_solomon_code_buffered coder_buffered_;
    class Tx_block_helper {
        friend class Modulator;

    private:
        Tx_block *block_;
        unsigned int pos_pri_;
        unsigned int pos_sec_;
        const int max_pos_;
        int escape_ptr_pri_;
        int escape_ptr_sec_;
        int escape_buffer[27];

        Tx_block_helper(Tx_block& block) : block_(&block), pos_pri_(0), pos_sec_(0),
                                           max_pos_(block_->sidelength * block_->sidelength), escape_ptr_pri_(0),
                                           escape_ptr_sec_(0), escape_buffer({
                                         0, 1, 2, 3, 4,
                                         block_->sidelength / 2,
                                         block_->sidelength - 2,
                                         block_->sidelength - 1, //8
                                         block_->sidelength + 0,
                                         block_->sidelength + 1,
                                         block_->sidelength + 2,
                                         block_->sidelength * 2 - 2,
                                         block_->sidelength * 2 - 1, //5
                                         block_->sidelength * 2 + 0,
                                         block_->sidelength * 2 + 1,
                                         block_->sidelength * 3 - 1, //3
                                         (block_->sidelength / 2) *
                                         block_->sidelength, //1
                                         block_->sidelength *
                                         (block_->sidelength - 2) + 0,
                                         block_->sidelength *
                                         (block_->sidelength - 2) + 1,
                                         block_->sidelength *
                                         (block_->sidelength - 1) - 1, //3
                                         block_->sidelength *
                                         (block_->sidelength - 1) + 0,
                                         block_->sidelength *
                                         (block_->sidelength - 1) + 1,
                                         block_->sidelength *
                                         (block_->sidelength - 1) + 2, //3
                                         block_->sidelength *
                                         block_->sidelength - 4,
                                         block_->sidelength *
                                         block_->sidelength - 3,
                                         block_->sidelength *
                                         block_->sidelength - 2,
                                         block_->sidelength *
                                         block_->sidelength - 1}) { } //2

    public:
        unsigned int get_actual_pos(unsigned int pos) {
            unsigned int actual_pos = pos + 5;
            if (actual_pos >= block_->sidelength / 2)actual_pos++;
            if (actual_pos >= block_->sidelength - 2)actual_pos += 5;
            if (actual_pos >= block_->sidelength * 2 - 2)actual_pos += 4;
            if (actual_pos >= block_->sidelength * 3 - 1)actual_pos++;
            if (actual_pos >= block_->sidelength * block_->sidelength / 2)actual_pos++;
            if (actual_pos >= block_->sidelength * (block_->sidelength - 2))actual_pos += 2;
            if (actual_pos >= block_->sidelength * (block_->sidelength - 1) - 1)actual_pos += 4;
            return actual_pos;
        }

        int get_total_symbol_count() {
            return block_->sidelength * block_->sidelength
                   - 8 - 3 - 5 - 5 //anchor
                   - 4 - 2; //header & footer
        }

        bool push_primary(uint8_t data) {
            while (escape_ptr_pri_ < ARRSIZE(escape_buffer) && pos_pri_ < max_pos_ &&
                   escape_buffer[escape_ptr_pri_] == pos_pri_) {
                pos_pri_++;
                escape_ptr_pri_++;
            }
            if (pos_pri_ >= max_pos_)return false;
            block_->set(pos_pri_, RGB(data));
            pos_pri_++;
            return true;
        }

        int seek_primary(int place = -1) {
            if (place < 0) {
                int p = bsearch_less(escape_buffer, ARRSIZE(escape_buffer), pos_pri_);
                return pos_pri_ - p - 1;
            }
            if (place >= get_total_symbol_count())place = get_total_symbol_count();
            pos_pri_ = get_actual_pos(place);

        }

        bool push_secondary(uint8_t mask, uint8_t data) {
            while (escape_ptr_sec_ < ARRSIZE(escape_buffer) && pos_sec_ < max_pos_ &&
                   escape_buffer[escape_ptr_sec_] == pos_sec_) {
                pos_sec_++;
                escape_ptr_sec_++;
            }
            if (pos_sec_ >= max_pos_)return false;
            block_->get(pos_sec_).set_secondary(mask, data);
            pos_sec_++;
            return true;
        }

        bool skip_secondary() {
            while (escape_ptr_sec_ < ARRSIZE(escape_buffer) && pos_sec_ < max_pos_ &&
                   escape_buffer[escape_ptr_sec_] == pos_sec_) {
                pos_sec_++;
                escape_ptr_sec_++;
            }
            if (pos_sec_ >= max_pos_)return false;
            pos_sec_++;
            return true;
        }

        int seek_secondary(int place = -1) {
            if (place < 0) {
                int p = bsearch_less(escape_buffer, ARRSIZE(escape_buffer), pos_sec_);
                return pos_sec_ - p - 1;
            }
            if (place >= get_total_symbol_count())place = get_total_symbol_count();
            pos_sec_ = get_actual_pos(place);
        }
    };

    void _push_primary_with_escape(Escaper& escaper, Tx_block_helper& helper, uint8_t byte);
    void _fill_rest_block(Tx_block_helper& helper);
public:
    void modulate_idle(Tx_block& dest);

    //need some extra info: last block is data? frame ID? is start/end of packet? current packet type?
    int modulate_data(const uint8_t* source_ptr, Tx_block& dest,
                      const Block_meta& meta,
                      int max_size=-1);

    //Tx->Rx
    void modulate_probe(const Tx_PHY_probe& probe, Tx_block& dest);

    //Rx->Tx
    void modulate_action(const Tx_PHY_action& action, Tx_block& dest);
};

class Demodulator: public Noncopyable {
private:
    Block_content _get_block_content(Pixel_reader* reader, int sidelength, Symbol_scanner::Block_anchor& anchor, bool parity);
    Reed_solomon_code_buffered coder_buffered_;
    class Block_content_helper{
    private:
        int pos_;
        int escape_pos_;
        Block_content* content_;
        int escape_buffer[27];
    public:
        Block_content_helper(Block_content& content);
        int get_total_symbol_count();
        bool pull_symbol(RGB& out_color);
    };

public:

    class Block_content{
    public:
        bool parameter_parity;
        Lazy_mat<RGB> centered_;
        Lazy_mat<RGB> smoothed_;
        int sidelength;
        RGB get_center_color(int x_id, int y_id){return centered_[x_id][y_id];}
        RGB get_smoothed_color(int x_id, int y_id){return smoothed_[x_id][y_id];}

        Block_content(int sidelen, std::function<RGB(int,int)> func_center, std::function<RGB(int,int)> func_smoothed, bool parity):
                sidelength(sidelen),centered_(Lazy_mat<RGB>(sidelen,sidelen,func_center)),smoothed_(Lazy_mat<RGB>(sidelen,sidelen,func_smoothed)), parameter_parity(parity){}
    };

    Option<Block_content> get_block_content(Symbol_scanner::Block_anchor& src,Pixel_reader* reader, bool& out_parameter_parity);
    Option<Block_type> get_block_type(Block_content& src);
    bool demodulate_data(Block_content& src,
                         uint8_t* data_dest, int& out_len, Block_meta& out_meta);
    bool demodulate_probe(Block_content& src,
                          Rx_PHY_probe_result& probe_dest);
    bool demodulate_action(Block_content& src,
                           Rx_PHY_action_result& action_dest);
};

class Modulator_factory{
public:
    static Modulator& get_modulator(){
        static Modulator singleton;
        return singleton;
    }
    static Demodulator& get_demodulator(){
        static Demodulator singleton;
        return singleton;
    }

};
#endif //DCODE_DEMODULATOR_H
