#include <iostream>
#include <cassert>
#include "../database.h"
using namespace std;

void testHashSet() {
    cout << "Probando HashSet... ";
    HashSet s;
    assert(s.size() == 0);
    assert(!s.contains(1));

    s.insert(1);
    s.insert(2);
    s.insert(2); // duplicado
    assert(s.size() == 2);
    assert(s.contains(1));
    assert(s.contains(2));
    assert(!s.contains(3));

    s.remove(2);
    assert(s.size() == 1);
    assert(!s.contains(2));
    assert(s.contains(1));

    s.clear();
    assert(s.size() == 0);
    assert(!s.contains(1));
    cout << "PASADO." << endl;
}

void testHashTable() {
    cout << "Probando HashTable... ";
    HashTable<int, string> ht;
    assert(ht.size() == 0);

    ht.insert(1, "Uno");
    ht.insert(2, "Dos");
    assert(ht.size() == 2);
    assert(ht.contains(1));
    assert(*ht.get(1) == "Uno");

    ht.insert(1, "Uno modificado");
    assert(ht.size() == 2);
    assert(*ht.get(1) == "Uno modificado");

    ht.remove(2);
    assert(ht.size() == 1);
    assert(!ht.contains(2));
    assert(ht.get(2) == nullptr);

    cout << "PASADO." << endl;
}

void testDatabaseAndRBT() {
    cout << "Probando Database y Red-Black Tree Aumentado... ";
    Database db;

    // Insertar películas
    db.insertMovie(Movie{1, "Alice in Wonderland", "Adventure|Children", 10.0});
    db.insertMovie(Movie{2, "Bob the Builder", "Children|Comedy", 20.0});
    db.insertMovie(Movie{3, "Charlie and the Chocolate Factory", "Children|Drama|Fantasy", 20.0}); // Calificación duplicada
    db.insertMovie(Movie{4, "David's Story", "Action|Drama", 30.0});
    db.insertMovie(Movie{5, "Eve of Destruction", "Sci-Fi|Thriller", 40.0});

    assert(db.size() == 5);

    // Buscar por ID
    Movie* r = db.findById(3);
    assert(r != nullptr);
    assert(r->title == "Charlie and the Chocolate Factory");
    assert(r->averageRating == 20.0);

    // Buscar no existente
    assert(db.findById(99) == nullptr);

    // Búsqueda por calificación exacta
    vector<Movie> r20 = db.findEqual(20.0);
    assert(r20.size() == 2); // Bob y Charlie
    assert((r20[0].movieId == 2 && r20[1].movieId == 3) || (r20[0].movieId == 3 && r20[1].movieId == 2));

    // Conteo en rango [15.0, 35.0]
    // Deberían estar Bob (20.0), Charlie (20.0) y David (30.0) -> Total: 3
    int count = db.countBetween(15.0, 35.0);
    assert(count == 3);

    // Conteo en rango no existente
    assert(db.countBetween(50.0, 60.0) == 0);

    // Mediana de {10.0, 20.0, 20.0, 30.0, 40.0} -> 20.0
    double med = db.median();
    assert(med == 20.0);

    // Percentiles
    // Percentil 0 -> 10.0, Percentil 100 -> 40.0
    assert(db.percentile(0.0) == 10.0);
    assert(db.percentile(100.0) == 40.0);
    
    // Top K
    vector<Movie> top3 = db.topK(3);
    assert(top3.size() == 3);
    // Deberían ser Eve (40), David (30), y luego Bob o Charlie (20)
    assert(top3[0].averageRating == 40.0);
    assert(top3[1].averageRating == 30.0);
    assert(top3[2].averageRating == 20.0);

    // Eliminar película
    bool delOk = db.deleteMovie(2); // Eliminar Bob (rating 20.0)
    assert(delOk);
    assert(db.size() == 4);
    assert(db.findById(2) == nullptr);

    // El rating 20.0 ahora debería tener solo a Charlie (1 registro)
    assert(db.findEqual(20.0).size() == 1);
    assert(db.countBetween(15.0, 35.0) == 2); // Charlie (20.0) y David (30.0)

    // Actualizar calificación
    // Charlie (ID 3) cambia de 20.0 a 35.0
    bool updOk = db.updateRating(3, 35.0);
    assert(updOk);
    assert(db.findEqual(20.0).empty()); // ya no hay nadie con 20.0
    assert(db.findEqual(35.0).size() == 1); // Charlie ahora tiene 35.0

    cout << "PASADO." << endl;
}

int main() {
    testHashSet();
    testHashTable();
    testDatabaseAndRBT();
    cout << "TODOS LOS TEST PASARON SATISFACTORIAMENTE." << endl;
    return 0;
}
