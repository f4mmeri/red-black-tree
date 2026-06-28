#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <vector>
#include <string>
#include <iostream>
using namespace std;

inline size_t get_hash(int key) {
    // La función get_hash(int) simplemente devuelve la clave convertida a size_t,
    // ya que el método de multiplicación de CLRS opera directamente sobre el valor entero.
    return static_cast<size_t>(key);
}

inline size_t get_hash(const string& key) {
    // Convierte la cadena en un entero utilizando la regla de Horner en base 128,
    // tal como sugiere CLRS para extender el método a claves no numéricas.
    size_t h = 0;
    for (char c : key) {
        h = h * 128 + static_cast<unsigned char>(c);
    }
    return h;
}

template <typename K>
struct CustomHash {
    size_t operator()(const K& key) const {
        return get_hash(key);
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

    vector<Node*> buckets;
    int numElements;
    CustomHash<K> hasher;

    int hash(const K& key) const {
        size_t m = buckets.size();
        if (m <= 1) return 0;
        
        // Calculamos p de forma portátil tal que m = 2^p
        int p = 0;
        while (m > 1) {
            m >>= 1;
            p++;
        }

        // Método de multiplicación de Cormen: h(k) = (k * s) >> (w - p)
        // Donde s = floor(A * 2^w). Para w = 64 y A = (sqrt(5) - 1) / 2:
        // s = 11400714819323198485ULL
        unsigned long long k = static_cast<unsigned long long>(hasher(key));
        unsigned long long s = 11400714819323198485ULL;
        return static_cast<int>((k * s) >> (64 - p));
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

    vector<V> toVector() const {
        vector<V> res;
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
