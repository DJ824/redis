
#include <cstdint>
#include <cstddef>
#include <algorithm>

struct AVLNode {
    uint32_t depth = 0; // subtree height
    uint32_t cnt = 0;   // subtree size
    AVLNode *left = nullptr;
    AVLNode *right = nullptr;
    AVLNode *parent = nullptr;
};

static void avl_init(AVLNode *node) {
    node->depth = 1;
    node->cnt = 1;
    node->left = node->right = node->parent = nullptr;
}

static uint32_t avl_depth(AVLNode *node) {
    return node ? node->depth : 0;
}

static uint32_t avl_cnt(AVLNode *node) {
    return node ? node->cnt : 0;
}

static void avl_update(AVLNode *node) {
    node->depth = 1 + std::max(avl_depth(node->left), avl_depth(node->right));
    node->cnt = 1 + avl_cnt((node->left) + avl_cnt(node->right));
}

static AVLNode *rot_left(AVLNode *node) {
    AVLNode *new_node = node->right;
    if (new_node->left) {
        new_node->left->parent = node;
    }
    node->right = new_node->left;   // rotation
    new_node->left = node;          // rotation
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode *rot_right(AVLNode *node) {
    AVLNode *new_node = node->left;
    if (new_node->right) {
        new_node->right->parent = node;
    }
    node->left = new_node->right;
    new_node->right = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_depth(new_node);
    return new_node;
}

// left subtree is too deep
static AVLNode *avl_fix_left(AVLNode *root) {
    if (avl_depth(root->left->left) < avl_depth(root->left->right)) {
        root->left = rot_left(root->left);
    }
    return rot_right(root);
}

static AVLNode *avl_fix_right(AVLNode *root) {
    if (avl_depth(root->right->right) < avl_depth(root->right->left)) {
        root->right = rot_right(root->right);
    }
    return rot_left(root);
}

// fix imabalanced nodes and maitain invariants until the root is reached
static AVLNode *avl_fix(AVLNode *node) {
    while (true) {
        avl_update(node);
        uint32_t l = avl_depth(node->left);
        uint32_t r = avl_depth(node->right);
        AVLNode **from = nullptr;
        if (AVLNode *p = node->parent) {
            from = (p->left == node) ? &p->left : &p->right;
        }
        if (l == r + 2) {
            node = avl_fix_left(node);
        } else if (l + 2 == r) {
            node = avl_fix_right(node);
        }
        if (!from) {
            return node;
        }
        *from = node;
        node = node->parent;
    }
}

static AVLNode *avl_del(AVLNode *node) {
    if (node->right == nullptr) {
        AVLNode *parent = node->parent;
        if (node->left) {
            node->left->parent = parent;
        }
        if (parent) {
            (parent->left == node ? parent->left : parent->right) = node->left;
            return avl_fix(parent);
        } else {
            return node->left;
        }
    } else {
        AVLNode *successor = node->right;
        while (successor->left) {
            successor = successor->left;
        }
        AVLNode *root = avl_del(successor);
        *successor = *root;
        if (successor->left) {
            successor->left->parent = successor;
        }
        if (successor->right) {
            successor->right->parent = successor;
        }
        if (AVLNode *parent = node->parent) {
            (parent->left == node ? parent->left : parent->right) = successor;
            return root;
        } else {
            return successor;
        }
    }
}

// offset into the succeeding or preceding node
// worst case is o(log(n))
static AVLNode *avl_offset(AVLNode *node, int64_t offset) {
    // relative to starting node
    int64_t pos = 0;
    while (offset != pos) {
        if (pos < offset && pos + avl_cnt(node->right) >= offset) {
            // target is inside the right subtree
            node = node->right;
            pos += avl_cnt(node->left) + 1;
        } else if (pos > offset && pos - avl_cnt(node->left) <= offset) {
            // target in left subtree
            node = node->left;
            pos -= avl_cnt(node->right) + 1;
        } else {
            AVLNode *parent = node->parent;
            if (!parent) {
                return nullptr;
            }
            if (parent->right == node) {
                pos -= avl_cnt(node->left) + 1;
            } else {
                pos += avl_cnt(node->right) + 1;
            }
            node = parent;
        }
    }
    return node;
}