//
// Created by Anran on 15/7/2.
//

#ifndef DCODE_PALETTE_H
#define DCODE_PALETTE_H

#include <string.h>
#include <assert.h>
#include "config.h"
#include "utils/RGB.h"
#include "utils/utils.h"

//in charge of managing all the color palettes.
//in DCode, the data are modulated in three channels RGB independently.
//if possible, we can enable a secondary channel for each primary channel, to increase 1bit capacity per color.
class Palette: public Noncopyable{
private:
    static const int channel=3, channel_depth=256;

    int mask_; //the bits one color can present
    int primary_thresholds_[channel]; //there exists a primary threshold for each channel
    int secondary_thresholds_[channel][2]; //each channel can have a sub-channel
    int primary_enable_mask_[channel]; //the bit position corresponded to each primary channel
    int secondary_enable_mask_[channel]; //the bit position corresponded to each secondary channel

    Palette(){
        //by default, enable all primary, disable all secondary
        for(int i=0;i<channel;i++){
            primary_enable_mask_[i]=(1<<i);
            secondary_enable_mask_[i]=0;
            //by default, the threshold is 100.
            //need to be changed
            primary_thresholds_[i]=Config::current().recognition_config.initial_color_channel_threshold;
        }
        mask_=7;
    }

public:
    static const int BLACK=0;
    static const int WHITE=(1<<channel)-1;

    static Palette current(){
        static Palette p;
        return p;
    }

    class Matcher{
        friend class Palette;
    private:
        const Palette* palette_;
        uint8_t values_[channel][channel_depth];
        Matcher(const Palette* conf):palette_(conf){
            memset(values_,0,sizeof(uint8_t)*256*channel_depth);
            for(int i=0;i<channel;i++) {
                for (int j = 0; j < channel_depth; j++)
                    if (j >= conf->primary_thresholds_[i])
                        values_[i][j] |= conf->primary_enable_mask_[i];
            }
            for(int i=0;i<channel;i++){
                for(int j=0;j<channel_depth;j++)
                    if((j>=conf->secondary_thresholds_[i][0]&&j<conf->primary_thresholds_[i])||
                       (j>=conf->secondary_thresholds_[i][1]))
                        values_[i][j]|=conf->secondary_enable_mask_[i];
            }
        }
    public:
        uint8_t match(RGB color){
            return values_[0][color.R]|values_[1][color.G]|values_[2][color.B];
        }

        uint8_t match_primary(RGB color){
            return match(color)&7;
        }

        int get_threshold_primary(int channel) {
            return palette_->primary_thresholds_[channel];
        }

        int get_threshold_secondary(int channel, bool low) {
            return palette_->secondary_thresholds_[channel][low ? 0 : 1];
        }

    };

    class Analyzer{
        friend class Palette;
    private:
        const int mask_;
        struct Stat_t{
            int sum[channel];
            int power_sum[channel];
            int tot;
            Stat_t(){
                memset(sum,0,sizeof(int)*channel);
                memset(power_sum,0,sizeof(int)*channel);
                tot=0;
            }
            void add(const RGB& c){
                sum[0]+=c.R;
                sum[1]+=c.G;
                sum[2]+=c.B;
                power_sum[0]+=c.R*c.R;
                power_sum[1]+=c.G*c.G;
                power_sum[2]+=c.B*c.B;
                tot++;
            }
        } *stat;

        Analyzer(const Palette* conf):mask_(conf->mask_){
            assert(((mask_+1)&mask_)==0);
            stat=new Stat_t[mask_+1];
        }

        inline bool _match(int value, int filter, int data){
            return (value&filter)==(data&filter);
        }

    public:
        ~Analyzer(){
            if(stat)delete[] stat;
        }
        //supervised statistic
        void add(int type, const RGB color){
            stat[type].add(color);
        }
        //get the statistics of a given channel.
        //'channel' indicates which primary/secondary channel you want to count,
        //'filter_mask' indicates which channels are needed to be matched,
        //'value_mask' indicates the actual value of the channels to be matched,
        //return the total number of involved entries.
        int get_stat(int channel,int filter_mask, int value_mask, double& out_mean, double& out_variance){
            int mean=0;
            int mean_tot=0;
            int power_sum=0;
            for(int i=0;i<=mask_;i++)
                if(_match(value_mask, filter_mask, i)){
                    mean+=stat[i].sum[channel];
                    mean_tot+=stat[i].tot;
                    power_sum+=stat[i].power_sum[channel];
                }
            out_mean=mean/(double)mean_tot;
            out_variance=power_sum/(double)mean_tot-out_mean*out_mean;
            return mean_tot;
        }
    };

    class Adjuster {
        friend class Palette;

    private:
        Palette *palette_;

        Adjuster(Palette *conf) : palette_(conf) { }

    public:
        int get_threshold_primary(int channel) {
            return palette_->primary_thresholds_[channel];
        }

        void set_threshold_primary(int channel, int newthres) {
            palette_->primary_thresholds_[channel] = newthres;
        }

        int get_threshold_secondary(int channel, bool low) {
            return palette_->secondary_thresholds_[channel][low ? 0 : 1];
        }

        void set_threshold_secondary(int channel, bool low, int newthres) {
            palette_->secondary_thresholds_[channel][low ? 0 : 1] = newthres;
        }

        bool has_secondary(int channel) const {
            return palette_->secondary_enable_mask_[channel] > 0;
        }

        int enable_secondary(int channel, int threshold_low, int threshold_high) {
            palette_->secondary_thresholds_[channel][0] = threshold_low;
            palette_->secondary_thresholds_[channel][1] = threshold_high;

            if (palette_->secondary_enable_mask_[channel] == 0) {
                palette_->secondary_enable_mask_[channel] = palette_->mask_ + 1;
                palette_->mask_ |= palette_->mask_ + 1;
                return palette_->mask_ + 1;
            }
            else
                return palette_->secondary_enable_mask_[channel];
        }

        void disable_secondary(int channel) {
            if (palette_->secondary_enable_mask_[channel] != 0) {
                int newmask = palette_->mask_ ^palette_->secondary_enable_mask_[channel];
                if ((newmask | (newmask + 1)) != 0) {
                    int highest_mask = ((palette_->mask_ + 1) >> 1);
                    if (palette_->secondary_enable_mask_[(channel + 1) % 3] == highest_mask) {
                        palette_->secondary_enable_mask_[(channel + 1) % 3] = palette_->secondary_enable_mask_[channel];
                    }
                    else if (palette_->secondary_enable_mask_[(channel + 2) % 3] == highest_mask) {
                        palette_->secondary_enable_mask_[(channel + 2) % 3] = palette_->secondary_enable_mask_[channel];
                    }
                    else
                        assert(false);
                    palette_->mask_ >>= 1;
                }
                else palette_->mask_ = newmask;
            }
            palette_->secondary_enable_mask_[channel] = 0;
        }
    };

    std::unique_ptr<Matcher> get_matcher() const{
        return std::unique_ptr<Matcher>(new Matcher(this));
    }

    std::unique_ptr<Analyzer> get_analyzer() const{
        return std::unique_ptr<Analyzer>(new Analyzer(this));
    }

    std::unique_ptr<Adjuster> get_adjuster(){
        return std::unique_ptr<Adjuster>(new Adjuster(this));
    }

    //return the bit mask
    int get_mask() const{return mask_;}


};

#endif //DCODE_PALETTE_H
