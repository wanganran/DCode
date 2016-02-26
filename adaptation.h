//
// Created by 王安然 on 16/2/1.
//

#include "structures.h"
#include "palette.h"

#ifndef DCODE_ADAPTATION_H
#define DCODE_ADAPTATION_H

#endif //DCODE_ADAPTATION_H

//this class is to receive probes, error stats, block availabilities, etc.
//and output actions.
class Adaptation_unit{
private:

    class Palette_analyzer{
    private:
        struct Stat_t{
            int sum[3];
            int power_sum[3];
            int tot;
            Stat_t(){
                memset(sum,0,sizeof(int)*3);
                memset(power_sum,0,sizeof(int)*3);
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
            void clear(){
                sum[0]=sum[1]=sum[2]=0;
                power_sum[0]=power_sum[1]=power_sum[2]=0;
                tot=0;
            }
        } stat[64];


        inline bool _match(int value, int filter, int data){
            return (value&filter)==(data&filter);
        }

    public:
        Palette_analyzer(){
        }
        ~Palette_analyzer(){
        }
        //supervised statistic
        void add(int type, const RGB color){
            stat[type].add(color);
        }
        //get the statistics of a given channel.
        //'channel' indicates which primary/secondary channel you want to count,
        //'filter_mask' indicates which channels are needed to be matched, (0-63)
        //'value_mask' indicates the actual value of the channels to be matched, (0-63)
        //return the total number of involved entries.
        int get_stat(int channel,int filter_mask, int value_mask, double& out_mean, double& out_variance){
            int mean=0;
            int mean_tot=0;
            int power_sum=0;
            for(int i=0;i<64;i++)
                if(_match(value_mask, filter_mask, i)){
                    mean+=stat[i].sum[channel];
                    mean_tot+=stat[i].tot;
                    power_sum+=stat[i].power_sum[channel];
                }
            out_mean=mean/(double)mean_tot;
            out_variance=power_sum/(double)mean_tot-out_mean*out_mean;
            return mean_tot;
        }
        void reset(){
            for(int i=0;i<64;i++)
                stat[i].clear();
        }
    };
    int last_action_id_;
    Palette_analyzer current_analyzer_;

    Adaptation_unit():last_action_id_(-1){}
public:
    void input_probe(const Rx_PHY_probe_result& probe);
    void input_valid_data_block(const Rx_block& block);
    void input_availability(const Rx_frame_availability& avail);
    void input_error_analysis(const Rx_frame_error_rate& err);

    bool fetch_action(Tx_PHY_action& out_action);

    static Adaptation_unit& shared(){
        static Adaptation_unit singleton;
        return singleton;
    }
};