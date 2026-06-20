#ifndef DATABASE_H
#define DATABASE_H

#include "hash/hashtable.h"
#include "tree/redblacktree.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

struct Record {
    int id;
    std::string name;
    int age;
    double score;
    std::string category;
};

class Database {
private:
    HashTable<int, Record> recordsById;
    RedBlackTree<double> scoreIndex;

public:
    Database() {}

    bool insertRecord(const Record& r) {
        if (recordsById.contains(r.id)) {
            return false;
        }
        recordsById.insert(r.id, r);
        scoreIndex.insert(r.score, r.id);
        return true;
    }

    bool deleteRecord(int id) {
        Record* r = recordsById.get(id);
        if (r == nullptr) {
            return false;
        }
        scoreIndex.remove(r->score, id);
        recordsById.remove(id);
        return true;
    }

    bool updateIndexedField(int id, double newScore) {
        Record* r = recordsById.get(id);
        if (r == nullptr) {
            return false;
        }
        if (r->score == newScore) {
            return true;
        }
        scoreIndex.remove(r->score, id);
        r->score = newScore;
        scoreIndex.insert(newScore, id);
        return true;
    }

    Record* findById(int id) const {
        return recordsById.get(id);
    }

    std::vector<Record> findEqual(double score) const {
        std::vector<Record> res;
        RBTNode<double>* node = scoreIndex.search(score);
        if (node != nullptr) {
            std::vector<int> ids = node->rowIds.toVector();
            res.reserve(ids.size());
            for (int id : ids) {
                Record* r = recordsById.get(id);
                if (r != nullptr) {
                    res.push_back(*r);
                }
            }
        }
        return res;
    }

    std::vector<Record> findEqual(double score, int& nodesVisited, std::vector<double>& path) const {
        std::vector<Record> res;
        RBTNode<double>* node = scoreIndex.search(score, nodesVisited, path);
        if (node != nullptr) {
            std::vector<int> ids = node->rowIds.toVector();
            res.reserve(ids.size());
            for (int id : ids) {
                Record* r = recordsById.get(id);
                if (r != nullptr) {
                    res.push_back(*r);
                }
            }
        }
        return res;
    }

    std::vector<Record> findBetween(double low, double high, int& nodesVisited) const {
        std::vector<Record> res;
        std::vector<int> ids = scoreIndex.rangeQuery(low, high, nodesVisited);
        res.reserve(ids.size());
        for (int id : ids) {
            Record* r = recordsById.get(id);
            if (r != nullptr) {
                res.push_back(*r);
            }
        }
        return res;
    }

    int countBetween(double low, double high) const {
        return scoreIndex.countRange(low, high);
    }

    std::vector<Record> topK(int k) const {
        std::vector<Record> result;
        int N = scoreIndex.size();
        if (N == 0 || k <= 0) return result;
        if (k > N) k = N;

        int count = 0;
        int currentRank = N - 1;
        while (count < k && currentRank >= 0) {
            RBTNode<double>* node = scoreIndex.select(currentRank);
            if (node == nullptr) break;
            std::vector<int> ids = node->rowIds.toVector();
            for (int id : ids) {
                Record* r = recordsById.get(id);
                if (r != nullptr) {
                    result.push_back(*r);
                    count++;
                    if (count >= k) break;
                }
            }
            currentRank -= ids.size();
        }
        return result;
    }

    double median() const {
        return scoreIndex.median();
    }

    double percentile(double p) const {
        return scoreIndex.percentile(p);
    }

    int size() const {
        return recordsById.size();
    }

    void printTreeIndex() const {
        scoreIndex.print();
    }
};

class NaiveDatabase {
private:
    std::vector<Record> records;

public:
    NaiveDatabase() {}

    bool insertRecord(const Record& r) {
        for (const auto& rec : records) {
            if (rec.id == r.id) return false;
        }
        records.push_back(r);
        return true;
    }

    bool deleteRecord(int id) {
        for (auto it = records.begin(); it != records.end(); ++it) {
            if (it->id == id) {
                records.erase(it);
                return true;
            }
        }
        return false;
    }

    bool updateIndexedField(int id, double newScore) {
        for (auto& rec : records) {
            if (rec.id == id) {
                rec.score = newScore;
                return true;
            }
        }
        return false;
    }

    Record* findById(int id) {
        for (auto& rec : records) {
            if (rec.id == id) return &rec;
        }
        return nullptr;
    }

    std::vector<Record> findEqual(double score, int& comparisons) {
        std::vector<Record> res;
        comparisons = 0;
        for (const auto& rec : records) {
            comparisons++;
            if (rec.score == score) {
                res.push_back(rec);
            }
        }
        return res;
    }

    std::vector<Record> findBetween(double low, double high, int& comparisons) {
        std::vector<Record> res;
        comparisons = 0;
        for (const auto& rec : records) {
            comparisons++;
            if (rec.score >= low && rec.score <= high) {
                res.push_back(rec);
            }
        }
        return res;
    }

    int countBetween(double low, double high, int& comparisons) {
        int count = 0;
        comparisons = 0;
        for (const auto& rec : records) {
            comparisons++;
            if (rec.score >= low && rec.score <= high) {
                count++;
            }
        }
        return count;
    }

    double median() {
        if (records.empty()) return 0.0;
        std::vector<double> scores;
        scores.reserve(records.size());
        for (const auto& rec : records) {
            scores.push_back(rec.score);
        }
        std::sort(scores.begin(), scores.end());
        int N = scores.size();
        if (N % 2 != 0) {
            return scores[N / 2];
        } else {
            return (scores[N / 2 - 1] + scores[N / 2]) / 2.0;
        }
    }

    double percentile(double p) {
        if (records.empty()) return 0.0;
        std::vector<double> scores;
        scores.reserve(records.size());
        for (const auto& rec : records) {
            scores.push_back(rec.score);
        }
        std::sort(scores.begin(), scores.end());
        int N = scores.size();
        int idx = std::round(p * (N - 1) / 100.0);
        return scores[idx];
    }

    int size() const {
        return records.size();
    }
};

#endif // DATABASE_H
