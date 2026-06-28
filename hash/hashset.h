#ifndef HASHSET_H
#define HASHSET_H

#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

class HashSet {
private:
    struct Node {
        int val;
        Node* next;
        Node(int v, Node* n = nullptr) : val(v), next(n) {}
    };

    vector<Node*> buckets;
    int numElements;

    int hash(int val) const {
        if (buckets.empty()) return 0;
        unsigned int h = val;
        return h % buckets.size();
    }

    void rehash() {
        int oldSize = buckets.size();
        vector<Node*> oldBuckets = buckets;

        buckets.assign(oldSize * 2, nullptr);
        numElements = 0;

        for (int i = 0; i < oldSize; ++i) {
            Node* curr = oldBuckets[i];
            while (curr != nullptr) {
                Node* nextNode = curr->next;
                insertInternal(curr->val);
                delete curr;
                curr = nextNode;
            }
        }
    }

    void insertInternal(int val) {
        int idx = hash(val);
        Node* curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->val == val) return;
            curr = curr->next;
        }
        buckets[idx] = new Node(val, buckets[idx]);
        numElements++;
    }

public:
    HashSet(int initialBuckets = 16) : buckets(initialBuckets, nullptr), numElements(0) {}

    ~HashSet() {
        clear();
    }

    HashSet(const HashSet& other) {
        buckets.assign(other.buckets.size(), nullptr);
        numElements = 0;
        for (size_t i = 0; i < other.buckets.size(); ++i) {
            Node* curr = other.buckets[i];
            while (curr != nullptr) {
                insertInternal(curr->val);
                curr = curr->next;
            }
        }
    }

    HashSet& operator=(const HashSet& other) {
        if (this != &other) {
            clear();
            buckets.assign(other.buckets.size(), nullptr);
            numElements = 0;
            for (size_t i = 0; i < other.buckets.size(); ++i) {
                Node* curr = other.buckets[i];
                while (curr != nullptr) {
                    insertInternal(curr->val);
                    curr = curr->next;
                }
            }
        }
        return *this;
    }

    void clear() {
        for (size_t i = 0; i < buckets.size(); ++i) {
            Node* curr = buckets[i];
            while (curr != nullptr) {
                Node* nextNode = curr->next;
                delete curr;
                curr = nextNode;
            }
            buckets[i] = nullptr;
        }
        numElements = 0;
    }

    void insert(int val) {
        if ((double)(numElements + 1) / buckets.size() > 0.75) {
            rehash();
        }
        insertInternal(val);
    }

    void remove(int val) {
        if (buckets.empty()) return;
        int idx = hash(val);
        Node* curr = buckets[idx];
        Node* prev = nullptr;
        while (curr != nullptr) {
            if (curr->val == val) {
                if (prev == nullptr) {
                    buckets[idx] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                numElements--;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    bool contains(int val) const {
        if (buckets.empty()) return false;
        int idx = hash(val);
        Node* curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->val == val) return true;
            curr = curr->next;
        }
        return false;
    }

    int size() const {
        return numElements;
    }

    vector<int> toVector() const {
        vector<int> res;
        res.reserve(numElements);
        for (size_t i = 0; i < buckets.size(); ++i) {
            Node* curr = buckets[i];
            while (curr != nullptr) {
                res.push_back(curr->val);
                curr = curr->next;
            }
        }
        return res;
    }
};

#endif // HASHSET_H
