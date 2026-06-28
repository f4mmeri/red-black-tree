#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cmath>
#include "database.h"

using namespace std;

// Colores ANSI para la consola
const string ANSI_RESET   = "\033[0m";
const string ANSI_RED     = "\033[31m";
const string ANSI_GREEN   = "\033[32m";
const string ANSI_YELLOW  = "\033[33m";
const string ANSI_BLUE    = "\033[34m";
const string ANSI_MAGENTA = "\033[35m";
const string ANSI_CYAN    = "\033[36m";
const string ANSI_BOLD    = "\033[1m";


// Función auxiliar para imprimir el encabezado de la tabla de películas
void printMovieHeader() {
    cout << ANSI_BOLD << ANSI_CYAN << left 
              << setw(10) << "MovieID" 
              << setw(45) << "Título" 
              << setw(30) << "Géneros" 
              << setw(15) << "Rating Prom." << ANSI_RESET << endl;
    cout << string(100, '-') << endl;
}

// Función auxiliar para imprimir una fila de película
void printMovieRow(const Movie& m) {
    string cleanTitle = m.title;
    if (cleanTitle.length() > 42) {
        cleanTitle = cleanTitle.substr(0, 39) + "...";
    }
    string cleanGenres = m.genres;
    if (cleanGenres.length() > 27) {
        cleanGenres = cleanGenres.substr(0, 24) + "...";
    }
    cout << left 
              << setw(10) << m.movieId 
              << setw(45) << cleanTitle 
              << setw(30) << cleanGenres 
              << setw(15) << fixed << setprecision(2) << m.averageRating << endl;
}

// Parseador de líneas CSV que respeta las comillas
vector<string> parseCSVLine(const string& line) {
    vector<string> result;
    string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    result.push_back(current);
    return result;
}

struct RatingSumCount {
    double sum = 0.0;
    int count = 0;
};

// Carga de base de datos desde CSV
bool loadDataset(Database& db, const string& archivePath, vector<Movie>& allMovies) {
    string ratingsPath = archivePath + "/ratings.csv";
    string moviesPath = archivePath + "/movies.csv";

    cout << ANSI_CYAN << "[.] Cargando calificaciones desde " << ratingsPath << "..." << ANSI_RESET << endl;
    ifstream ratingsFile(ratingsPath);
    if (!ratingsFile.is_open()) {
        cerr << ANSI_RED << "[!] Error: No se pudo abrir " << ratingsPath << ANSI_RESET << endl;
        return false;
    }

    HashTable<int, RatingSumCount> ratingStats;
    string line;
    getline(ratingsFile, line); // Cabecera
    int ratingCount = 0;
    while (getline(ratingsFile, line)) {
        if (line.empty()) continue;
        auto fields = parseCSVLine(line);
        if (fields.size() >= 3) {
            try {
                int movieId = stoi(fields[1]);
                double rating = stod(fields[2]);
                RatingSumCount* stats = ratingStats.get(movieId);
                if (stats == nullptr) {
                    RatingSumCount newStats;
                    newStats.sum = rating;
                    newStats.count = 1;
                    ratingStats.insert(movieId, newStats);
                } else {
                    stats->sum += rating;
                    stats->count += 1;
                }
                ratingCount++;
            } catch (...) {
                // Ignorar errores de casteo
            }
        }
    }
    ratingsFile.close();
    cout << ANSI_GREEN << "[✔] Calificaciones procesadas: " << ratingCount << ANSI_RESET << endl;

    cout << ANSI_CYAN << "[.] Cargando películas desde " << moviesPath << "..." << ANSI_RESET << endl;
    ifstream moviesFile(moviesPath);
    if (!moviesFile.is_open()) {
        cerr << ANSI_RED << "[!] Error: No se pudo abrir " << moviesPath << ANSI_RESET << endl;
        return false;
    }

    getline(moviesFile, line); // Cabecera
    int movieCount = 0;
    while (getline(moviesFile, line)) {
        if (line.empty()) continue;
        auto fields = parseCSVLine(line);
        if (fields.size() >= 3) {
            try {
                int movieId = stoi(fields[0]);
                string title = fields[1];
                string genres = fields[2];

                double avgRating = 0.0;
                RatingSumCount* stats = ratingStats.get(movieId);
                if (stats != nullptr && stats->count > 0) {
                    avgRating = stats->sum / stats->count;
                    // Redondear a 2 decimales
                    avgRating = round(avgRating * 100.0) / 100.0;
                }

                Movie m{movieId, title, genres, avgRating};
                db.insertMovie(m);
                allMovies.push_back(m);
                movieCount++;
            } catch (...) {
                // Ignorar errores de casteo
            }
        }
    }
    moviesFile.close();
    cout << ANSI_GREEN << "[✔] Películas cargadas e indexadas exitosamente: " << movieCount << ANSI_RESET << endl;
    return true;
}

// Ejecución de pruebas experimentales
void runBenchmark(const vector<Movie>& testData) {
    cout << ANSI_BOLD << ANSI_BLUE << "\n==============================================" << ANSI_RESET << endl;
    cout << ANSI_BOLD << ANSI_BLUE << "  INICIANDO EXPERIMENTO - N = " << testData.size() << " PELÍCULAS" << ANSI_RESET << endl;
    cout << ANSI_BOLD << ANSI_BLUE << "==============================================" << ANSI_RESET << endl;

    mt19937 rng(42); // Semilla fija para consistencia
    Database indexedDb;
    NaiveDatabase naiveDb;

    // --- 1. TIEMPO DE CONSTRUCCIÓN ---
    auto start = chrono::high_resolution_clock::now();
    for (const auto& m : testData) {
        indexedDb.insertMovie(m);
    }
    auto end = chrono::high_resolution_clock::now();
    double timeBuildIndexed = chrono::duration<double, milli>(end - start).count();

    start = chrono::high_resolution_clock::now();
    for (const auto& m : testData) {
        naiveDb.insertMovie(m);
    }
    end = chrono::high_resolution_clock::now();
    double timeBuildNaive = chrono::duration<double, milli>(end - start).count();

    cout << ANSI_GREEN << "[✔] Bases de datos construidas." << ANSI_RESET << endl;
    cout << "  - Tiempo construcción RBT/Hash Index: " << ANSI_BOLD << timeBuildIndexed << " ms" << ANSI_RESET << endl;
    cout << "  - Tiempo construcción Naive Vector  : " << ANSI_BOLD << timeBuildNaive << " ms" << ANSI_RESET << endl;

    // --- 2. BÚSQUEDA POR ID ---
    vector<int> searchIds;
    uniform_int_distribution<int> idIndexDist(0, testData.size() - 1);
    for (int i = 0; i < 1000; ++i) {
        searchIds.push_back(testData[idIndexDist(rng)].movieId);
    }

    start = chrono::high_resolution_clock::now();
    for (int id : searchIds) {
        indexedDb.findById(id);
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindIdIndexed = chrono::duration<double, nano>(end - start).count() / 1000.0;

    start = chrono::high_resolution_clock::now();
    for (int id : searchIds) {
        naiveDb.findById(id);
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindIdNaive = chrono::duration<double, nano>(end - start).count() / 1000.0;

    // --- 3. BÚSQUEDA POR RATING EXACTO ---
    vector<double> searchRatings;
    uniform_real_distribution<double> ratingDist(0.0, 5.0);
    for (int i = 0; i < 1000; ++i) {
        searchRatings.push_back(round(ratingDist(rng) * 10.0) / 10.0);
    }

    start = chrono::high_resolution_clock::now();
    int rbtSearchNodesVisitedTotal = 0;
    for (double rating : searchRatings) {
        int visited = 0;
        vector<double> path;
        indexedDb.findEqual(rating, visited, path);
        rbtSearchNodesVisitedTotal += visited;
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindEqualIndexed = chrono::duration<double, nano>(end - start).count() / 1000.0;
    double avgNodesVisitedFindEqual = (double)rbtSearchNodesVisitedTotal / 1000.0;

    start = chrono::high_resolution_clock::now();
    int naiveSearchComparisonsTotal = 0;
    for (double rating : searchRatings) {
        int comparisons = 0;
        naiveDb.findEqual(rating, comparisons);
        naiveSearchComparisonsTotal += comparisons;
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindEqualNaive = chrono::duration<double, nano>(end - start).count() / 1000.0;
    double avgComparisonsFindEqualNaive = (double)naiveSearchComparisonsTotal / 1000.0;

    // --- 4. BÚSQUEDA POR RANGO ---
    vector<pair<double, double>> ranges;
    for (int i = 0; i < 100; ++i) {
        double low = round(ratingDist(rng) * 10.0) / 10.0;
        double high = low + 1.0;
        ranges.push_back({low, high});
    }

    start = chrono::high_resolution_clock::now();
    int rbtRangeNodesVisited = 0;
    for (const auto& r : ranges) {
        int visited = 0;
        indexedDb.findBetween(r.first, r.second, visited);
        rbtRangeNodesVisited += visited;
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindBetweenIndexed = chrono::duration<double, milli>(end - start).count() / 100.0;
    double avgNodesVisitedRange = (double)rbtRangeNodesVisited / 100.0;

    start = chrono::high_resolution_clock::now();
    int naiveRangeComparisons = 0;
    for (const auto& r : ranges) {
        int comps = 0;
        naiveDb.findBetween(r.first, r.second, comps);
        naiveRangeComparisons += comps;
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeFindBetweenNaive = chrono::duration<double, milli>(end - start).count() / 100.0;
    double avgComparisonsRangeNaive = (double)naiveRangeComparisons / 100.0;

    // --- 5. CONTEO DE RANGO ---
    start = chrono::high_resolution_clock::now();
    for (const auto& r : ranges) {
        indexedDb.countBetween(r.first, r.second);
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeCountBetweenIndexed = chrono::duration<double, nano>(end - start).count() / 100.0;

    start = chrono::high_resolution_clock::now();
    for (const auto& r : ranges) {
        int comps = 0;
        naiveDb.countBetween(r.first, r.second, comps);
    }
    end = chrono::high_resolution_clock::now();
    double avgTimeCountBetweenNaive = chrono::duration<double, milli>(end - start).count() / 100.0;

    // --- 6. MEDIANA Y PERCENTIL ---
    start = chrono::high_resolution_clock::now();
    indexedDb.median();
    indexedDb.percentile(90.0);
    end = chrono::high_resolution_clock::now();
    double timeStatsIndexed = chrono::duration<double, nano>(end - start).count();

    start = chrono::high_resolution_clock::now();
    naiveDb.median();
    naiveDb.percentile(90.0);
    end = chrono::high_resolution_clock::now();
    double timeStatsNaive = chrono::duration<double, milli>(end - start).count();

    // --- RESULTADOS IMPRESOS ---
    cout << "\n" << ANSI_BOLD << "CUADRO COMPARATIVO DE MÉTRICAS (PELÍCULAS):" << ANSI_RESET << endl;
    cout << string(90, '=') << endl;
    cout << left 
              << setw(28) << "Operación" 
              << setw(28) << "Estructura Indexada" 
              << setw(28) << "Solución Ingenua (Linear)" 
              << "Aceleración" << endl;
    cout << string(90, '-') << endl;

    stringstream ss1, ss2;
    ss1 << fixed << setprecision(2) << avgTimeFindIdIndexed << " ns (Hash)";
    ss2 << fixed << setprecision(2) << avgTimeFindIdNaive << " ns (O(N))";
    cout << left << setw(28) << "Buscar por ID (Prom.)"
              << setw(28) << ss1.str()
              << setw(28) << ss2.str()
              << fixed << setprecision(1) << (avgTimeFindIdNaive / avgTimeFindIdIndexed) << "x" << endl;

    ss1.str(""); ss2.str("");
    ss1 << avgTimeFindEqualIndexed << " ns (" << avgNodesVisitedFindEqual << " nod.)";
    ss2 << avgTimeFindEqualNaive << " ns (" << avgComparisonsFindEqualNaive << " comp.)";
    cout << left << setw(28) << "Buscar Rating Exacto"
              << setw(28) << ss1.str()
              << setw(28) << ss2.str()
              << fixed << setprecision(1) << (avgTimeFindEqualNaive / avgTimeFindEqualIndexed) << "x" << endl;

    ss1.str(""); ss2.str("");
    ss1 << avgTimeFindBetweenIndexed << " ms (" << avgNodesVisitedRange << " nod.)";
    ss2 << avgTimeFindBetweenNaive << " ms (" << avgComparisonsRangeNaive << " comp.)";
    cout << left << setw(28) << "Rango [Rating, Rating+1]"
              << setw(28) << ss1.str()
              << setw(28) << ss2.str()
              << fixed << setprecision(1) << (avgTimeFindBetweenNaive / avgTimeFindBetweenIndexed) << "x" << endl;

    ss1.str(""); ss2.str("");
    ss1 << avgTimeCountBetweenIndexed << " ns (O(log u))";
    ss2 << avgTimeCountBetweenNaive << " ms (O(N))";
    cout << left << setw(28) << "Contar Rango (CountRange)"
              << setw(28) << ss1.str()
              << setw(28) << ss2.str()
              << fixed << setprecision(1) << ((avgTimeCountBetweenNaive * 1000000.0) / avgTimeCountBetweenIndexed) << "x" << endl;

    ss1.str(""); ss2.str("");
    ss1 << timeStatsIndexed << " ns (O(log u))";
    ss2 << timeStatsNaive << " ms (O(N log N))";
    cout << left << setw(28) << "Mediana & P90"
              << setw(28) << ss1.str()
              << setw(28) << ss2.str()
              << fixed << setprecision(1) << ((timeStatsNaive * 1000000.0) / timeStatsIndexed) << "x" << endl;

    cout << string(90, '=') << endl;
}

int main() {
    Database db;
    vector<Movie> allMovies;

    cout << ANSI_BOLD << ANSI_GREEN << endl;
    cout << "  ╔═══════════════════════════════════════════════════════╗" << endl;
    cout << "  ║       SISTEMA DE INDEXACIÓN DE CATÁLOGO DE PELÍCULAS   ║" << endl;
    cout << "  ║       ÍNDICES: HashTable (ID) + RBT Aumentado (Rating) ║" << endl;
    cout << "  ╚═══════════════════════════════════════════════════════╝" << ANSI_RESET << endl;

    // Intentar cargar la base de datos desde la ruta especificada
    string archivePath = "archive";
    if (!loadDataset(db, archivePath, allMovies)) {
        cout << ANSI_YELLOW << "[!] Advertencia: No se pudieron cargar los datos reales. Iniciando con base de datos vacía." << ANSI_RESET << endl;
    }

    int choice = 0;
    while (true) {
        cout << "\n" << ANSI_BOLD << ANSI_CYAN << "--- MENÚ PRINCIPAL ---" << ANSI_RESET << endl;
        cout << "1.  Insertar película manualmente" << endl;
        cout << "2.  Eliminar película por MovieID" << endl;
        cout << "3.  Actualizar rating de una película (modifica RBT)" << endl;
        cout << "4.  Buscar película por MovieID (O(1))" << endl;
        cout << "5.  Buscar películas por Rating Exacto (Evidencia de camino RBT)" << endl;
        cout << "6.  Buscar películas en rango de Ratings (Evidencia de recorrido RBT)" << endl;
        cout << "7.  Contar películas en rango de Ratings (O(log N))" << endl;
        cout << "8.  Mostrar Top K películas con mayor rating" << endl;
        cout << "9.  Calcular estadísticas de orden (Mediana y Percentiles en O(log N))" << endl;
        cout << "10. Correr benchmark con datos reales (CSV)" << endl;
        cout << "11. Imprimir la estructura visual del Red-Black Tree" << endl;
        cout << "12. Salir del programa" << endl;
        cout << ANSI_BOLD << "Seleccione una opción: " << ANSI_RESET;
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << ANSI_RED << "[!] Entrada inválida. Ingrese un número." << ANSI_RESET << endl;
            continue;
        }

        if (choice == 12) {
            cout << ANSI_GREEN << "\n¡Gracias por usar el sistema! Saliendo..." << ANSI_RESET << endl;
            break;
        }

        switch (choice) {
            case 1: {
                Movie m;
                cout << "Ingrese MovieID (entero único): ";
                cin >> m.movieId;
                cout << "Ingrese Título: ";
                cin.ignore();
                getline(cin, m.title);
                cout << "Ingrese Géneros (separados por |): ";
                getline(cin, m.genres);
                cout << "Ingrese Rating Promedio (double, ej. 4.25): ";
                cin >> m.averageRating;

                if (db.insertMovie(m)) {
                    cout << ANSI_GREEN << "[✔] Película insertada exitosamente." << ANSI_RESET << endl;
                } else {
                    cout << ANSI_RED << "[!] Error: El ID " << m.movieId << " ya existe en la base de datos." << ANSI_RESET << endl;
                }
                break;
            }
            case 2: {
                int id;
                cout << "Ingrese el MovieID de la película a eliminar: ";
                cin >> id;
                if (db.deleteMovie(id)) {
                    cout << ANSI_GREEN << "[✔] Película eliminada correctamente de ambos índices." << ANSI_RESET << endl;
                } else {
                    cout << ANSI_RED << "[!] Película con ID " << id << " no encontrada." << ANSI_RESET << endl;
                }
                break;
            }
            case 3: {
                int id;
                double rating;
                cout << "Ingrese el MovieID de la película a actualizar: ";
                cin >> id;
                cout << "Ingrese el nuevo Rating Promedio: ";
                cin >> rating;

                if (db.updateRating(id, rating)) {
                    cout << ANSI_GREEN << "[✔] Rating actualizado y árbol RBT balanceado." << ANSI_RESET << endl;
                } else {
                    cout << ANSI_RED << "[!] Película con ID " << id << " no encontrada." << ANSI_RESET << endl;
                }
                break;
            }
            case 4: {
                int id;
                cout << "Ingrese el MovieID a buscar: ";
                cin >> id;

                auto start = chrono::high_resolution_clock::now();
                Movie* m = db.findById(id);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, nano>(end - start).count();

                if (m != nullptr) {
                    cout << ANSI_GREEN << "\n[✔] Película encontrada en HashTable (O(1) Promedio, Tiempo: " << t << " ns):" << ANSI_RESET << endl;
                    printMovieHeader();
                    printMovieRow(*m);
                } else {
                    cout << ANSI_RED << "[!] Película con ID " << id << " no encontrada." << ANSI_RESET << endl;
                }
                break;
            }
            case 5: {
                double rating;
                cout << "Ingrese el Rating exacto a buscar: ";
                cin >> rating;

                int nodesVisited = 0;
                vector<double> path;
                auto start = chrono::high_resolution_clock::now();
                vector<Movie> movies = db.findEqual(rating, nodesVisited, path);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, nano>(end - start).count();

                cout << ANSI_BOLD << "\nEvidencia del Recorrido RBT:" << ANSI_RESET << endl;
                cout << "  - Nodos visitados: " << nodesVisited << endl;
                cout << "  - Ruta de búsqueda (claves): ";
                for (size_t i = 0; i < path.size(); ++i) {
                    cout << path[i] << (i + 1 < path.size() ? " ➔ " : "");
                }
                cout << endl;

                if (!movies.empty()) {
                    cout << ANSI_GREEN << "\n[✔] Se encontraron " << movies.size() << " película(s) en " << t << " ns:" << ANSI_RESET << endl;
                    printMovieHeader();
                    for (const auto& m : movies) {
                        printMovieRow(m);
                    }
                } else {
                    cout << ANSI_YELLOW << "\n[!] No se encontraron películas con el rating " << rating << "." << ANSI_RESET << endl;
                }
                break;
            }
            case 6: {
                double low, high;
                cout << "Ingrese el límite inferior de Rating: ";
                cin >> low;
                cout << "Ingrese el límite superior de Rating: ";
                cin >> high;

                int nodesVisited = 0;
                auto start = chrono::high_resolution_clock::now();
                vector<Movie> movies = db.findBetween(low, high, nodesVisited);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, milli>(end - start).count();

                cout << ANSI_BOLD << "\nEvidencia del Recorrido de Rango:" << ANSI_RESET << endl;
                cout << "  - Nodos visitados (llamadas recursivas/ramas exploradas): " << nodesVisited << endl;

                if (!movies.empty()) {
                    cout << ANSI_GREEN << "\n[✔] Se encontraron " << movies.size() << " película(s) en " << t << " ms:" << ANSI_RESET << endl;
                    printMovieHeader();
                    for (const auto& m : movies) {
                        printMovieRow(m);
                    }
                } else {
                    cout << ANSI_YELLOW << "\n[!] No se encontraron películas en el rango [" << low << ", " << high << "]." << ANSI_RESET << endl;
                }
                break;
            }
            case 7: {
                double low, high;
                cout << "Ingrese el límite inferior: ";
                cin >> low;
                cout << "Ingrese el límite superior: ";
                cin >> high;

                auto start = chrono::high_resolution_clock::now();
                int count = db.countBetween(low, high);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, nano>(end - start).count();

                cout << ANSI_GREEN << "\n[✔] Cantidad de películas en rango: " << count << ANSI_RESET << endl;
                cout << "  - Complejidad: " << ANSI_BOLD << "O(log u)" << ANSI_RESET << " (usando subtreeSize aumentado)" << endl;
                cout << "  - Tiempo de cálculo: " << ANSI_BOLD << t << " ns" << ANSI_RESET << endl;
                break;
            }
            case 8: {
                int k;
                cout << "Ingrese la cantidad k de películas con mejor rating a mostrar: ";
                cin >> k;

                auto start = chrono::high_resolution_clock::now();
                vector<Movie> movies = db.topK(k);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, nano>(end - start).count();

                if (!movies.empty()) {
                    cout << ANSI_GREEN << "\n[✔] Top " << movies.size() << " películas con mayor rating (Tiempo: " << t << " ns):" << ANSI_RESET << endl;
                    printMovieHeader();
                    for (const auto& m : movies) {
                        printMovieRow(m);
                    }
                } else {
                    cout << ANSI_YELLOW << "[!] No hay películas en el sistema." << ANSI_RESET << endl;
                }
                break;
            }
            case 9: {
                auto start = chrono::high_resolution_clock::now();
                double med = db.median();
                double p25 = db.percentile(25.0);
                double p75 = db.percentile(75.0);
                double p90 = db.percentile(90.0);
                auto end = chrono::high_resolution_clock::now();
                double t = chrono::duration<double, nano>(end - start).count();

                cout << ANSI_GREEN << "\n[✔] Estadísticas calculadas en O(log u) (Tiempo: " << t << " ns):" << ANSI_RESET << endl;
                cout << "  - Total de películas indexadas: " << db.size() << endl;
                cout << "  - Mediana (Percentil 50): " << ANSI_BOLD << med << ANSI_RESET << endl;
                cout << "  - Percentil 25:           " << p25 << endl;
                cout << "  - Percentil 75:           " << p75 << endl;
                cout << "  - Percentil 90:           " << p90 << endl;
                break;
            }
            case 10: {
                if (allMovies.empty()) {
                    cout << ANSI_RED << "[!] No hay datos cargados en memoria para realizar el experimento." << ANSI_RESET << endl;
                } else {
                    runBenchmark(allMovies);
                }
                break;
            }
            case 11: {
                cout << ANSI_BOLD << ANSI_YELLOW << "\nEstructura Visual del Red-Black Tree Aumentado:" << ANSI_RESET << endl;
                cout << "Nomenclatura: [Clave] (Color, sz = SubtreeSize Acumulado, ids = Cantidad de IDs en HashSet)" << endl;
                cout << string(80, '-') << endl;
                db.printTreeIndex();
                break;
            }
            default: {
                cout << ANSI_RED << "[!] Opción no reconocida." << ANSI_RESET << endl;
                break;
            }
        }
    }

    return 0;
}
