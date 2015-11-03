//
// Created by Anran on 15/5/12.
//

#ifndef DCODE_DATALINK_H
#define DCODE_DATALINK_H

#include "config.h"
#ifdef ANDROID_PLATFORM
#include "pixel_reader_Android.h"
#include "physical_Android.h"
#endif

class Datalink{
public:
	Datalink(Physical* phy);

	struct Pair_info{
		bool sender;
		bool receiver;
		Config config;
		bool resume;
	};

	//currently only support half-duplex transmission.
	//at one time only one device is permitted to send data.
	bool pair(bool sender, Pair_info& another);

	//if timeout, or manually re-pair but failed, or manually close, return false
	bool is_paired(Pair_info& another);

	//if not paired, or this is receiver, always return false.
	//queue it first then send
	//if queue is full, will block.
	bool send(void* data, int size);

	//if not paired, or this is sender, always return false.
	//will immediately return if queue is not empty
	//otherwise, will block.
	//note: 1. packets may lost. 2. packets may be received out of order. But 3. packets are guaranteed to be correctly received.
	bool receive(void* dest);

	bool unpair();
};

#endif