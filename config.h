//
// Created by Anran on 15/5/12.
//

#ifndef DCODE_CONFIG_H
#define DCODE_CONFIG_H

#include <sys/stat.h>
#include <stdio.h>
#include <utility>

struct Config{
private:
    inline static bool __exists (const char* name) {
        struct stat buffer;
        return (stat (name, &buffer) == 0);
    }
    static const char* CONFIG_PATH(){return "/Users/wanganran/config.txt";}
    static int read_int(FILE* f){
        int res;
        fscanf(f, "%d", &res);
        return res;
    }
    static double read_double(FILE* f){
        double res;
        fscanf(f, "%llf", &res);
        return res;
    }
public:
    struct Hardware_config {
        const int tx_width;
        const int tx_height;
        const int rx_width;
        const int rx_height;
        Hardware_config(int _tx_width,int _tx_height,int _rx_width, int _rx_height):
            tx_width(_tx_width),
            tx_height(_tx_height),
            rx_width(_rx_width),
            rx_height(_rx_height){}
    } hardware_config;
    struct Barcode_config {
        const int supported_block_size[4];

        const int vertical_block_count;
        const int horizontal_block_count;

        Barcode_config(int supported_block_size_0, int supported_block_size_1, int supported_block_size_2, int supported_block_size_3,
        int vcount, int hcount): vertical_block_count(vcount),horizontal_block_count(hcount),
                supported_block_size{supported_block_size_0,supported_block_size_1,supported_block_size_2,supported_block_size_3}{}
    } barcode_config;
	struct Performance_config{
        const int rx_thread_count;
        const int LDPC_max_iteration;

        Performance_config(int _thread_count, int _LDPC_max_iteration):
                rx_thread_count(_thread_count), LDPC_max_iteration(_LDPC_max_iteration){}
    } performance_config;

    struct Recognition_config{
        const int initial_color_channel_threshold;

        const int vignette_depth;
        const double vignette_width;

        enum class Distortion_level{
            LOW_DISTORTION,
            MEDIUM_DISTORTION,
            HIGH_DISTORTION
        } distortion_level;

        Recognition_config(int _initial_color_channel_threshold, int _vignette_depth, int _vignette_width, Distortion_level _distortion_level):
                initial_color_channel_threshold(_initial_color_channel_threshold),
                vignette_depth(_vignette_depth),
                vignette_width(_vignette_width),
                distortion_level(_distortion_level){}
    } recognition_config;


    Config(Hardware_config&& h_config, Barcode_config&& b_config, Performance_config&& p_config, Recognition_config&& r_config):
            hardware_config(std::move(h_config)),
            barcode_config(std::move(b_config)),
            performance_config(std::move(p_config)), recognition_config(r_config) {}

    static std::unique_ptr<Config>& current(){
        static std::unique_ptr<Config> singleton(nullptr);
        if(singleton)return singleton;
        else{
            if(!__exists(CONFIG_PATH())) {
                FILE* fp=fopen(CONFIG_PATH(), "w");
                fprintf(fp,"%d %d %d %d\n", 1920,1080,1902,1080);
                fprintf(fp,"%d %d %d %d\n", 12,16,20,24);
                fprintf(fp,"%d %d\n",2,10);
            }
            FILE *fp = fopen(CONFIG_PATH(), "r");
            int tx_width = read_int(fp);
            int tx_height = read_int(fp);
            int rx_width = read_int(fp);
            int rx_height = read_int(fp);

            int supported_block_size_0 = read_int(fp);
            int supported_block_size_1 = read_int(fp);
            int supported_block_size_2 = read_int(fp);
            int supported_block_size_3 = read_int(fp);

            int vcount=read_int(fp);
            int hcount=read_int(fp);

            int thread_count = read_int(fp);
            int LDPC_max_iteration = read_int(fp);

            int initial_color_channel_threshold=read_int(fp);
            int vignette_depth=read_int(fp);
            double vignette_width=read_double(fp);
            int distortion_level=read_int(fp);

            singleton.reset(new Config(
                Hardware_config(tx_width, tx_height, rx_width, rx_height),
                Barcode_config(supported_block_size_0, supported_block_size_1, supported_block_size_2,
                               supported_block_size_3, vcount,hcount),
                Performance_config(thread_count, LDPC_max_iteration),
                Recognition_config(initial_color_channel_threshold,vignette_depth,vignette_width,distortion_level)
            ));

            return singleton;
        }
    }
};

#endif