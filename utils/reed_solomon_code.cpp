/*
 * Reed_solomon_code.cpp
 *
 *  Created on: 2014/7/30
 *      Author: Anran
 */
#include <string.h>
#include "Reed_solomon_code.h"

template<typename T=uint8_t>
class galois{

private:
	int polynomial;
	int sym_start;

	void init_galois_table(){
		int d=1;
		for(int i=0;i<255;i++){
			exp_tbl[i]=exp_tbl[255+i]=(T)d;
			log_tbl[d]=i;
			d<<=1;
			if((d&0x100)!=0)
				d=(d^polynomial)&0xff;
		}
	}


public:

	T exp_tbl[255*2];
	T log_tbl[255+1];

	galois(int _polynomial,int _sym_start):polynomial(_polynomial),sym_start(_sym_start){
		init_galois_table();
	}
	inline int to_pos(int length,T a){
		return length-1-log_tbl[a];
	}
	inline T mul(T a,T b){
		return (a==0||b==0)?0:exp_tbl[log_tbl[a]+log_tbl[b]];
	}
	inline T mul_exp(T a,T b){
		return a==0?0:exp_tbl[log_tbl[a]+b];
	}
	inline T div(T a,T b){
		return a==0?0:exp_tbl[log_tbl[a]-log_tbl[b]+255];
	}
	inline T div_exp(T a,T b){
		return a==0?0:exp_tbl[log_tbl[a]-b+255];
	}
	inline T inv(T a){
		return exp_tbl[255-log_tbl[a]];
	}
    inline T min(const T& a,const T& b) {return (a<b?a:b);}
	T* mul_poly(T* a, int alen, T* b,int blen, T* dest, int jisu){
		memset(dest,0,sizeof(T)*jisu);
		int ia2=min(jisu,alen);
		for(int ia=0;ia<ia2;ia++){
			if(a[ia]!=0){
				T loga=log_tbl[a[ia]];
				int ib2=min(blen,(jisu-ia));
				for(int ib=0;ib<ib2;ib++){
					if(b[ib]!=0)
						dest[ia+ib]^=exp_tbl[loga+log_tbl[b[ib]]];
				}
			}
		}
		return dest;
	}

	bool calc_syndrome(T* data,int len,T* syn,int syn_len){
		int has_err=0;
		for(int i=0,s=sym_start;i<syn_len;i++,s++){
			T wk=0;
			for(int idx=0;idx<len;idx++){
				if(wk!=0)
					wk=exp_tbl[log_tbl[wk]+s];
				wk^=data[idx];
			}
			syn[i]=wk;
			has_err|=wk;
		}
		return has_err==0;
	}
	T* make_encode_gx(T* dest,int npar){
		memset(dest,0,sizeof(T)*npar);
		dest[npar-1]=1;
		for(int i=0,kou=sym_start;i<npar;i++,kou++){
			T ex=exp_tbl[kou];
			for(int j=0;j<npar-1;j++)
				dest[j]=mul(dest[j],ex)^dest[j+1];
			dest[npar-1]=mul(dest[npar-1],ex);
		}
		return dest;
	}
	T calc_omega_value(T* omega,int len,int zlog){
		T wz=(T)zlog;
		T ov=omega[0];
		for(int i=1;i<len;i++){
			ov^=mul_exp(omega[i],wz);
			wz=(T)((wz+zlog)%255);
		}
		if(sym_start!=0)
			ov=mul_exp(ov,(T)((zlog*sym_start)%255));
		return ov;
	}
	T calc_sigma_dash_value(T* sigma,int len,int zlog){
		int jisu=len-1;
		int zlog2=(zlog*2)%255;
		uint8_t wz=(uint8_t)zlog2;
		T dv=sigma[1];
		for(int i=3;i<=jisu;i+=2){
			dv^=mul_exp(sigma[i],wz);
			wz=(uint8_t)((wz+zlog2)%255);
		}
		return dv;
	}
};
class galois_qr:public galois{
public:
	galois_qr():galois(0x1d,0){}
};

static galois_qr shared_galois;

Reed_solomon_code::Reed_solomon_code(int _n,int _k):n(_n),k(_k),npar(_n-_k){
	assert(_n<=255&&_k>0&&_k<_n);
	shared_galois.make_encode_gx(encode_gx,npar);
}
uint8_t* Reed_solomon_code::encode(uint8_t* input, uint8_t* parity){
	assert(input!=NULL&&parity!=NULL);
	memset(parity,0,sizeof(uint8_t)*npar);
	for(int idx=0;idx<k;idx++){
		uint8_t c=input[idx];
		uint8_t ib=parity[0]^c;
		for(int i=0;i<npar-1;i++)
			parity[i]=parity[i+1]^shared_galois.mul(ib,encode_gx[i]);
		parity[npar-1]=shared_galois.mul(ib,encode_gx[npar-1]);
	}
	return input;
}
uint8_t* Reed_solomon_code::encode(uint8_t* input){
	return encode(input,input+k);
}

uint8_t* Reed_solomon_code::calc_sigma_mbm(uint8_t* syn,uint8_t* dest, int& res_len){

	uint8_t _sg0[256];
	uint8_t _sg1[256];
	uint8_t _wk[256];
	memset(_sg0,0,sizeof(_sg0));
	memset(_sg1,0,sizeof(_sg1));
	memset(_wk,0,sizeof(_wk));

	uint8_t* sg0=_sg0;
	uint8_t* sg1=_sg1;
	uint8_t* wk=_wk;

	sg0[1]=1;
	sg1[0]=1;
	int jisu0=1;
	int jisu1=0;
	int m=-1;

	for(int n=0;n<npar;n++){
		uint8_t d=syn[n];
		for(int i=1;i<=jisu1;i++)
			d^=shared_galois.mul(sg1[i],syn[n-i]);
		if(d!=0){
			uint8_t logd=shared_galois.log_tbl[d];
			for(int i=0;i<=n;i++)
				wk[i]=sg1[i]^shared_galois.mul_exp(sg0[i],logd);
			int js=n-m;


			if(js>jisu1){
				for(int i=0;i<=jisu0;i++)
					sg0[i]=shared_galois.div_exp(sg1[i],logd);
				m=n-jisu1;
				jisu1=js;
				jisu0=js;
			}
			uint8_t* tmp=sg1;
			sg1=wk;
			wk=tmp;
		}
		for(int i=jisu0;i>0;i--)
			sg0[i]=sg0[i-1];
		sg0[0]=0;
		jisu0++;
	}


	if(sg1[jisu1]==0)return NULL;
	memcpy(dest,sg1,sizeof(uint8_t)*(jisu1+1));
	res_len=jisu1+1;
	return dest;
}
bool Reed_solomon_code::chien_search(int length,int start,uint8_t wa,uint8_t seki, uint8_t& res1,uint8_t& res2){
	for(int i=start;i<length;i++){
		uint8_t z0=shared_galois.exp_tbl[i];
		uint8_t z1=wa^z0;
		if(shared_galois.mul_exp(z1,i)==seki){
			uint8_t idx=shared_galois.log_tbl[z1];
			if(idx<=i||idx>=length)return false;
			res1=z1;
			res2=z0;
			return true;
		}
	}
	return false;
}
bool Reed_solomon_code::chien_search(int length,uint8_t* sigma,int sigma_len,uint8_t* res,int& res_len){
	int jisu=sigma_len-1;
	uint8_t wa=sigma[1];
	uint8_t seki=sigma[jisu];
	if(jisu==1){
		if(shared_galois.log_tbl[wa]>=length)return false;
		res[0]=wa;
		res_len=1;
		return true;
	}
	if(jisu==2){
		res_len=2;
		return chien_search(length,0,wa,seki,res[0],res[1]);
	}

	res_len=jisu;
	int pos_idx=jisu-1;
	for(int i=0,z=255;i<length;i++,z--){
		uint8_t wk=1;
		for(int j=1,wz=z;j<=jisu;j++,wz=(wz+z)%255)
			wk^=shared_galois.mul_exp(sigma[j],wz);
		if(wk==0){
			uint8_t pv=shared_galois.exp_tbl[i];
			wa^=pv;
			seki=shared_galois.div(seki,pv);
			res[pos_idx--]=pv;
			if(pos_idx==1){
				return chien_search(length,i+1,wa,seki,res[0],res[1]);
			}
		}
	}

	return false;
}


void Reed_solomon_code::do_forney(uint8_t* data,int len,uint8_t* pos, int pos_len, uint8_t* sigma, int sigma_len, uint8_t* omega, int omega_len){
	for(int i=0;i<pos_len;i++){
		uint8_t zlog=255-shared_galois.log_tbl[pos[i]];
		uint8_t ov=shared_galois.calc_omega_value(omega,omega_len,zlog);
		uint8_t dv=shared_galois.calc_sigma_dash_value(sigma,sigma_len,zlog);
		data[shared_galois.to_pos(len,pos[i])]^=shared_galois.div_exp(shared_galois.div(ov,dv),zlog);
	}
}

uint8_t* Reed_solomon_code::decode(uint8_t* input){
	uint8_t syn[255];
	if(shared_galois.calc_syndrome(input,n,syn,npar))return input;
	uint8_t sigma[256];
	int sigma_len;
	if(calc_sigma_mbm(syn,sigma,sigma_len)==NULL)return NULL;
	uint8_t pos[256];
	int pos_len;
	if(!chien_search(n,sigma,sigma_len,pos,pos_len))return NULL;
	uint8_t omega[256];
	shared_galois.mul_poly(syn,npar,sigma,sigma_len,omega,sigma_len-1);
	do_forney(input,n,pos,pos_len,sigma,sigma_len,omega,sigma_len-1);
	return input;
}

Reed_solomon_code::~Reed_solomon_code(){}
