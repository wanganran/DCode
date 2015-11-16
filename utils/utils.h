//
// Created by 王安然 on 15/7/16.
//

#ifndef DCODE_UTILS_H
#define DCODE_UTILS_H

//debug
#include <functional>
#include <type_traits>
#include <assert.h>

#define DEBUG(...) printf(__VA_ARGS__)
#define WARN(...) {printf("WARNING:\t"); printf(__VA_ARGS__);}
#define ERROR(...) {printf("ERROR:\t"); printf(__VA_ARGS__); exit(0);}

#define MAX_INT ((1<<31)-1)

//statistics consts

struct Normal_dist{
private:
    static constexpr double THETA=1.0;
    static constexpr double NORMAL_DIST[]={
        0.398942280401433, 0.398922333786082, 0.398862499923666, 0.3987627967621, 0.398623254204605, 0.398443914094764, 0.398224830195607, 0.397966068162751, 0.397667705511609, 0.397329831578688,
        0.396952547477012, 0.396535966045686, 0.396080211793656, 0.395585420837687, 0.395051740834611, 0.394479330907889, 0.393868361568541, 0.393219014630497, 0.392531483120429, 0.391805971182121,
        0.391042693975456, 0.390241877570074, 0.38940375883379, 0.388528585315836, 0.387616615125014, 0.386668116802849, 0.385683369191816, 0.384662661298743, 0.383606292153479, 0.382514570662924,
        0.381387815460524, 0.380226354751325, 0.379030526152702, 0.377800676530865, 0.376537161833254, 0.375240346916938, 0.373910605373128, 0.372548319347933, 0.371153879359466, 0.369727684111432,
        0.368270140303323, 0.366781662437336, 0.365262672622154, 0.363713600373713, 0.362134882413092, 0.360526962461648, 0.358890291033545, 0.357225325225801, 0.355532528505997, 0.35381237049778,
        0.3520653267643, 0.350291878589726, 0.348492512758974, 0.346667721335792, 0.344818001439333, 0.342943855019384, 0.341045788630353, 0.339124313204192, 0.337179943822381, 0.335213199487106,
        0.3332246028918, 0.331214680191153, 0.329183960770765, 0.327132977016554, 0.325062264084082, 0.322972359667914, 0.320863803771173, 0.318737138475402, 0.316592907710893, 0.314431657027597,
        0.312253933366761, 0.310060284833416, 0.307851260469853, 0.30562741003021, 0.3033892837563, 0.301137432154804, 0.298872405775953, 0.296594754993816, 0.294305029788325, 0.292003779529141,
        0.289691552761483, 0.287368896994028, 0.285036358489007, 0.28269448205458, 0.280343810839621, 0.277984886130996, 0.275618247153457, 0.273244430872216, 0.270863971798338, 0.268477401797002,
        0.266085249898755, 0.263688042113818, 0.261286301249553, 0.258880546731149, 0.25647129442562, 0.254059056469189, 0.251644341098117, 0.249227652483066, 0.246809490567043, 0.244390350907,
        0.241970724519143, 0.239551097728013, 0.23713195201938, 0.234713763897012, 0.232297004743366, 0.229882140684233, 0.227469632457386, 0.22505993528527, 0.222653498751761, 0.220250766683033,
        0.217852177032551, 0.21545816177022, 0.213069146775718, 0.210685551736015, 0.208307790047108, 0.205936268719975, 0.203571388290759, 0.201213542735197, 0.198863119387276, 0.196520498862136,
        0.194186054983213, 0.191860154713599, 0.18954315809164, 0.18723541817073, 0.184937280963305, 0.182649085389022, 0.18037116322708, 0.178103839072694, 0.175847430297662, 0.173602247015033,
        0.171368592047807, 0.169146760901672, 0.166937041741714, 0.164739715373077, 0.162555055225534, 0.16038332734192, 0.158224790370383, 0.156079695560421, 0.153948286762634, 0.151830800432162,
        0.149727465635745, 0.147638504062356, 0.145564130037348, 0.143504550540062, 0.141459965224839, 0.13943056644536, 0.137416539282282, 0.135418061574071, 0.133435303951002, 0.131468429872231,
        0.129517595665892, 0.127582950572142, 0.125664636789088, 0.123762789521523, 0.121877537032402, 0.120009000696986, 0.118157295059582, 0.116322527892807, 0.114504800259292, 0.112704206575771,
        0.110920834679456, 0.109154765896647, 0.107406075113484, 0.105674830848764, 0.103961095328764, 0.102264924563978, 0.100586368427691, 0.0989254707363237, 0.0972822693314675, 0.095656796163524,
        0.0940490773768869, 0.0924591333965807, 0.0908869790162829, 0.089332623487655, 0.0877960706109056, 0.0862773188265115, 0.0847763613080222, 0.0832931860558745, 0.0818277759921428, 0.0803801090561542,
        0.0789501583008941, 0.077537891990134, 0.0761432736962073, 0.0747662623983676, 0.0734068125816569, 0.072064874336218, 0.0707403934569834, 0.0694333115436742, 0.0681435661010446, 0.0668710906393071,
        0.0656158147746766, 0.0643776643299694, 0.0631565614351987, 0.0619524246281052, 0.0607651689545648, 0.0595947060688161, 0.0584409443334515, 0.0573037889191171, 0.056183141903868, 0.0550789023721258,
        0.0539909665131881, 0.0529192277192403, 0.0518635766828206, 0.0508239014936912, 0.0498000877350708, 0.0487920185791828, 0.047799574882077, 0.0468226352776832, 0.0458610762710549, 0.0449147723307671,
        0.0439835959804272, 0.0430674178892657, 0.0421661069617703, 0.0412795304263304, 0.0404075539228603, 0.0395500415893702, 0.0387068561474556, 0.0378778589866775, 0.0370629102478065, 0.0362618689049062,
        0.0354745928462314, 0.0347009389539188, 0.0339407631824492, 0.0331939206358611, 0.0324602656436974, 0.0317396518356674, 0.0310319322150082, 0.0303369592305316, 0.0296545848473413, 0.0289846606162094,
        0.0283270377416012, 0.0276815671483366, 0.0270480995468818, 0.0264264854972618, 0.0258165754715877, 0.0252182199151944, 0.0246312693063825, 0.024055574214763, 0.0234909853582014, 0.0229373536583607,
        0.0223945302948429, 0.0218623667579294, 0.0213407148999228, 0.0208294269850922, 0.0203283557382258, 0.0198373543917953, 0.019356276731737, 0.0188849771418562, 0.018423310646862, 0.0179711329540397,
        0.0175283004935685, 0.017094670457497, 0.0166701008373811, 0.0162544504606005, 0.0158475790253608, 0.0154493471343952, 0.0150596163273774, 0.01467824911206, 0.0143051089941497, 0.0139400605059358,
        0.0135829692336856, 0.0132337018438214, 0.0128921261078953, 0.0125581109263782, 0.012231526351278, 0.0119122436076052, 0.0116001351137026, 0.0112950745004561, 0.0109969366294056, 0.0107055976097722,
        0.0104209348144226, 0.0101428268947871, 0.00987115379475113, 0.00960579676353959, 0.00934663836761228, 0.00909356250159105, 0.00884645439823722, 0.00860520063749967, 0.00836968915465302, 0.00813980924754602,
        0.00791545158297997, 0.00769650820223733, 0.00748287252578056, 0.00727443935714123, 0.00707110488601945, 0.00687276669061398, 0.00667932373920262, 0.00649067639099337, 0.00630672639626593, 0.00612737689582369,
        0.00595253241977585, 0.00578209888566948, 0.00561598359599097, 0.00545409523505655, 0.00529634386531102, 0.00514264092305394, 0.00499289921361238, 0.00484703290597895, 0.00470495752693398, 0.00456658995467015,
        0.00443184841193801, 0.00430065245873045, 0.00417292298452396, 0.00404858220009443, 0.00392755362892478, 0.00380976209822181, 0.00369513372955903, 0.00358359592916236, 0.00347507737785494, 0.00336950802067748,
        0.00326681905619992, 0.00316694292554008, 0.00306981330110474, 0.00297536507506825, 0.00288353434760344, 0.00279425841487945, 0.0027074757568407, 0.00262312602478102, 0.00254115002872652, 0.0024614897246407
    };
    static constexpr double NORMAL_DIST_CUL[]={
            0.5,	0.50399,	0.50798,	0.51197,	0.51595,	0.51994,	0.52392,	0.5279,	0.53188,	0.53586,
            0.5398,	0.5438,	0.54776,	0.55172,	0.55567,	0.55966,	0.5636,	0.56749,	0.57142,	0.57535,
            0.5793,	0.58317,	0.58706,	0.59095,	0.59483,	0.59871,	0.60257,	0.60642,	0.61026,	0.61409,
            0.61791,	0.62172,	0.62552,	0.6293,	0.63307,	0.63683,	0.64058,	0.64431,	0.64803,	0.65173,
            0.65542,	0.6591,	0.66276,	0.6664,	0.67003,	0.67364,	0.67724,	0.68082,	0.68439,	0.68793,
            0.69146,	0.69497,	0.69847,	0.70194,	0.7054,	0.70884,	0.71226,	0.71566,	0.71904,	0.7224,
            0.72575,	0.72907,	0.73237,	0.73565,	0.73891,	0.74215,	0.74537,	0.74857,	0.75175,	0.7549,
            0.75804,	0.76115,	0.76424,	0.7673,	0.77035,	0.77337,	0.77637,	0.77935,	0.7823,	0.78524,
            0.78814,	0.79103,	0.79389,	0.79673,	0.79955,	0.80234,	0.80511,	0.80785,	0.81057,	0.81327,
            0.81594,	0.81859,	0.82121,	0.82381,	0.82639,	0.82894,	0.83147,	0.83398,	0.83646,	0.83891,
            0.84134,	0.84375,	0.84614,	0.84849,	0.85083,	0.85314,	0.85543,	0.85769,	0.85993,	0.86214,
            0.86433,	0.8665,	0.86864,	0.87076,	0.87286,	0.87493,	0.87698,	0.879,	0.881,	0.88298,
            0.88493,	0.88686,	0.88877,	0.89065,	0.89251,	0.89435,	0.89617,	0.89796,	0.89973,	0.90147,
            0.9032,	0.9049,	0.90658,	0.90824,	0.90988,	0.91149,	0.91308,	0.91466,	0.91621,	0.91774,
            0.91924,	0.92073,	0.9222,	0.92364,	0.92507,	0.92647,	0.92785,	0.92922,	0.93056,	0.93189,
            0.93319,	0.93448,	0.93574,	0.93699,	0.93822,	0.93943,	0.94062,	0.94179,	0.94295,	0.94408,
            0.9452,	0.9463,	0.94738,	0.94845,	0.9495,	0.95053,	0.95154,	0.95254,	0.95352,	0.95449,
            0.95543,	0.95637,	0.95728,	0.95818,	0.95907,	0.95994,	0.9608,	0.96164,	0.96246,	0.96327,
            0.96407,	0.96485,	0.96562,	0.96638,	0.96712,	0.96784,	0.96856,	0.96926,	0.96995,	0.97062,
            0.97128,	0.97193,	0.97257,	0.9732,	0.97381,	0.97441,	0.975,	0.97558,	0.97615,	0.9767,
            0.97725,	0.97778,	0.97831,	0.97882,	0.97932,	0.97982,	0.9803,	0.98077,	0.98124,	0.98169,
            0.98214,	0.98257,	0.983,	0.98341,	0.98382,	0.98422,	0.98461,	0.985,	0.98537,	0.98574,
            0.9861,	0.98645,	0.98679,	0.98713,	0.98745,	0.98778,	0.98809,	0.9884,	0.9887,	0.98899,
            0.98928,	0.98956,	0.98983,	0.9901,	0.99036,	0.99061,	0.99086,	0.99111,	0.99134,	0.99158,
            0.9918,	0.99202,	0.99224,	0.99245,	0.99266,	0.99286,	0.99305,	0.99324,	0.99343,	0.99361,
            0.99379,	0.99396,	0.99413,	0.9943,	0.99446,	0.99461,	0.99477,	0.99492,	0.99506,	0.9952,
            0.99534,	0.99547,	0.9956,	0.99573,	0.99585,	0.99598,	0.99609,	0.99621,	0.99632,	0.99643,
            0.99653,	0.99664,	0.99674,	0.99683,	0.99693,	0.99702,	0.99711,	0.9972,	0.99728,	0.99736,
            0.99744,	0.99752,	0.9976,	0.99767,	0.99774,	0.99781,	0.99788,	0.99795,	0.99801,	0.99807,
            0.99813,	0.99819,	0.99825,	0.99831,	0.99836,	0.99841,	0.99846,	0.99851,	0.99856,	0.99861,
            0.99865,	0.99869,	0.99874,	0.99878,	0.99882,	0.99886,	0.99889,	0.99893,	0.99896,	0.999,
    };
    static constexpr int NORMAL_DIST_SIZE= sizeof(NORMAL_DIST)/sizeof(NORMAL_DIST[0]);
    static constexpr int NORMAL_DIST_CUL_SIZE=sizeof(NORMAL_DIST_CUL)/sizeof(NORMAL_DIST_CUL[0]);
public:
    //normalized
    static constexpr double N(double mean, double theta, double x){
        auto t=abs((int)((x-mean)/theta*100+0.5));
        if(t>=NORMAL_DIST_SIZE)t=NORMAL_DIST_SIZE-1;
        return NORMAL_DIST[t]/NORMAL_DIST[0];
    }
    static constexpr double N_cul(double mean, double theta, double x){
        auto t=abs((int)((x-mean)/theta*100+0.5));
        if(t>=NORMAL_DIST_CUL_SIZE)t=NORMAL_DIST_CUL_SIZE-1;
        return x>mean?NORMAL_DIST_CUL[t]:(1-NORMAL_DIST_CUL[t]);
    }
    static constexpr double N(double mean, double x){return N(mean,1.0,x);}
};


//structures
struct Point{
    int x,y;
    Point(int _x, int _y):x(_x),y(_y){}
    Point(const PointF& p):x((int)(p.x)),y((int)(p.y)){}
    Point():x(0),y(0){}
    bool is_zero() const {return x==0 && y==0;}

};
struct PointF{
    double x,y;
    PointF(double _x, double _y):x(_x),y(_y){}
    PointF():x(0),y(0){}
    PointF(const PointF&)=default;
    PointF(const Point& p):x(p.x),y(p.y){}
    PointF& operator =(const PointF& p)=default;
    PointF& operator =(const Point& p){
        this->x=p.x;
        this->y=p.y;
        return *this;
    }
    PointF operator *(double d) const{
        return PointF(x*d,y*d);
    }
    PointF operator +(const PointF& p)const {
        return PointF(x+p.x,y+p.y);
    }
    PointF operator /(double d) const{
        return PointF(x/d,y/d);
    }
    PointF operator -(const PointF& p)const{
        return PointF(x-p.x,y-p.y);
    }
};


//Option
template<typename T>
struct Option {
private:
    uint8_t placeholder_[sizeof(T)];
    T *ptr_;

    Option() : ptr_(nullptr) { }

public:
    Option(T &&content) : ptr_(new(placeholder_) T(std::move(content))) {
    }
    Option(const T& content):ptr_(new (placeholder_) T(content)){
    }

    Option(Option<T> &&rhs) : ptr_(rhs.ptr_ != nullptr ? (new(placeholder_) T(std::move(*(rhs.ptr_)))) : nullptr) {
        rhs.ptr_ = nullptr;
    }

    Option(const Option<T> &rhs) : ptr_(rhs.ptr_ != nullptr ? (new(placeholder_) T(*(rhs.ptr_))) : nullptr) {
    }

    Option<T> &operator=(const Option<T> &rhs) {
        if (ptr_)ptr_->~T();
        if (rhs.ptr_)
            ptr_ = new(placeholder_) T(*(rhs.ptr_));
        else ptr_ = nullptr;
        return *this;
    }

    Option<T> &operator=(Option<T> &&rhs) {
        if (ptr_)ptr_->~T();
        if (rhs.ptr_) {
            ptr_ = new(placeholder_) T(std::move(*(rhs.ptr_)));
            rhs.ptr_ = nullptr;
        }
        else ptr_ = nullptr;
        return *this;
    }

    ~Option() {
        if (ptr_)ptr_->~T();
    }

    T& get_reference_or(const T& def) const{
        if(!ptr_)return def;
        else return *ptr_;
    }
    //may be unsafe!
    T& get_reference() const{
        return *ptr_;
    }

    T get_or(T &&def) && {
        if (!ptr_)return std::forward(def);
        else {
            T &res = *ptr_;
            ptr_ = nullptr;
            return std::move(res);
        }
    }

    T get_or(T &&def) const & {
        if (!ptr_)return std::move(def);
        else return *ptr_;
    }

    T get_or(const T& def) && {
        if (!ptr_)return def;
        else {
            T &res = *ptr_;
            ptr_ = nullptr;
            return std::move(res);
        }
    }

    T get_or(const T& def) const & {
        if (!ptr_)return def;
        else return *ptr_;
    }

    bool empty() const { return ptr_ == nullptr; }

    static Option<T> &None() {
        static Option<T> none_;
        return none_;
    }

    template<typename T2>
    bool operator==(const Option<T2 *> &rhs) const {
        if (empty() && rhs.empty())return true;
        else return false;
    }

    template<typename T2>
    bool operator==(const Option<T2> &rhs) const {
        if (empty() && rhs.empty())return true;
        if(empty() || rhs.empty())return false;
        return *ptr_ == *(rhs.ptr_);
    }
    template<typename F>
    Option<F> transform(std::function<Option<F>(const T&)> func) const{
        if(empty())return Option<F>::None();
        else return func(*ptr_);
    };
};

template<typename T>
struct Option<T*> {
private:
    T* content_;
    Option(T* const & content);
    Option<T*>& operator =(const Option<T*>& rhs);
public:
    Option(T* &&content) : content_(std::move(content)) { }
    ~Option(){if(content_)delete content_; content_=nullptr;}

    T *get_or(T* def) {
        if (content_){
            auto res=content_;
            content_=nullptr;
            return res;
        }
        else return def;
    }

    T *get_or(T* const &&def)=delete;

    bool empty() const { return content_ == nullptr; }

    static Option<T *> &None() {
        static Option<T *> none_(nullptr);
        return none_;
    }

    template<typename T2>
    bool operator==(const Option<T2 *> &rhs) const {
        return this->content_ == rhs.content_;
    }

    template<typename T2>
    bool operator==(const Option<T2> &rhs) const {
        if (empty() && rhs.empty())return true;
        else return false;
    }
};


template<typename T>
static Option<T>& None(){return Option<T>::None();}
template<typename T>
static Option<T> Some(T&& x){return Option<T>(std::forward<T>(x));}



//Tuple

template<typename ...T>
using Tp=std::tuple<T...>;

template<typename ...T>
static std::tuple<T...> mk_Tp(T... arr){
    return std::make_tuple(arr...);
}

template<typename Tuple, typename F, int... stack>
struct _match_Tp_helper{
    static void _match(const Tuple& tp, const F& func);
};


template<typename Tuple, typename F, int... stack>
struct _match_Tp_helper<Tuple, F, 0, stack...> {
    static void _match(const Tuple &tp, const F &func) {
        func(std::get<0>(tp), std::get<stack>(tp)...);
    }
};

template<typename Tuple, typename F, int x, int... stack>
struct _match_Tp_helper<Tuple, F, x, stack...>{
    static void _match(const Tuple& tp, const F& func){
        return _match_Tp_helper<Tuple,F,x-1,x,stack...>::_match(tp,func);
    }
};

template<typename F, typename ...T>
static void match_Tp(const std::tuple<T...>& tp, const F& func){
    const int tplen=sizeof...(T);
    _match_Tp_helper<std::tuple<T...>,F,tplen-1>::_match(tp, func);
}

//Mat

template<typename T>
struct Mat{
protected:
    int w_,h_;
    uint8_t* arr_;
    uint8_t* get_arr_(int x, int y){
        return arr_+(x+y*w_)*sizeof(T);
    }
public:
    Mat(int w, int h, std::function<T(int, int)> def_gen=[](int x, int y){return T();}):w_(w),h_(h),arr_(new uint8_t[sizeof(T)*w_*h_]){
        for(int i=0;i<w_;i++)
            for(int j=0;j<h_;j++)
                new (get_arr_(i,j)) T(def_gen(i,j));
    }

    int get_width() const{return w_; }
    int get_height() const{return h_; }

    struct _Partial_sub{
    private:
        int x_;
        Mat<T>* ref_;
    public:
        _Partial_sub(int x, Mat<T>* ref):x_(x),ref_(ref){
        }
        T& operator [](int y) const{
            return *reinterpret_cast<T*>(ref_->get_arr_(x_,y));
        }
    };

    _Partial_sub operator [](int x) const{
        return _Partial_sub(x,this);
    }

    template<typename F>
    Mat<F> transform(std::function<F(const T&)> func) const {
        return Mat<F>(w_,h_,[&](int x, int y){
            return func(this->operator[](x)[y]);
        });
    }

    ~Mat() {
        if (!std::is_pointer<T>::value)
            for (int i = 0; i < w_; i++)
                for (int j = 0; j < h_; j++) {
                    reinterpret_cast<T *>(get_arr_(i, j))->~T();
                }
        delete[] arr_;
    }
};

template<typename T>
class Lazy_mat: private Mat<T*> {
protected:
    std::function<T(int, int)> val_func_;
    uint8_t *buffer_;

    uint8_t *get_arr_buffer_(int x, int y) {
        return buffer_ + (x + y * Mat<T*>::w_) * sizeof(T);
    }

public:
    Lazy_mat(int w, int h, std::function<T(int, int)> func = [](int x, int y) { return T(); })
            : Mat<T *>(w, h, [](int x, int y) { return nullptr; }), val_func_(func), buffer_(new uint8_t[w*h*sizeof(T)]) {
    }
    int get_width() const{return Mat<T*>::w_; }
    int get_height() const{return Mat<T*>::h_; }

    struct _Partial_sub_lazy {
    private:
        int x_;
        Lazy_mat<T> *ref_;
    public:
        _Partial_sub_lazy(int x, Lazy_mat<T> *ref) : x_(x), ref_(ref) { }

        T &operator[](int y)  {
            auto res = reinterpret_cast<T**>(ref_->get_arr_(x_, y));
            if (*res)
                return **res;
            else {
                auto ptr = new(ref_->get_arr_buffer_(x_, y)) T(ref_->val_func_(x_, y));
                *res = ptr;
                return **res;
            }
        }
    };

    _Partial_sub_lazy operator[](int x) {
        return _Partial_sub_lazy(x, this);
    }
    void reset(std::function<T(int,int)> func=[](int x, int y){return T();}){
        val_func_=func;
        if(!std::is_pointer<T>::value){
            for(int i=0;i<Mat<T*>::w_;i++)
                for(int j=0;j<Mat<T*>::h_;j++) {
                    auto t = reinterpret_cast<T **>(Mat<T *>::get_arr_(i, j));
                    if (*t) {
                        (*t)->~T();
                        *t = nullptr;
                    }
                }
        }
    }
    Option<T> get_option(int i, int j){
        auto t=reinterpret_cast<T**>(Mat<T*>::get_arr_(i,j));
        if(*t)return Option<T>(**t);
        else return None<T>();
    }

    template<typename F>
    Lazy_mat<F> transform(std::function<F(const T &)> func) const {
        auto capt_func=val_func_;
        return Lazy_mat<F>(Mat<T*>::w_, Mat<T*>::h_, [capt_func, func](int x, int y) {
            return func(capt_func(x, y));
        });
    }

    ~Lazy_mat() {
        if (!std::is_pointer<T>::value)
            for (int i = 0; i < Mat<T *>::w_; i++)
                for (int j = 0; j < Mat<T *>::h_; j++) {
                    auto t = reinterpret_cast<T **>(Mat<T *>::get_arr_(i, j));
                    if (*t)
                        (*t)->~T();
                }
        delete[] buffer_;
    }
};

//utilities
#define LET(a,b) auto& a=b;
#define IN(a,b,c) ((a>=b)&&(a<c))
#define ARRSIZE(arr) (sizeof(arr)/sizeof(arr[0]))

//defer from Seastar
template <typename Func>
class deferred_action {
    Func _func;
    bool _cancelled = false;
public:
    deferred_action(Func&& func) : _func(std::move(func)) {}
    ~deferred_action() { if(!_cancelled)_func(); }
    void cancel() { _cancelled = true; }
};

template <typename Func>
inline
deferred_action<Func>
defer(Func&& func) {
    return deferred_action<Func>(std::forward<Func>(func));
}

//encoding related
inline uint8_t PAR2(uint8_t x){
    x&=6;
    return (uint8_t)((x&6)|(((x&4)>>2)^((x&2)>>1)));
}
inline bool CHECK_PAR2(uint8_t x){
    return (((x&4)>>2)^((x&2)>>1)^(x&1))==0;
}

//random 0-7
uint8_t rand_8(){
    return (uint8_t)(rand()&7);
}
//random 0-255
uint8_t rand_256(){
    return (uint8_t)(rand()&255);
}

//algo utils
template<typename T>
int bsearch_nomore(T* data, int len, T tofind){
    assert(len>0);
    if(data[0]>tofind)return -1;
    if(data[len-1]<tofind)return len-1;

    int l=0,r=len-1;
    while(l<r-1){
        int mid=(l+r)/2;
        if(data[mid]>tofind)r=mid-1;
        else if(data[mid]==tofind)r=mid;
        else l=mid;
    }
    if(l==r)return l;
    else return data[r]<=tofind?r:l;
}

template<typename T>
int bsearch_less(T* data, int len, T tofind){
    assert(len>0);
    if(data[0]>=tofind)return -1;
    if(data[len-1]<tofind)return len-1;

    int l=0,r=len-1;
    while(l<r-1){
        int mid=(l+r)/2;
        if(data[mid]>=tofind)r=mid-1;
        else l=mid;
    }
    if(l==r)return l;
    else return data[r]<tofind?r:l;
}

//for cached objects and singletons: no copy allowed
struct Noncopyable{
protected:
    Noncopyable(){}
    ~Noncopyable(){}
public:
    Noncopyable(const Noncopyable&)=delete;
    Noncopyable& operator = (const Noncopyable&) =delete;
};

#endif //DCODE_UTILS_H
