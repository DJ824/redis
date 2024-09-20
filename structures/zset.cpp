

#include <string>
#include "zset.h"

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

inline uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}


enum {
    T_STR = 0,  // string
    T_ZSET = 1, // sorted set
};

// structure for the key
struct Entry {
    struct HNode node;
    std::string key;
    uint32_t type = 0;
    std::string val;    // string
    ZSet *zset = nullptr;   // sorted set
};

static ZNode *znode_new(const char* name, size_t len, double score) {
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len);
    assert(node);
    avl_init(&node->tree);
    node->hmap.next = nullptr;
    node->hmap.hcode = str_hash((uint8_t*)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);
    return node;
}

// compare by (score,name)
static bool zless(AVLNode *lhs, double score, const char* name, size_t len) {
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) {
        return zl->score < score;
    }
    int rv = memcmp(zl->name, name, std::min(zl->len, len));
    if (rv != 0) {
        return rv < 0;
    }
    return zl->len < len;
}

static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree);
    return zless(lhs, zr->score, zr->name, zr->len);
}

// insert into AVL tree
static void tree_add(ZSet *zset, ZNode *node) {
    AVLNode *curr = nullptr;
    AVLNode **from = &zset->tree;
    while (*from) {
        curr = *from;
        from = zless(&node->tree, curr) ? &curr->left : &curr->right;
    }
    *from = &node->tree;
    node->tree.parent = curr;
    zset->tree = avl_fix(&node->tree);
}


static void zset_update(ZSet *zset, ZNode *node, double score) {
    if (node->score == score) {
        return;
    }
    zset->tree = avl_del(&node->tree);
    node->score = score;
    avl_init(&node->tree);
    tree_add(zset, node);
}

// add a new (score,name) tupule, or update score of existing tupule
bool zset_add(ZSet *zset, const char* name, size_t len, double score) {
    ZNode *node = zset_lookup(zset, name, len);
    if (node) {
        zset_update(zset, node, score);
        return false;
    } else {
        node = znode_new(name, len, score);
        hm_insert(&zset->hmap, &node->hmap);
        tree_add(zset, node);
        return true;
    }
}

// helper struct for hashtable lookup
struct HKey {
    HNode node;
    const char* name = nullptr;
    size_t len = 0;
};

static bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, hmap);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

// lookup by name
ZNode *zset_lookup(ZSet *zset, const char* name, size_t len) {
    if (!zset->tree) {
        return nullptr;
    }
    HKey key;
    key.node.hcode = str_hash((uint8_t*)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_lookup(&zset->hmap, &key.node, &hcmp);
    return found ? container_of(found, ZNode, hmap) : nullptr;
}

// deletion by name
ZNode *zset_pop(ZSet *zset, const char* name, size_t len) {
    if (!zset->tree) {
        return nullptr;
    }

    HKey key;
    key.node.hcode = str_hash((uint8_t*)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_pop(&zset->hmap, &key.node, &hcmp);
    if (!found) {
        return nullptr;
    }

    ZNode *node = container_of(found, ZNode, hmap);
    zset->tree = avl_del(&node->tree);
}

// find the (score,name) tupule that is greater or equal to the arguement, simple tree search
ZNode *zset_query(ZSet *zset, double score, const char* name, size_t len) {
    AVLNode *found = nullptr;
    AVLNode *cur = zset->tree;
    while (cur) {
        if (zless(cur, score, name, len)) {
            cur = cur->right;
        } else {
            found = cur;
            cur = cur->left;
        }
    }
    return found ? container_of(found, ZNode, tree) : nullptr;
}

// offsert into the succeeding or preceding node
ZNode *znode_offsert(ZNode *node, int64_t offset) {
    AVLNode *tnode = node ? avl_offset(&node->tree, offset) : nullptr;
    return tnode ? container_of(tnode, ZNode, tree) : nullptr;
}

void znode_del(ZNode *node) {
    free(node);
}

static void tree_dispose(AVLNode *node) {
    if (!node) {
        return;
    }
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}

// destroy zset
void zset_dispose(ZSet *zset) {
    tree_dispose(zset->tree);
    hm_destroy(&zset->hmap);
}
