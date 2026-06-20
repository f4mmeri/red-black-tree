#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <vector>
#include <stdexcept>
#include <functional>

template <typename K>
struct CustomHash {
    size_t operator()(const K& key) const {
        return std::hash<K>{}(key);
    }
};

template <typename K, typename V>
class HashTable {
private:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v, Node* n = nullptr) : key(k), value(v), next(n) {}
    };

    std::vector<Node*> buckets;
    int numElements;
    CustomHash<K> hasher;

    int hash(const K& key) const {
        if (buckets.empty()) return 0;
        return hasher(key) % buckets.size();
    }

    void rehash() {
        int oldSize = buckets.size();
        std::vector<Node*> oldBuckets = buckets;

        buckets.assign(oldSize * 2, nullptr);
        numElements = 0;

        for (int i = 0; i < oldSize; ++i) {
            Node* curr = oldBuckets[i];
            while (curr != nullptr) {
                Node* nextNode = curr->next;
                insertInternal(curr->key, curr->value);
                delete curr;
                curr = nextNode;
            }
        }
    }

    void insertInternal(const K& key, const V& value) {
        int idx = hash(key);
        Node* curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->key == key) {
                curr->value = value;
                return;
            }
            curr = curr->next;
        }
        buckets[idx] = new Node(key, value, buckets[idx]);
        numElements++;
    }

public:
    HashTable(int initialBuckets = 16) : buckets(initialBuckets, nullptr), numElements(0) {}

    ~HashTable() {
        clear();
    }

    HashTable(const HashTable& other) {
        buckets.assign(other.buckets.size(), nullptr);
        numElements = 0;
        for (size_t i = 0; i < other.buckets.size(); ++i) {
            Node* curr = other.buckets[i];
            while (curr != nullptr) {
                insertInternal(curr->key, curr->value);
                curr = curr->next;
            }
        }
    }

    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            clear();
            buckets.assign(other.buckets.size(), nullptr);
            numElements = 0;
            for (size_t i = 0; i < other.buckets.size(); ++i) {
                Node* curr = other.buckets[i];
                while (curr != nullptr) {
                    insertInternal(curr->key, curr->value);
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

    void insert(const K& key, const V& value) {
        if ((double)(numElements + 1) / buckets.size() > 0.75) {
            rehash();
        }
        insertInternal(key, value);
    }

    void remove(const K& key) {
        if (buckets.empty()) return;
        int idx = hash(key);
        Node* curr = buckets[idx];
        Node* prev = nullptr;
        while (curr != nullptr) {
            if (curr->key == key) {
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

    bool contains(const K& key) const {
        if (buckets.empty()) return false;
        int idx = hash(key);
        Node* curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->key == key) return true;
            curr = curr->next;
        }
        return false;
    }

    V* get(const K& key) const {
        if (buckets.empty()) return nullptr;
        int idx = hash(key);
        Node* curr = buckets[idx];
        while (curr != nullptr) {
            if (curr->key == key) return const_cast<V*>(&(curr->value));
            curr = curr->next;
        }
        return nullptr;
    }

    int size() const {
        return numElements;
    }

    std::vector<V> toVector() const {
        std::vector<V> res;
        res.reserve(numElements);
        for (size_t i = 0; i < buckets.size(); ++i) {
            Node* curr = buckets[i];
            while (curr != nullptr) {
                res.push_back(curr->value);
                curr = curr->next;
            }
        }
        return res;
    }
};

#endif // HASHTABLE_H
