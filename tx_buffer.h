//
// Created by 王安然 on 16/1/8.
//

#ifndef DCODE_TX_BUFFER_H
#define DCODE_TX_BUFFER_H

#include "structures.h"
#include "modulator.h"
#include <thread>
#include <future>
#include <mutex>

class Tx_buffer{
private:
    static const int ID_RANGE=256;
    static const int R=3; //repeat NACK
    static const int NR=30; //maximum frames per RTT

    constexpr int _SHIFT2(int x){
        int shift=0;
        while(x>0){
            x>>=1;
            shift++;
        }
        return 1<<shift;
    }
    static const int BUFFER_SIZE=_SHIFT2(NR+K*(R+1)+D);
    static const int K=5; //if this frame is K frames later than last sent NACK, or current negative blocks are exceeding block size, and there exist nagative blocks since last sent NACK, then send a new NACK.
    static const int D=2; //regard the most recent D received frames as non-complete frames
    static const int MAX_BLOCK_PER_PACKET=64;

    struct Packet_ref{
        std::shared_ptr<Packet> pkt;
        int pos;
        int len;
        Packet_ref(std::shared_ptr<Packet> _pkt, int _pos, int _len):pkt(_pkt), pos(_pos),len(_len){}
        Packet_ref(): pkt(nullptr), pos(0),len(0){}
    };

    struct Block_ref{
    public:
        Block_type block_type;
        Packet_ref in_packet;
    };

    struct In_ref{
    public:
        static const int PACKET=1;
        static const int ACTION=2;
        static const int PROBE=3;

        In_ref(In_ref&& ref)=default;

        int in_type;
        std::promise<bool> sent;

        union _Workload{
            std::shared_ptr<Packet> in_packet;
            Tx_PHY_action in_action;
            Tx_PHY_probe in_probe;
            _Workload(std::shared_ptr<Packet> packet):in_packet(packet){}
            _Workload(const Tx_PHY_probe& probe):in_probe(probe){}
            _Workload(const Tx_PHY_action& action):in_action(action){}
        } workload;

        In_ref(std::shared_ptr<Packet> packet):in_type(PACKET), workload(packet){}
        In_ref(const Tx_PHY_probe& probe):in_type(PROBE), workload(probe){}
        In_ref(const Tx_PHY_action& action):in_type(ACTION), workload(action){}
    };

    std::queue<In_ref> urgent_queue_;
    std::queue<In_ref> regular_queue_;
    std::mutex queue_mutex_;

    Screen_fetcher* fetcher_;
    std::thread worker_thread_;
    bool worker_thread_flag_;
    static void worker_thread_func(Tx_buffer*, Screen_fetcher*);

    void _update_block_ref(int fid, int block_id, const Packet_ref& ref);
    void _update_block_ref(int fid, int block_id, Block_type type);
    std::shared_ptr<Packet> _form_retrans_packet(const Ack& ack);
    std::shared_ptr<Packet> _form_ack_packet(const Ack& ack);
public:

    std::future<bool> push_packet(uint8_t* source, int length);
    std::future<bool> push_ack(const Ack& ack);
    std::future<bool> push_action_block(const Tx_PHY_action& action);
    std::future<bool> push_probe_block(const Tx_PHY_probe& probe);

    void reset(Screen_fetcher* fetcher);

    Tx_buffer(Screen_fetcher* fetcher);
};

#endif //DCODE_TX_BUFFER_H
