#ifndef RBTNODE_H
#define RBTNODE_H

#include "../hash/hashset.h"

enum Color { RED, BLACK };

template <typename Key>
struct RBTNode {
    Key key;
    HashSet rowIds;
    Color color;
    RBTNode* left;
    RBTNode* right;
    RBTNode* parent;
    int subtreeSize;

    RBTNode(const Key& k, int rowId, Color col = RED)
        : key(k), color(col), left(nullptr), right(nullptr), parent(nullptr), subtreeSize(1) {
        rowIds.insert(rowId);
    }

    RBTNode()
        : key(Key{}), color(BLACK), left(nullptr), right(nullptr), parent(nullptr), subtreeSize(0) {}
};

#endif // RBTNODE_H
