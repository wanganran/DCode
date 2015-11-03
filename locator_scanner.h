//
// Created by Anran on 15/5/12.
//

#ifndef DCODE_LOCATOR_SCANNER_H
#define DCODE_LOCATOR_SCANNER_H

#include <stddef.h>
#include <vector>
#include <math.h>
#include <array>
#include "config.h"
#ifdef ANDROID_PLATFORM
#include "physical.h"
#include "palette.h"
#include "utils/utils.h"

#endif


//this class aim to locate locators when nothing about locators is known.
class Locator_scanner{
    class Locator_scanner_result;
private:
	const Pixel_reader* reader_;
	const Palette* shared_palette_;

    typedef std::array<int,4> Scanned_boundaries_primary_;
    typedef std::array<int,2> Scanned_boundaries_secondary_;

    static inline double _symbol_size_from_boundary(Scanned_boundaries_primary_ boundaries){
        return (boundaries[3]-boundaries[0]+boundaries[2]-boundaries[1])/4.0;
    }

    bool _check_corner_scan_primary(int k,
                                    const Scanned_boundaries_primary_& boundaries,
                                    bool k_is_x,
                                    float& likelihood,
                                    const Locator_scanner_result* reference,
                                    RGB& out_black,
                                    RGB& out_white);

    bool _check_corner_scan_secondary(int k,
                                      const Scanned_boundaries_secondary_& boundaries,
                                      bool k_is_x,
                                      float& likelihood,
                                      const Locator_scanner_result& reference,
                                      RGB& out_black,
                                      RGB& out_white);

    bool _find_other_primary(int k, int y, bool k_is_x, int estimated_symbol_size_min, int estimated_symbol_size_max,
                     int& out_center, double& out_symbol_size, double& out_black_symbol_size, float& likelihood);

    bool _find_other_secondary(int k, int y, bool k_is_x,
                             int& out_center, double& out_black_symbol_size, float& likelihood, const Locator_scanner_result& reference);

    int _do_one_direction_primary(int k, int f, int e, bool k_is_x, Locator_scanner_result& result, const Locator_scanner_result* reference);

    int _do_one_direction_secondary(int k, int f, int e, bool k_is_x, Locator_scanner_result& result, const Locator_scanner_result& reference);

    std::unique_ptr<Palette::Matcher> current_matcher_;
    int ythres_=0;
    void _update_matcher(){
        current_matcher_=shared_palette_->get_matcher();

        int thresR=current_matcher_->get_threshold_primary(0);
        int thresG=current_matcher_->get_threshold_primary(1);
        int thresB=current_matcher_->get_threshold_primary(2);
        ythres_=(thresR*299+thresG*587+thresB*114)/1000;
    }

    //return true if black, false if white, not ok if others
    Option<bool> _palette_is_black_or_white(RGB color){
        int res=current_matcher_->match_primary(color);
        if(res==Palette::BLACK)return Some(true);
        else if(res==Palette::WHITE)return Some(false);
        else return None<bool>();
    }
    //return true if black.
    bool _y_is_black_or_white(int Y){
        return Y<ythres_;
    }
public:
	struct Locator_scanner_result{
		//if not success, other results are undefined
        bool is_primary;

		int center_x;
		int center_y;
		float likelihood;

        //if not primary, the following two are estimated
		double estimated_symbol_size_x;
		double estimated_symbol_size_y;

        double black_symbol_size_x;
        double black_symbol_size_y;
        RGB color_black;
        RGB color_white;

        Locator_scanner_result(){}
	};

	Locator_scanner(Pixel_reader* reader);

    bool locate_single_secondary(Locator_scanner_result&, int approx_x, int approx_y, const Locator_scanner_result& ref);
    bool track_single_secondary(Locator_scanner_result& ref_result){
        auto copy=ref_result;
        return locate_single_secondary(ref_result, ref_result.center_x,ref_result.center_y,copy);
    }

	bool locate_single_primary(Locator_scanner_result&, int, int, const Locator_scanner_result& ref);
    bool locate_single_primary(Locator_scanner_result&, int l, int t, int r, int b);
    bool track_single_primary(Locator_scanner_result&);

	int locate_multiple_primary(Locator_scanner_result* result_buffer, int maximum_count);
    int locate_multiple_primary(Locator_scanner_result* result_buffer, int maximum_count, int l, int t, int r, int b);


    void _find_adj(Mat<Option<Locator_scanner::Locator_scanner_result>>& mat, int x, int y, std::queue<Point>& queue);
    bool _guess_and_locate(const Locator_scanner::Locator_scanner_result& p1, const Locator_scanner::Locator_scanner_result& p2,
                           Option<Locator_scanner::Locator_scanner_result>& result, bool is_primary);
    Option<Mat<Option<Locator_scanner_result>>> get_locator_net(int vcnt, int hcnt);
    Option<Mat<Option<Locator_scanner_result>>> update_locator_net(Mat<Option<Locator_scanner_result>> prev);
};
#endif