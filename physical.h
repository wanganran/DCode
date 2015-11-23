//
// Created by Anran on 15/5/12.
// First version finished on 15/6/10.
//

#ifndef DCODE_PHYSICAL_H
#define DCODE_PHYSICAL_H

#include "config.h"
#include "utils/semaphore.h"

#ifdef ANDROID_PLATFORM
class Screen_fetcher_Android;
class Camera_fetcher_Android;

#include "pixel_reader_Android.h"
#include "screen_painter_Android.h"

typedef Screen_fetcher_Android Screen_fetcher;
typedef Camera_fetcher_Android Camera_fetcher;
typedef Pixel_reader_Android Pixel_reader;
typedef Screen_painter_Android Screen_painter;
#endif

//this class is not thread-safe!
//only one thread is permitted to manupulate this class.
class Physical{
private:
    int rx_width_;
    int rx_height_;
    int tx_width_;
    int tx_height_;

    Screen_fetcher* s_fetcher_;
    Camera_fetcher* c_fetcher_;

public:
	int rx_width(){return rx_width_;}
	int rx_height(){return rx_height_;}
	int tx_width(){return tx_width_;}
	int tx_height(){return tx_height_;}

    Physical(Config&, Screen_fetcher*, Camera_fetcher*);
	//fetch a frame of raw data from camera
	//if no data available currently, it will block until a frame is available
	Pixel_reader* fetch_from_camera();

    //free the raw data from camera. It will then serve for another frame.
    void free_to_camera(Pixel_reader* reader);
	
	//alloc a block of memory of tx_width*tx_height*3 (RGB)
	//if no memory available in the memory pool, it will block
	Screen_painter* get_screen_painter();
	
	//push a memory frame to display queue
	//if queue is full it will block
	//the memory will return to the memory pool after displayed.
	void submit_screen_painter(Screen_painter* mem);
};

#endif