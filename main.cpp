#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include "database.h"

// Colores ANSI para la consola
const std::string ANSI_RESET   = "\033[0m";
const std::string ANSI_RED     = "\033[31m";
const std::string ANSI_GREEN   = "\033[32m";
const std::string ANSI_YELLOW  = "\033[33m";
const std::string ANSI_BLUE    = "\033[34m";
const std::string ANSI_MAGENTA = "\033[35m";
const std::string ANSI_CYAN    = "\033[36m";
const std::string ANSI_BOLD    = "\033[1m";

// Datos estáticos para generación semi-real
const std::vector<std::string> FIRST_NAMES = {
    "Alvaro", "Beatriz", "Carlos", "Diana", "Eduardo", "Flavia", "Gustavo", "Helena",
    "Ignacio", "Julia", "Kevin", "Laura", "Mateo", "Natalia", "Oscar", "Patricia",
    "Quique", "Rosa", "Santiago", "Teresa", "Ulises", "Valeria", "Walter", "Ximena"
};

const std::vector<std::string> LAST_NAMES = {
    "Alvarez", "Bermudez", "Castro", "Diaz", "Espinosa", "Flores", "Gomez", "Huaman",
    "Ibanez", "Jimenez", "Kishimoto", "Lopez", "Medina", "Nunez", "Ortega", "Perez",
    "Quispe", "Ramos", "Sanchez", "Torres", "Ugarte", "Vargas", "Wong", "Zarate"
};

const std::vector<std::string> CATEGORIES = {
    "Premium", "Regular", "VIP", "Estudiante", "Docente", "Invitado", "Corporativo"
};

// Generador de registros aleatorios
Record generateRandomRecord(int id, std::mt19937& rng) {
    std::uniform_int_distribution<int> firstNameDist(0, FIRST_NAMES.size() - 1);
    std::uniform_int_distribution<int> lastNameDist(0, LAST_NAMES.size() - 1);
    std::uniform_int_distribution<int> ageDist(18, 90);
    std::uniform_real_distribution<double> scoreDist(0.0, 100.0);
    std::uniform_int_distribution<int> catDist(0, CATEGORIES.size() - 1);

    std::string name = FIRST_NAMES[firstNameDist(rng)] + " " + LAST_NAMES[lastNameDist(rng)];
    int age = ageDist(rng);
    // Redondear a 1 decimal para provocar duplicidad de claves score (clave del RBT)
    double score = std::round(scoreDist(rng) * 10.0) / 10.0;
    std::string category = CATEGORIES[catDist(rng)];

    return Record{id, name, age, score, category};
}

// Función auxiliar para imprimir registros formateados en una tabla
void printRecordHeader() {
    std::cout << ANSI_BOLD << ANSI_CYAN << std::left 
              << std::setw(10) << "ID" 
              << std::setw(22) << "Nombre" 
              << std::setw(8) << "Edad" 
              << std::setw(10) << "Score" 
              << std::setw(15) << "Categoría" << ANSI_RESET << std::endl;
    std::cout << std::string(65, '-') << std::endl;
}

void printRecordRow(const Record& r) {
    std::cout << std::left 
              << std::setw(10) << r.id 
              << std::setw(22) << r.name 
              << std::setw(8) << r.age 
              << std::setw(10) << std::fixed << std::setprecision(1) << r.score 
              << std::setw(15) << r.category << std::endl;
}

// Ejecución de pruebas experimentales
void runBenchmark(int size) {
    std::cout << ANSI_BOLD << ANSI_BLUE << "\n==============================================" << ANSI_RESET << std::endl;
    std::cout << ANSI_BOLD << ANSI_BLUE << "  INICIANDO EXPERIMENTO - N = " << size << " REGISTROS" << ANSI_RESET << std::endl;
    std::cout << ANSI_BOLD << ANSI_BLUE << "==============================================" << ANSI_RESET << std::endl;

    std::mt19937 rng(42); // Semilla fija para consistencia
    std::vector<Record> testData;
    testData.reserve(size);
    for (int i = 1; i <= size; ++i) {
        testData.push_back(generateRandomRecord(i, rng));
    }

    Database indexedDb;
    NaiveDatabase naiveDb;

    // --- 1. TIEMPO DE CONSTRUCCIÓN ---
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& r : testData) {
        indexedDb.insertRecord(r);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double timeBuildIndexed = std::chrono::duration<double, std::milli>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    for (const auto& r : testData) {
        naiveDb.insertRecord(r);
    }
    end = std::chrono::high_resolution_clock::now();
    double timeBuildNaive = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << ANSI_GREEN << "[✔] Bases de datos construidas." << ANSI_RESET << std::endl;
    std::cout << "  - Tiempo construcción RBT/Hash Index: " << ANSI_BOLD << timeBuildIndexed << " ms" << ANSI_RESET << std::endl;
    std::cout << "  - Tiempo construcción Naive Vector  : " << ANSI_BOLD << timeBuildNaive << " ms" << ANSI_RESET << std::endl;

    // --- 2. BÚSQUEDA POR ID ---
    // Hacemos 1,000 búsquedas aleatorias de IDs
    std::vector<int> searchIds;
    std::uniform_int_distribution<int> idDist(1, size);
    for (int i = 0; i < 1000; ++i) {
        searchIds.push_back(idDist(rng));
    }

    start = std::chrono::high_resolution_clock::now();
    for (int id : searchIds) {
        indexedDb.findById(id);
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindIdIndexed = std::chrono::duration<double, std::nano>(end - start).count() / 1000.0;

    start = std::chrono::high_resolution_clock::now();
    for (int id : searchIds) {
        naiveDb.findById(id);
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindIdNaive = std::chrono::duration<double, std::nano>(end - start).count() / 1000.0;

    // --- 3. BÚSQUEDA POR SCORE EXACTO ---
    std::vector<double> searchScores;
    std::uniform_real_distribution<double> scoreDist(0.0, 100.0);
    for (int i = 0; i < 1000; ++i) {
        searchScores.push_back(std::round(scoreDist(rng) * 10.0) / 10.0);
    }

    start = std::chrono::high_resolution_clock::now();
    int rbtSearchNodesVisitedTotal = 0;
    for (double score : searchScores) {
        int visited = 0;
        std::vector<double> path;
        indexedDb.findEqual(score, visited, path);
        rbtSearchNodesVisitedTotal += visited;
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindEqualIndexed = std::chrono::duration<double, std::nano>(end - start).count() / 1000.0;
    double avgNodesVisitedFindEqual = (double)rbtSearchNodesVisitedTotal / 1000.0;

    start = std::chrono::high_resolution_clock::now();
    int naiveSearchComparisonsTotal = 0;
    for (double score : searchScores) {
        int comparisons = 0;
        naiveDb.findEqual(score, comparisons);
        naiveSearchComparisonsTotal += comparisons;
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindEqualNaive = std::chrono::duration<double, std::nano>(end - start).count() / 1000.0;
    double avgComparisonsFindEqualNaive = (double)naiveSearchComparisonsTotal / 1000.0;

    // --- 4. BÚSQUEDA POR RANGO ---
    // Generar 100 rangos aleatorios de amplitud 10.0
    std::vector<std::pair<double, double>> ranges;
    for (int i = 0; i < 100; ++i) {
        double low = std::round(scoreDist(rng) * 10.0) / 10.0;
        double high = low + 10.0;
        ranges.push_back({low, high});
    }

    start = std::chrono::high_resolution_clock::now();
    int rbtRangeNodesVisited = 0;
    for (const auto& r : ranges) {
        int visited = 0;
        indexedDb.findBetween(r.first, r.second, visited);
        rbtRangeNodesVisited += visited;
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindBetweenIndexed = std::chrono::duration<double, std::milli>(end - start).count() / 100.0;
    double avgNodesVisitedRange = (double)rbtRangeNodesVisited / 100.0;

    start = std::chrono::high_resolution_clock::now();
    int naiveRangeComparisons = 0;
    for (const auto& r : ranges) {
        int comps = 0;
        naiveDb.findBetween(r.first, r.second, comps);
        naiveRangeComparisons += comps;
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeFindBetweenNaive = std::chrono::duration<double, std::milli>(end - start).count() / 100.0;
    double avgComparisonsRangeNaive = (double)naiveRangeComparisons / 100.0;

    // --- 5. CONTEO DE RANGO (AUMENTADO vs NAIVE) ---
    start = std::chrono::high_resolution_clock::now();
    for (const auto& r : ranges) {
        indexedDb.countBetween(r.first, r.second);
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeCountBetweenIndexed = std::chrono::duration<double, std::nano>(end - start).count() / 100.0;

    start = std::chrono::high_resolution_clock::now();
    for (const auto& r : ranges) {
        int comps = 0;
        naiveDb.countBetween(r.first, r.second, comps);
    }
    end = std::chrono::high_resolution_clock::now();
    double avgTimeCountBetweenNaive = std::chrono::duration<double, std::milli>(end - start).count() / 100.0;

    // --- 6. MEDIANA Y PERCENTIL ---
    start = std::chrono::high_resolution_clock::now();
    indexedDb.median();
    indexedDb.percentile(90.0);
    end = std::chrono::high_resolution_clock::now();
    double timeStatsIndexed = std::chrono::duration<double, std::nano>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    naiveDb.median();
    naiveDb.percentile(90.0);
    end = std::chrono::high_resolution_clock::now();
    double timeStatsNaive = std::chrono::duration<double, std::milli>(end - start).count();

    // --- RESULTADOS IMPRESOS ---
    std::cout << "\n" << ANSI_BOLD << "CUADRO COMPARATIVO DE MÉTRICAS:" << ANSI_RESET << std::endl;
    std::cout << std::string(90, '=') << std::endl;
    std::cout << std::left 
              << std::setw(28) << "Operación" 
              << std::setw(28) << "Estructura Indexada" 
              << std::setw(28) << "Solución Ingenua (Linear)" 
              << "Aceleración" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    // Búsqueda por ID
    std::stringstream ss1, ss2;
    ss1 << std::fixed << std::setprecision(2) << avgTimeFindIdIndexed << " ns (Hash)";
    ss2 << std::fixed << std::setprecision(2) << avgTimeFindIdNaive << " ns (O(N))";
    std::cout << std::left << std::setw(28) << "Buscar por ID (Prom.)"
              << std::setw(28) << ss1.str()
              << std::setw(28) << ss2.str()
              << std::fixed << std::setprecision(1) << (avgTimeFindIdNaive / avgTimeFindIdIndexed) << "x" << std::endl;

    // Búsqueda exactitud Score
    ss1.str(""); ss2.str("");
    ss1 << avgTimeFindEqualIndexed << " ns (" << avgNodesVisitedFindEqual << " nod.)";
    ss2 << avgTimeFindEqualNaive << " ns (" << avgComparisonsFindEqualNaive << " comp.)";
    std::cout << std::left << std::setw(28) << "Buscar Score Exacto"
              << std::setw(28) << ss1.str()
              << std::setw(28) << ss2.str()
              << std::fixed << std::setprecision(1) << (avgTimeFindEqualNaive / avgTimeFindEqualIndexed) << "x" << std::endl;

    // Búsqueda por rango
    ss1.str(""); ss2.str("");
    ss1 << avgTimeFindBetweenIndexed << " ms (" << avgNodesVisitedRange << " nod.)";
    ss2 << avgTimeFindBetweenNaive << " ms (" << avgComparisonsRangeNaive << " comp.)";
    std::cout << std::left << std::setw(28) << "Rango [Score, Score+10]"
              << std::setw(28) << ss1.str()
              << std::setw(28) << ss2.str()
              << std::fixed << std::setprecision(1) << (avgTimeFindBetweenNaive / avgTimeFindBetweenIndexed) << "x" << std::endl;

    // Conteo por Rango (Optimización Crítica de Aumento)
    ss1.str(""); ss2.str("");
    ss1 << avgTimeCountBetweenIndexed << " ns (O(log u))";
    ss2 << avgTimeCountBetweenNaive << " ms (O(N))";
    std::cout << std::left << std::setw(28) << "Contar Rango (CountRange)"
              << std::setw(28) << ss1.str()
              << std::setw(28) << ss2.str()
              << std::fixed << std::setprecision(1) << ((avgTimeCountBetweenNaive * 1000000.0) / avgTimeCountBetweenIndexed) << "x" << std::endl;

    // Estadísticas de orden
    ss1.str(""); ss2.str("");
    ss1 << timeStatsIndexed << " ns (O(log u))";
    ss2 << timeStatsNaive << " ms (O(N log N))";
    std::cout << std::left << std::setw(28) << "Mediana & P90"
              << std::setw(28) << ss1.str()
              << std::setw(28) << ss2.str()
              << std::fixed << std::setprecision(1) << ((timeStatsNaive * 1000000.0) / timeStatsIndexed) << "x" << std::endl;

    std::cout << std::string(90, '=') << std::endl;
    std::cout << "Nota: Los tiempos en 'ns' corresponden a nanosegundos (1 ms = 1,000,000 ns)." << std::endl;
}

int main() {
    Database db;
    std::mt19937 rng(1337);

    // Inicializar la base de datos interactiva con un pequeño lote de 15 registros
    for (int i = 1; i <= 15; ++i) {
        db.insertRecord(generateRandomRecord(i, rng));
    }

    std::cout << ANSI_BOLD << ANSI_GREEN << std::endl;
    std::cout << "  ╔═══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "  ║     MINI BASE DE DATOS EN MEMORIA DE ALTO RENDIMIENTO ║" << std::endl;
    std::cout << "  ║         ÍNDICES: HashTable (ID) + RBT Aumentado       ║" << std::endl;
    std::cout << "  ╚═══════════════════════════════════════════════════════╝" << ANSI_RESET << std::endl;

    int choice = 0;
    while (true) {
        std::cout << "\n" << ANSI_BOLD << ANSI_CYAN << "--- MENÚ PRINCIPAL ---" << ANSI_RESET << std::endl;
        std::cout << "1.  Insertar registro manualmente" << std::endl;
        std::cout << "2.  Eliminar registro por ID" << std::endl;
        std::cout << "3.  Actualizar score de un registro (modifica RBT)" << std::endl;
        std::cout << "4.  Buscar registro por ID (O(1))" << std::endl;
        std::cout << "5.  Buscar registros por Score Exacto (Evidencia de camino RBT)" << std::endl;
        std::cout << "6.  Buscar registros en rango de Scores (Evidencia de recorrido RBT)" << std::endl;
        std::cout << "7.  Contar registros en rango de Scores (O(log N))" << std::endl;
        std::cout << "8.  Mostrar Top K registros con mayor score" << std::endl;
        std::cout << "9.  Calcular estadísticas de orden (Mediana y Percentiles en O(log N))" << std::endl;
        std::cout << "10. Correr suite de experimentos (10k, 100k, 500k registros)" << std::endl;
        std::cout << "11. Imprimir la estructura visual del Red-Black Tree" << std::endl;
        std::cout << "12. Salir del programa" << std::endl;
        std::cout << ANSI_BOLD << "Seleccione una opción: " << ANSI_RESET;
        
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << ANSI_RED << "[!] Entrada inválida. Ingrese un número." << ANSI_RESET << std::endl;
            continue;
        }

        if (choice == 12) {
            std::cout << ANSI_GREEN << "\n¡Gracias por usar el sistema! Saliendo..." << ANSI_RESET << std::endl;
            break;
        }

        switch (choice) {
            case 1: {
                Record r;
                std::cout << "Ingrese ID (entero único): ";
                std::cin >> r.id;
                std::cout << "Ingrese Nombre completo: ";
                std::cin.ignore();
                std::getline(std::cin, r.name);
                std::cout << "Ingrese Edad (entero): ";
                std::cin >> r.age;
                std::cout << "Ingrese Score (double, ej. 85.5): ";
                std::cin >> r.score;
                std::cout << "Ingrese Categoría: ";
                std::cin.ignore();
                std::getline(std::cin, r.category);

                if (db.insertRecord(r)) {
                    std::cout << ANSI_GREEN << "[✔] Registro insertado exitosamente." << ANSI_RESET << std::endl;
                } else {
                    std::cout << ANSI_RED << "[!] Error: El ID " << r.id << " ya existe en la base de datos." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 2: {
                int id;
                std::cout << "Ingrese el ID del registro a eliminar: ";
                std::cin >> id;
                if (db.deleteRecord(id)) {
                    std::cout << ANSI_GREEN << "[✔] Registro eliminado correctamente de ambos índices." << ANSI_RESET << std::endl;
                } else {
                    std::cout << ANSI_RED << "[!] Registro con ID " << id << " no encontrado." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 3: {
                int id;
                double score;
                std::cout << "Ingrese el ID del registro a actualizar: ";
                std::cin >> id;
                std::cout << "Ingrese el nuevo Score: ";
                std::cin >> score;

                if (db.updateIndexedField(id, score)) {
                    std::cout << ANSI_GREEN << "[✔] Score actualizado y árbol RBT balanceado." << ANSI_RESET << std::endl;
                } else {
                    std::cout << ANSI_RED << "[!] Registro con ID " << id << " no encontrado." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 4: {
                int id;
                std::cout << "Ingrese el ID a buscar: ";
                std::cin >> id;

                auto start = std::chrono::high_resolution_clock::now();
                Record* r = db.findById(id);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::nano>(end - start).count();

                if (r != nullptr) {
                    std::cout << ANSI_GREEN << "\n[✔] Registro encontrado en HashTable (O(1) Promedio, Tiempo: " << t << " ns):" << ANSI_RESET << std::endl;
                    printRecordHeader();
                    printRecordRow(*r);
                } else {
                    std::cout << ANSI_RED << "[!] Registro con ID " << id << " no encontrado." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 5: {
                double score;
                std::cout << "Ingrese el Score exacto a buscar: ";
                std::cin >> score;

                int nodesVisited = 0;
                std::vector<double> path;
                auto start = std::chrono::high_resolution_clock::now();
                std::vector<Record> records = db.findEqual(score, nodesVisited, path);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::nano>(end - start).count();

                std::cout << ANSI_BOLD << "\nEvidencia del Recorrido RBT:" << ANSI_RESET << std::endl;
                std::cout << "  - Nodos visitados: " << nodesVisited << std::endl;
                std::cout << "  - Ruta de búsqueda (claves): ";
                for (size_t i = 0; i < path.size(); ++i) {
                    std::cout << path[i] << (i + 1 < path.size() ? " ➔ " : "");
                }
                std::cout << std::endl;

                if (!records.empty()) {
                    std::cout << ANSI_GREEN << "\n[✔] Se encontraron " << records.size() << " registro(s) en " << t << " ns:" << ANSI_RESET << std::endl;
                    printRecordHeader();
                    for (const auto& r : records) {
                        printRecordRow(r);
                    }
                } else {
                    std::cout << ANSI_YELLOW << "\n[!] No se encontraron registros con el score " << score << "." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 6: {
                double low, high;
                std::cout << "Ingrese el límite inferior de Score: ";
                std::cin >> low;
                std::cout << "Ingrese el límite superior de Score: ";
                std::cin >> high;

                int nodesVisited = 0;
                auto start = std::chrono::high_resolution_clock::now();
                std::vector<Record> records = db.findBetween(low, high, nodesVisited);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::milli>(end - start).count();

                std::cout << ANSI_BOLD << "\nEvidencia del Recorrido de Rango:" << ANSI_RESET << std::endl;
                std::cout << "  - Nodos visitados (llamadas recursivas/ramas exploradas): " << nodesVisited << std::endl;

                if (!records.empty()) {
                    std::cout << ANSI_GREEN << "\n[✔] Se encontraron " << records.size() << " registro(s) en " << t << " ms:" << ANSI_RESET << std::endl;
                    printRecordHeader();
                    for (const auto& r : records) {
                        printRecordRow(r);
                    }
                } else {
                    std::cout << ANSI_YELLOW << "\n[!] No se encontraron registros en el rango [" << low << ", " << high << "]." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 7: {
                double low, high;
                std::cout << "Ingrese el límite inferior: ";
                std::cin >> low;
                std::cout << "Ingrese el límite superior: ";
                std::cin >> high;

                auto start = std::chrono::high_resolution_clock::now();
                int count = db.countBetween(low, high);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::nano>(end - start).count();

                std::cout << ANSI_GREEN << "\n[✔] Cantidad de registros en rango: " << count << ANSI_RESET << std::endl;
                std::cout << "  - Complejidad: " << ANSI_BOLD << "O(log u)" << ANSI_RESET << " (usando subtreeSize aumentado)" << std::endl;
                std::cout << "  - Tiempo de cálculo: " << ANSI_BOLD << t << " ns" << ANSI_RESET << std::endl;
                break;
            }
            case 8: {
                int k;
                std::cout << "Ingrese la cantidad k de registros con mejor score a mostrar: ";
                std::cin >> k;

                auto start = std::chrono::high_resolution_clock::now();
                std::vector<Record> records = db.topK(k);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::nano>(end - start).count();

                if (!records.empty()) {
                    std::cout << ANSI_GREEN << "\n[✔] Top " << records.size() << " registros con mayor score (Tiempo: " << t << " ns):" << ANSI_RESET << std::endl;
                    printRecordHeader();
                    for (const auto& r : records) {
                        printRecordRow(r);
                    }
                } else {
                    std::cout << ANSI_YELLOW << "[!] No hay registros en el sistema." << ANSI_RESET << std::endl;
                }
                break;
            }
            case 9: {
                auto start = std::chrono::high_resolution_clock::now();
                double med = db.median();
                double p25 = db.percentile(25.0);
                double p75 = db.percentile(75.0);
                double p90 = db.percentile(90.0);
                auto end = std::chrono::high_resolution_clock::now();
                double t = std::chrono::duration<double, std::nano>(end - start).count();

                std::cout << ANSI_GREEN << "\n[✔] Estadísticas calculadas en O(log u) (Tiempo: " << t << " ns):" << ANSI_RESET << std::endl;
                std::cout << "  - Total de registros indexados: " << db.size() << std::endl;
                std::cout << "  - Mediana (Percentil 50): " << ANSI_BOLD << med << ANSI_RESET << std::endl;
                std::cout << "  - Percentil 25:           " << p25 << std::endl;
                std::cout << "  - Percentil 75:           " << p75 << std::endl;
                std::cout << "  - Percentil 90:           " << p90 << std::endl;
                break;
            }
            case 10: {
                int optSize;
                std::cout << "¿Qué tamaño de experimento desea correr?\n";
                std::cout << "1. Pequeño (10,000 registros)\n";
                std::cout << "2. Mediano (100,000 registros)\n";
                std::cout << "3. Grande (500,000 registros)\n";
                std::cout << "Seleccione (1-3): ";
                std::cin >> optSize;

                int testSize = 10000;
                if (optSize == 2) testSize = 100000;
                else if (optSize == 3) testSize = 500000;

                runBenchmark(testSize);
                break;
            }
            case 11: {
                std::cout << ANSI_BOLD << ANSI_YELLOW << "\nEstructura Visual del Red-Black Tree Aumentado:" << ANSI_RESET << std::endl;
                std::cout << "Nomenclatura: [Clave] (Color, sz = SubtreeSize Acumulado, ids = Cantidad de IDs en HashSet)" << std::endl;
                std::cout << std::string(80, '-') << std::endl;
                db.printTreeIndex();
                break;
            }
            default: {
                std::cout << ANSI_RED << "[!] Opción no reconocida." << ANSI_RESET << std::endl;
                break;
            }
        }
    }

    return 0;
}
