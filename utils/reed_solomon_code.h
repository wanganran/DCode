/*
 * reed_solomon.h
 *
 *  Created on: 2014/7/30
 *      Author: Anran
 */

#ifndef REED_SOLOMON_H_
#define REED_SOLOMON_H_

#include <assert.h>
#include <memory>

class Reed_solomon_code{
private:
	int n,k;
	int npar;
	uint8_t encode_gx[255];
	uint8_t * calc_sigma_mbm(uint8_t* syn,uint8_t* dest, int& res_len);
	bool chien_search(int length,int start,uint8_t wa,uint8_t seki, uint8_t& res1,uint8_t& res2);
	bool chien_search(int length,uint8_t* sigma,int sigma_len,uint8_t* res,int& res_len);
	void do_forney(uint8_t* data,int len,uint8_t* pos, int pos_len, uint8_t* sigma, int sigma_len, uint8_t* omega, int omega_len);
public:
	Reed_solomon_code(int _n,int _k);
	uint8_t* encode(uint8_t * input,uint8_t * parity);
	uint8_t* encode(uint8_t * input);
	uint8_t* decode(uint8_t * input);
	~Reed_solomon_code();

};

class Reed_solomon_code_buffered{
private:
    int n_,k_;
    std::unique_ptr<Reed_solomon_code> coder_;
public:
    Reed_solomon_code_buffered():coder_(nullptr), n_(-1){}
    std::unique_ptr<Reed_solomon_code>& get_coder(int n, int k){
        if(n!=n_ || k!=k_) {
            n_ = n;
            k_ = k;
            coder_.reset(new Reed_solomon_code(n,k));
        }
        return coder_;
    }
};




#endif /* REED_SOLOMON_H_ */
