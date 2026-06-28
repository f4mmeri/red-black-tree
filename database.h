#ifndef DATABASE_H
#define DATABASE_H

#include "hash/hashtable.h"
#include "tree/redblacktree.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

struct Movie {
    int movieId;
    string title;
    string genres;
    double averageRating;
};

class Database {
private:
    vector<Movie> movies;
    HashTable<int, size_t> movieIdToIndex;
    RedBlackTree<double> scoreIndex;

public:
    Database() {}

    bool insertMovie(const Movie& m) {
        if (movieIdToIndex.contains(m.movieId)) {
            return false;
        }
        movies.push_back(m);
        size_t idx = movies.size() - 1;
        movieIdToIndex.insert(m.movieId, idx);
        scoreIndex.insert(m.averageRating, m.movieId);
        return true;
    }

    bool deleteMovie(int id) {
        size_t* idxPtr = movieIdToIndex.get(id);
        if (idxPtr == nullptr) {
            return false;
        }
        size_t idx = *idxPtr;
        Movie& m = movies[idx];

        scoreIndex.remove(m.averageRating, id);
        movieIdToIndex.remove(id);

        if (idx < movies.size() - 1) {
            movies[idx] = movies.back();
            movieIdToIndex.insert(movies[idx].movieId, idx);
        }
        movies.pop_back();
        return true;
    }

    bool updateRating(int id, double newRating) {
        size_t* idxPtr = movieIdToIndex.get(id);
        if (idxPtr == nullptr) {
            return false;
        }
        size_t idx = *idxPtr;
        Movie& m = movies[idx];
        if (m.averageRating == newRating) {
            return true;
        }
        scoreIndex.remove(m.averageRating, id);
        m.averageRating = newRating;
        scoreIndex.insert(newRating, id);
        return true;
    }

    Movie* findById(int id) const {
        const size_t* idxPtr = movieIdToIndex.get(id);
        if (idxPtr == nullptr) {
            return nullptr;
        }
        return const_cast<Movie*>(&movies[*idxPtr]);
    }

    vector<Movie> findEqual(double rating) const {
        vector<Movie> res;
        RBTNode<double>* node = scoreIndex.search(rating);
        if (node != nullptr) {
            vector<int> ids = node->rowIds.toVector();
            res.reserve(ids.size());
            for (int id : ids) {
                Movie* m = findById(id);
                if (m != nullptr) {
                    res.push_back(*m);
                }
            }
        }
        return res;
    }

    vector<Movie> findEqual(double rating, int& nodesVisited, vector<double>& path) const {
        vector<Movie> res;
        RBTNode<double>* node = scoreIndex.search(rating, nodesVisited, path);
        if (node != nullptr) {
            vector<int> ids = node->rowIds.toVector();
            res.reserve(ids.size());
            for (int id : ids) {
                Movie* m = findById(id);
                if (m != nullptr) {
                    res.push_back(*m);
                }
            }
        }
        return res;
    }

    vector<Movie> findBetween(double low, double high, int& nodesVisited) const {
        vector<Movie> res;
        vector<int> ids = scoreIndex.rangeQuery(low, high, nodesVisited);
        res.reserve(ids.size());
        for (int id : ids) {
            Movie* m = findById(id);
            if (m != nullptr) {
                res.push_back(*m);
            }
        }
        return res;
    }

    int countBetween(double low, double high) const {
        return scoreIndex.countRange(low, high);
    }

    vector<Movie> topK(int k) const {
        vector<Movie> result;
        int N = scoreIndex.size();
        if (N == 0 || k <= 0) return result;
        if (k > N) k = N;

        int count = 0;
        int currentRank = N - 1;
        while (count < k && currentRank >= 0) {
            RBTNode<double>* node = scoreIndex.select(currentRank);
            if (node == nullptr) break;
            vector<int> ids = node->rowIds.toVector();
            for (int id : ids) {
                Movie* m = findById(id);
                if (m != nullptr) {
                    result.push_back(*m);
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
        return movies.size();
    }

    void printTreeIndex() const {
        scoreIndex.print();
    }
};

class NaiveDatabase {
private:
    vector<Movie> movies;

public:
    NaiveDatabase() {}

    bool insertMovie(const Movie& m) {
        for (const auto& movie : movies) {
            if (movie.movieId == m.movieId) return false;
        }
        movies.push_back(m);
        return true;
    }

    bool deleteMovie(int id) {
        for (auto it = movies.begin(); it != movies.end(); ++it) {
            if (it->movieId == id) {
                movies.erase(it);
                return true;
            }
        }
        return false;
    }

    bool updateRating(int id, double newRating) {
        for (auto& movie : movies) {
            if (movie.movieId == id) {
                movie.averageRating = newRating;
                return true;
            }
        }
        return false;
    }

    Movie* findById(int id) {
        for (auto& movie : movies) {
            if (movie.movieId == id) return &movie;
        }
        return nullptr;
    }

    vector<Movie> findEqual(double rating, int& comparisons) {
        vector<Movie> res;
        comparisons = 0;
        for (const auto& movie : movies) {
            comparisons++;
            if (movie.averageRating == rating) {
                res.push_back(movie);
            }
        }
        return res;
    }

    vector<Movie> findBetween(double low, double high, int& comparisons) {
        vector<Movie> res;
        comparisons = 0;
        for (const auto& movie : movies) {
            comparisons++;
            if (movie.averageRating >= low && movie.averageRating <= high) {
                res.push_back(movie);
            }
        }
        return res;
    }

    int countBetween(double low, double high, int& comparisons) {
        int count = 0;
        comparisons = 0;
        for (const auto& movie : movies) {
            comparisons++;
            if (movie.averageRating >= low && movie.averageRating <= high) {
                count++;
            }
        }
        return count;
    }

    double median() {
        if (movies.empty()) return 0.0;
        vector<double> ratings;
        ratings.reserve(movies.size());
        for (const auto& movie : movies) {
            ratings.push_back(movie.averageRating);
        }
        sort(ratings.begin(), ratings.end());
        int N = ratings.size();
        if (N % 2 != 0) {
            return ratings[N / 2];
        } else {
            return (ratings[N / 2 - 1] + ratings[N / 2]) / 2.0;
        }
    }

    double percentile(double p) {
        if (movies.empty()) return 0.0;
        vector<double> ratings;
        ratings.reserve(movies.size());
        for (const auto& movie : movies) {
            ratings.push_back(movie.averageRating);
        }
        sort(ratings.begin(), ratings.end());
        int N = ratings.size();
        int idx = round(p * (N - 1) / 100.0);
        return ratings[idx];
    }

    int size() const {
        return movies.size();
    }
};

#endif // DATABASE_H
