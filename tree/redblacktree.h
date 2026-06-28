#ifndef REDBLACKTREE_H
#define REDBLACKTREE_H

#include "RBTnode.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace std;

template <typename Key>
class RedBlackTree {
private:
    RBTNode<Key>* root;
    RBTNode<Key>* nil;

    void destroy(RBTNode<Key>* node) {
        if (node == nil) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    void updateSubtreeSize(RBTNode<Key>* x) {
        if (x == nil) return;
        x->subtreeSize = x->left->subtreeSize + x->right->subtreeSize + x->rowIds.size();
    }

    void updateSubtreeSizesUp(RBTNode<Key>* curr) {
        while (curr != nil && curr != nullptr) {
            updateSubtreeSize(curr);
            curr = curr->parent;
        }
    }

    void leftRotate(RBTNode<Key>* x) {
        RBTNode<Key>* y = x->right;
        x->right = y->left;
        if (y->left != nil) {
            y->left->parent = x;
        }
        y->parent = x->parent;
        if (x->parent == nil) {
            root = y;
        } else if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
        y->left = x;
        x->parent = y;

        updateSubtreeSize(x);
        updateSubtreeSize(y);
    }

    void rightRotate(RBTNode<Key>* y) {
        RBTNode<Key>* x = y->left;
        y->left = x->right;
        if (x->right != nil) {
            x->right->parent = y;
        }
        x->parent = y->parent;
        if (y->parent == nil) {
            root = x;
        } else if (y == y->parent->left) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }
        x->right = y;
        y->parent = x;

        updateSubtreeSize(y);
        updateSubtreeSize(x);
    }

    void insertFixup(RBTNode<Key>* z) {
        while (z->parent->color == RED) {
            if (z->parent == z->parent->parent->left) {
                RBTNode<Key>* y = z->parent->parent->right;
                if (y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        z = z->parent;
                        leftRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    rightRotate(z->parent->parent);
                }
            } else {
                RBTNode<Key>* y = z->parent->parent->left;
                if (y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rightRotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    leftRotate(z->parent->parent);
                }
            }
        }
        root->color = BLACK;
    }

    void transplant(RBTNode<Key>* u, RBTNode<Key>* v) {
        if (u->parent == nil) {
            root = v;
        } else if (u == u->parent->left) {
            u->parent->left = v;
        } else {
            u->parent->right = v;
        }
        if (v != nil) {
            v->parent = u->parent;
        }
    }

    RBTNode<Key>* minimum(RBTNode<Key>* node) const {
        while (node->left != nil) {
            node = node->left;
        }
        return node;
    }

    void deleteFixup(RBTNode<Key>* x, RBTNode<Key>* xParent) {
        while (x != root && x->color == BLACK) {
            if (x == xParent->left) {
                RBTNode<Key>* w = xParent->right;
                if (w->color == RED) {
                    w->color = BLACK;
                    xParent->color = RED;
                    leftRotate(xParent);
                    w = xParent->right;
                }
                if (w->left->color == BLACK && w->right->color == BLACK) {
                    w->color = RED;
                    x = xParent;
                    xParent = x->parent;
                } else {
                    if (w->right->color == BLACK) {
                        w->left->color = BLACK;
                        w->color = RED;
                        rightRotate(w);
                        w = xParent->right;
                    }
                    w->color = xParent->color;
                    xParent->color = BLACK;
                    w->right->color = BLACK;
                    leftRotate(xParent);
                    x = root;
                }
            } else {
                RBTNode<Key>* w = xParent->left;
                if (w->color == RED) {
                    w->color = BLACK;
                    xParent->color = RED;
                    rightRotate(xParent);
                    w = xParent->left;
                }
                if (w->right->color == BLACK && w->left->color == BLACK) {
                    w->color = RED;
                    x = xParent;
                    xParent = x->parent;
                } else {
                    if (w->left->color == BLACK) {
                        w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = xParent->left;
                    }
                    w->color = xParent->color;
                    xParent->color = BLACK;
                    w->left->color = BLACK;
                    rightRotate(xParent);
                    x = root;
                }
            }
        }
        x->color = BLACK;
    }

    void rangeQueryHelper(RBTNode<Key>* node, const Key& low, const Key& high, vector<int>& result, int& nodesVisited) const {
        if (node == nil) return;
        nodesVisited++;
        if (node->key >= low) {
            rangeQueryHelper(node->left, low, high, result, nodesVisited);
        }
        if (node->key >= low && node->key <= high) {
            vector<int> ids = node->rowIds.toVector();
            result.insert(result.end(), ids.begin(), ids.end());
        }
        if (node->key <= high) {
            rangeQueryHelper(node->right, low, high, result, nodesVisited);
        }
    }

    int countLessThanOrEqual(const Key& val) const {
        RBTNode<Key>* curr = root;
        int count = 0;
        while (curr != nil) {
            if (curr->key <= val) {
                count += curr->left->subtreeSize + curr->rowIds.size();
                curr = curr->right;
            } else {
                curr = curr->left;
            }
        }
        return count;
    }

    int countLessThan(const Key& val) const {
        RBTNode<Key>* curr = root;
        int count = 0;
        while (curr != nil) {
            if (curr->key < val) {
                count += curr->left->subtreeSize + curr->rowIds.size();
                curr = curr->right;
            } else {
                curr = curr->left;
            }
        }
        return count;
    }

    void printHelper(RBTNode<Key>* node, string indent, bool last) const {
        if (node != nil) {
            cout << indent;
            if (last) {
                cout << "R----";
                indent += "   ";
            } else {
                cout << "L----";
                indent += "|  ";
            }
            string sColor = (node->color == RED) ? "RED" : "BLACK";
            cout << node->key << " (" << sColor << ", sz=" << node->subtreeSize << ", ids=" << node->rowIds.size() << ")" << endl;
            printHelper(node->left, indent, false);
            printHelper(node->right, indent, true);
        }
    }

public:
    RedBlackTree() {
        nil = new RBTNode<Key>();
        root = nil;
    }

    ~RedBlackTree() {
        destroy(root);
        delete nil;
    }

    void insert(const Key& key, int rowId) {
        RBTNode<Key>* curr = root;
        while (curr != nil) {
            if (curr->key == key) {
                if (!curr->rowIds.contains(rowId)) {
                    curr->rowIds.insert(rowId);
                    updateSubtreeSizesUp(curr);
                }
                return;
            } else if (key < curr->key) {
                curr = curr->left;
            } else {
                curr = curr->right;
            }
        }

        RBTNode<Key>* z = new RBTNode<Key>(key, rowId);
        z->left = nil;
        z->right = nil;

        RBTNode<Key>* y = nil;
        RBTNode<Key>* x = root;

        while (x != nil) {
            y = x;
            if (z->key < x->key) {
                x = x->left;
            } else {
                x = x->right;
            }
        }

        z->parent = y;
        if (y == nil) {
            root = z;
        } else if (z->key < y->key) {
            y->left = z;
        } else {
            y->right = z;
        }

        updateSubtreeSizesUp(z);
        insertFixup(z);
    }

    bool remove(const Key& key, int rowId) {
        RBTNode<Key>* z = root;
        while (z != nil) {
            if (z->key == key) {
                break;
            } else if (key < z->key) {
                z = z->left;
            } else {
                z = z->right;
            }
        }

        if (z == nil) return false;
        if (!z->rowIds.contains(rowId)) return false;

        z->rowIds.remove(rowId);

        if (z->rowIds.size() > 0) {
            updateSubtreeSizesUp(z);
            return true;
        }

        RBTNode<Key>* y = z;
        Color y_original_color = y->color;
        RBTNode<Key>* x = nullptr;
        RBTNode<Key>* p = nullptr;

        if (z->left == nil) {
            x = z->right;
            p = z->parent;
            transplant(z, z->right);
        } else if (z->right == nil) {
            x = z->left;
            p = z->parent;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;

            if (y->parent == z) {
                p = y;
            } else {
                p = y->parent;
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }

        if (y_original_color == BLACK) {
            deleteFixup(x, p);
        }

        updateSubtreeSizesUp(p);
        delete z;
        return true;
    }

    RBTNode<Key>* search(const Key& key) const {
        RBTNode<Key>* curr = root;
        while (curr != nil) {
            if (curr->key == key) return curr;
            else if (key < curr->key) curr = curr->left;
            else curr = curr->right;
        }
        return nullptr;
    }

    RBTNode<Key>* search(const Key& key, int& nodesVisited, vector<Key>& path) const {
        RBTNode<Key>* curr = root;
        nodesVisited = 0;
        path.clear();
        while (curr != nil) {
            nodesVisited++;
            path.push_back(curr->key);
            if (curr->key == key) return curr;
            else if (key < curr->key) curr = curr->left;
            else curr = curr->right;
        }
        return nullptr;
    }

    vector<int> rangeQuery(const Key& low, const Key& high, int& nodesVisited) const {
        vector<int> result;
        rangeQueryHelper(root, low, high, result, nodesVisited);
        return result;
    }

    int countRange(const Key& low, const Key& high) const {
        if (low > high) return 0;
        int countHigh = countLessThanOrEqual(high);
        int countLow = countLessThan(low);
        return countHigh - countLow;
    }

    int rank(const Key& key) const {
        return countLessThan(key);
    }

    RBTNode<Key>* select(int k) const {
        if (k < 0 || k >= root->subtreeSize) return nullptr;

        RBTNode<Key>* curr = root;
        while (curr != nil) {
            int leftSize = curr->left->subtreeSize;
            int currSize = curr->rowIds.size();

            if (k < leftSize) {
                curr = curr->left;
            } else if (k < leftSize + currSize) {
                return curr;
            } else {
                k -= (leftSize + currSize);
                curr = curr->right;
            }
        }
        return nullptr;
    }

    RBTNode<Key>* select(int k, int& nodesVisited, vector<Key>& path) const {
        nodesVisited = 0;
        path.clear();
        if (k < 0 || k >= root->subtreeSize) return nullptr;

        RBTNode<Key>* curr = root;
        while (curr != nil) {
            nodesVisited++;
            path.push_back(curr->key);
            int leftSize = curr->left->subtreeSize;
            int currSize = curr->rowIds.size();

            if (k < leftSize) {
                curr = curr->left;
            } else if (k < leftSize + currSize) {
                return curr;
            } else {
                k -= (leftSize + currSize);
                curr = curr->right;
            }
        }
        return nullptr;
    }

    double median() const {
        int N = root->subtreeSize;
        if (N == 0) return 0.0;
        if (N % 2 != 0) {
            RBTNode<Key>* mNode = select(N / 2);
            return mNode ? mNode->key : 0.0;
        } else {
            RBTNode<Key>* m1 = select(N / 2 - 1);
            RBTNode<Key>* m2 = select(N / 2);
            double val1 = m1 ? m1->key : 0.0;
            double val2 = m2 ? m2->key : 0.0;
            return (val1 + val2) / 2.0;
        }
    }

    double percentile(double p) const {
        int N = root->subtreeSize;
        if (N == 0) return 0.0;
        if (p < 0.0) p = 0.0;
        if (p > 100.0) p = 100.0;

        int idx = round(p * (N - 1) / 100.0);
        RBTNode<Key>* node = select(idx);
        return node ? node->key : 0.0;
    }

    int size() const {
        return root->subtreeSize;
    }

    int nodeCount() const {
        return countNodes(root);
    }

    int countNodes(RBTNode<Key>* node) const {
        if (node == nil) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    }

    void print() const {
        if (root == nil) {
            cout << "[Árbol Vacío]" << endl;
        } else {
            printHelper(root, "", true);
        }
    }
};

#endif // REDBLACKTREE_H
