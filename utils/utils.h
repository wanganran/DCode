//
// Created by 王安然 on 15/7/16.
//

#ifndef DCODE_UTILS_H
#define DCODE_UTILS_H

//debug
#include <functional>
#include <type_traits>

#define DEBUG(...) printf(__VA_ARGS__)
#define WARN(...) {printf("WARNING:\t"); printf(__VA_ARGS__);}
#define ERROR(...) {printf("ERROR:\t"); printf(__VA_ARGS__); exit(0);}

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
    static constexpr int NORMAL_DIST_SIZE= sizeof(NORMAL_DIST)/sizeof(NORMAL_DIST[0]);
public:
    //normalized
    static constexpr double N(double mean, double theta, double x){
        auto t=abs((int)((x-mean)/theta*100+0.5));
        if(t>=NORMAL_DIST_SIZE)t=NORMAL_DIST_SIZE-1;
        return NORMAL_DIST[t]/NORMAL_DIST[0];
    }
    static constexpr double N(double mean, double x){return N(mean,1.0,x);}
};


//structures
struct Point{
    int x,y;
    Point(int _x, int _y):x(_x),y(_y){}
    Point():x(0),y(0){}
};
struct PointF{
    double x,y;
    PointF(double _x, double _y):x(_x),y(_y){}
    PointF(const PointF&)=default;
    PointF(const Point& p):x(p.x),y(p.y){}
    PointF& operator =(const PointF& p)=default;
    PointF& operator =(const Point& p){
        this->x=p.x;
        this->y=p.y;
        return *this;
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

    const T& get_reference_or(const T& def) const{
        if(!ptr_)return def;
        else return *ptr_;
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
    return x|((x&4)>>2)|((x&2)>>1);
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
#endif //DCODE_UTILS_H