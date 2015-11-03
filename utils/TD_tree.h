//
// Created by 王安然 on 15/8/15.
//

#ifndef DCODE_TD_TREE_H
#define DCODE_TD_TREE_H

#include <assert.h>

template<typename K, typename V>
class TD_tree{
private:
    bool values_need_free_;
    V* values_;

    uint8_t* node_buffer;
    int node_buffer_id_;

    //only a temporary struct
    struct Triple_t{
        K x,y;
        V* value;
    };
    struct Binary_tree_t{
        Binary_tree_t* left, *right;
        bool split_x;
        K x,y;
        V* value;

        Binary_tree_t(Binary_tree_t* _left, Binary_tree_t* _right, bool _split_x, Triple_t& triple):
                left(_left),right(_right), split_x(_split_x), x(triple.x),y(triple.y),value(triple.value){}
    };

    Binary_tree_t* new_node_(Binary_tree_t* left, Binary_tree_t* right, bool split_x, Triple_t& triple){
        Binary_tree_t* res=new(node_buffer+node_buffer_id_*sizeof(Binary_tree_t)) Binary_tree_t(left,right,split_x,triple);
        node_buffer_id_++;
        return res;
    }
    void free_nodes_(){
        while(--node_buffer_id_>=0){
            ((Binary_tree_t*)(node_buffer+node_buffer_id_*sizeof(Binary_tree_t)))->~Binary_tree_t();
        }
        delete[] node_buffer;
    }

    Binary_tree_t* root_;

    bool _cmp_x(Triple_t& t1, Triple_t& t2){return t1.x<t2.x;}
    bool _cmp_y(Triple_t& t1, Triple_t& t2){return t1.y<t2.y;}

    void swap(Triple_t& t1, Triple_t& t2){
        Triple_t tmp=t1;
        t1=t2;
        t2=tmp;
    }

    template<K Triple_t::* XY>
    int find_(Triple_t* arr, int size, int kth){
        assert(size>0);
        assert(size>=kth);
        if(size==1)return 0;
        if(size==2)return 0;

        int l=0,r=size-1;
        int pivot=arr[0].*XY;
        while(l<r){
            while(l<r&&arr[l].*XY<=pivot)l++;
            while(l<r&&arr[r].*XY>pivot)r--;
            if(l!=r)
                swap(arr[l],arr[r]);
            l++;
            r--;
        }
        int s=(l>r?l:(arr[l].*XY<=pivot?(l+1):l));
        if(s>=kth)return find_<XY>(arr, s, kth);
        else return find_<XY>(arr+s, size-s, kth-s);
    }

    Binary_tree_t* insert_(Triple_t* values, int size, bool is_x){
        if(size==0)return nullptr;
        if(size==1)return new_node_(nullptr,nullptr,is_x,values[0]);
        int med_id=is_x?find_<&Triple_t::x>(values, size, size/2):find_<&Triple_t::y>(values,size,size/2);
        auto left=insert_(values, med_id, !is_x);
        auto right=insert_(values+med_id+1, size-med_id-1, !is_x);
        return new_node_(left,right,is_x,values[med_id]);
    }

    inline K dist(K x1, K y1, K x2, K y2){
        return (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
    }
    Binary_tree_t* find_nearest_(Triple_t& v, Binary_tree_t* root, K& best) {
        if ((root->split_x && v.x <= root->x) || (!root->split_x && v.y <= root->y)) {
            auto dist_self = dist(root->x, root->y, v.x, v.y);
            if (root->left == nullptr) {
                //found
                best = dist_self;
                return root;
            }
            else {
                K dist_cand;
                auto candidate = find_nearest_(v, root->left, dist_cand);
                auto dist_perp = (root->split_x ? abs(v.x - root->x) : abs(v.y - root->y));

                Binary_tree_t *res = dist_cand < dist_self ? candidate : root;
                auto curr_min = dist_cand < dist_self ? dist_cand : dist_self;

                if (dist_perp < curr_min && root->right) {
                    K dist_other;
                    Binary_tree_t *nearest_other = find_nearest_(v, root->right, dist_other);
                    if (dist_other < curr_min) {
                        best = dist_other;
                        return nearest_other;
                    }
                    else {
                        best = curr_min;
                        return res;
                    }
                }
            }
        }

        else {
            auto dist_self = dist(root->x, root->y, v.x, v.y);
            if (root->right == nullptr) {
                //found
                best = dist_self;
                return root;
            }
            else {
                K dist_cand;
                auto candidate = find_nearest_(v, root->right, dist_cand);
                auto dist_perp = (root->split_x ? abs(v.x - root->x) : abs(v.y - root->y));

                Binary_tree_t *res = dist_cand < dist_self ? candidate : root;
                auto curr_min = dist_cand < dist_self ? dist_cand : dist_self;

                if (dist_perp < curr_min && root->left) {
                    K dist_other;
                    Binary_tree_t *nearest_other = find_nearest_(v, root->left, dist_other);
                    if (dist_other < curr_min) {
                        best = dist_other;
                        return nearest_other;
                    }
                    else {
                        best = curr_min;
                        return res;
                    }
                }
            }
        }
    }
public:
    TD_tree(const K* xs, const K* ys, const V* values, int capacity, bool copy_v=true):
            root_(nullptr), values_need_free_(copy_v),
            node_buffer(new uint8_t[(capacity+1)*sizeof(Binary_tree_t)]), node_buffer_id_(0){
        if(copy_v){
            values_=new V[capacity];
            for(int i=0;i<capacity;i++)values_[i]=values[i];
        }
        else values_=values;
        Triple_t* triples=new Triple_t[capacity];
        for(int i=0;i<capacity;i++){
            triples[i].x=xs[i];
            triples[i].y=ys[i];
            triples[i].value=values_+i;
        }
        root_=insert_(triples, capacity, true);
        delete[] triples;
    }
    ~TD_tree(){
        free_nodes_();
        if(values_need_free_)delete[] values_;
    }

    V& find_nearest(const K key_x, const K key_y, K& dist){
        Triple_t tmp;
        tmp.x=key_x;
        tmp.y=key_y;
        return *(find_nearest_(tmp,root_,dist)->value);
    }

};
#endif //DCODE_KD_TREE_H
