//
// Created by Devang Jaiswal on 8/22/24.
//

#ifndef REDIS_DLL_H
#define REDIS_DLL_H

struct DList {
    DList *prev = nullptr;
    DList *next = nullptr;
};

inline void dlist_init(DList *node) {
    node->prev = node->next = node;
}

inline bool dlist_empty(DList *node) {
    return node->next == node;
}

inline void dlist_detach(DList *node) {
    DList *prev = node->prev;
    DList *next = node->next;
    prev->next = next;
    next->prev = prev;
}

inline void dlist_insert_before(DList *target, DList *rookie) {
    DList *prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
}
#endif //REDIS_DLL_H
