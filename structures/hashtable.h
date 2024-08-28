//
// Created by Devang Jaiswal on 8/20/24.
//

#ifndef REDIS_HASHTABLE_H
#define REDIS_HASHTABLE_H
#include <cstddef>
#include <cstdint>

struct HNode {
    HNode *next = nullptr;
    uint64_t hcode = 0;
};

struct HTab {
    HNode **tab = nullptr; // array of nodes
    size_t mask = 0;
    size_t size = 0;
};

// real hashtable interface, uses 2 tables for progressive resizing
struct HMap {
    HTab ht1; // newer
    HTab ht2;
    size_t resizing_pos = 0;
};

HNode *hm_lookup(HMap *hmap, HNode *key, bool(*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_pop(HMap *hmap, HNode *key, bool(*eq)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_destroy(HMap *hmap);




#endif //REDIS_HASHTABLE_H
