# Base de Datos en Memoria con Red-Black Tree Aumentado

Este proyecto consiste en una base de datos en memoria de alto rendimiento diseñada e implementada desde cero en C++ para el curso **CS2023 – Algoritmos y Estructuras de Datos**.

El sistema integra un **índice primario** para accesos puntuales veloces y un **índice secundario balanceado aumentado** para consultas por rango y estadísticas de orden en tiempo logarítmico.

---

## 1. Arquitectura e Integración de Índices

El motor de almacenamiento coordina dos estructuras de datos principales implementadas desde cero:

### A. Índice Primario (`HashTable<int, Record>`)
- Es una tabla hash con encadenamiento que mapea la clave primaria (`id` del registro) a los datos completos del registro en memoria.
- Ofrece búsquedas, inserciones y eliminaciones en tiempo constante promedio $O(1)$.
- Se redimensiona automáticamente (rehashing) duplicando su capacidad cada vez que el factor de carga supera el `0.75`.

### B. Índice Secundario Aumentado (`RedBlackTree<double>`)
- Un árbol binario de búsqueda balanceado (Rojo-Negro) cuyas claves son los atributos de puntaje (`score`).
- **Manejo de Claves Duplicadas (Estrategia RBT + HashSet):**
  Tradicionalmente, los árboles Rojo-Negro asumen claves únicas. Para permitir que múltiples registros compartan el mismo score exacto de manera eficiente, el RBT **agrupa los duplicados a nivel de nodo**.
  - Cada nodo `RBTNode<Key>` almacena un único `Key` (el score) y contiene internamente un **`HashSet` personalizado** con los IDs de todos los registros que poseen dicho score.
  - Esto evita la creación innecesaria de múltiples nodos con claves repetidas, lo cual aumentaría la altura del árbol, complicaría el rebalanceo y degradaría la complejidad de búsqueda a $O(N)$ en el peor caso.
  - Con esta integración, la altura del árbol depende del número de scores únicos $u$, manteniéndose estrictamente en $O(\log u)$.

### C. Integración en la Capa Coordinadora (`Database`)
La clase `Database` actúa como fachada y coordina ambos índices para garantizar la consistencia referencial:
- Al buscar registros por score (`findEqual` o `findBetween`), la base de datos consulta el `RedBlackTree` para obtener rápidamente los IDs calificados ($O(\log u)$) y luego recupera la información detallada de cada registro consultando la `HashTable` en tiempo constante ($O(1)$ por registro).

---

## 2. Invariante de Aumento y Puntos Críticos de Funcionamiento

### A. La Invariante `subtreeSize`
Para optimizar las operaciones de rango y orden, cada nodo $x$ en el RBT mantiene un acumulador entero `subtreeSize` que representa el número total de **registros individuales** indexados en su subárbol:
$$\text{subtreeSize}(x) = \text{subtreeSize}(x.\text{left}) + \text{subtreeSize}(x.\text{right}) + |x.\text{rowIds}|$$

Donde $|x.\text{rowIds}|$ es la cardinalidad del `HashSet` de IDs del propio nodo.

Esta invariante permite resolver consultas estadísticas en tiempo logarítmico:
- **`select(k)`**: Encuentra el score en la posición ordenada $k$ en $O(\log u)$. Compara $k$ contra el tamaño acumulado del hijo izquierdo. Si $k$ es menor, desciende a la izquierda; si cae dentro de los IDs del nodo actual, retorna el nodo; de lo contrario, resta los tamaños acumulados a $k$ y viaja a la derecha.
- **`countRange(low, high)`**: Devuelve el conteo exacto de registros en $[low, high]$ en $O(\log u)$ calculando la diferencia de rangos acumulados sin necesidad de recorrer ni instanciar los registros.

### B. El Nodo Centinela `NIL` (Evitación de Punteros Nulos)
El árbol utiliza un único nodo dummy global llamado `nil` para representar todas las hojas vacías:
- Este nodo `nil` es de color `BLACK` y tiene un `subtreeSize` explícito de `0`.
- Su uso crítico consiste en **eliminar chequeos manuales de punteros nulos** (`nullptr`) en las fórmulas de rotación, balanceo y estadísticas. Fórmulas como `x->left->subtreeSize` son siempre seguras y retornan `0` de forma natural cuando no hay hijo izquierdo.

### C. Mantenimiento del Tamaño en Puntos Críticos

1. **Rotaciones (Ajustes Locales en $O(1)$):**
   Al rotar un subárbol, la estructura de enlaces cambia y altera los tamaños acumulados de los nodos involucrados. En una rotación izquierda sobre $x$ que promueve a su hijo derecho $y$:
   - Los hijos de $x$ y $y$ cambian. Primero recalculamos el tamaño de $x$ (que ahora es el hijo) usando el estado actualizado de sus subárboles izquierdos y derechos.
   - Seguidamente, recalculamos el tamaño de $y$ (que ahora es el padre) utilizando el nuevo tamaño de $x$.
   - Esto mantiene la invariante al instante en tiempo constante $O(1)$ sin requerir travesías completas.

2. **Inserción (Propagación Ascendente):**
   - Si el score ya existe, el ID se añade al `HashSet` del nodo. Dado que no cambia la estructura del árbol, solo se recorren los punteros `parent` hacia arriba hasta la raíz, incrementando el `subtreeSize` de cada ancestro en `+1` en $O(\log u)$.
   - Si el score es nuevo, se crea un nodo con tamaño inicial `1` (que contiene el ID). Tras posicionarlo, se sube por sus ancestros incrementando sus tamaños en `+1`. Si el rebalanceo (`insertFixup`) realiza rotaciones, estas ajustan localmente los tamaños correspondientes.

3. **Borrado y Trasplantes (La Operación Más Crítica):**
   - Si al remover un ID el `HashSet` del nodo RBT aún conserva elementos, el nodo no se remueve físicamente. Solo decrementamos en `1` los acumulados `subtreeSize` de todos sus ancestros subiendo hasta la raíz en $O(\log u)$.
   - Si el `HashSet` se vacía, el nodo se elimina físicamente aplicando el algoritmo estándar de borrado CLRS (reemplazando el nodo por su sucesor o hijo). Al desvincular o trasplantar nodos, se rastrea el **nodo físico más bajo que sufrió una modificación estructural** (el padre del nodo desvinculado). A partir de ese nodo padre, se propaga una actualización de `subtreeSize` hacia la raíz (`updateSubtreeSizesUp`) y se aplica el balanceo de colores (`deleteFixup`).

---

## 3. Implementación de Funciones Principales

### A. Inserción de Registros (`insertRecord`)
1. Se verifica la unicidad de la clave primaria `id` en la `HashTable` en $O(1)$. Si ya existe, se aborta la operación.
2. Si el ID es único, se almacena el registro en el índice primario (`HashTable`).
3. Se inserta la pareja `(score, id)` en el `RedBlackTree` actualizando los tamaños y el balanceo.

### B. Eliminación de Registros (`deleteRecord`)
1. Se busca el registro por su `id` en la `HashTable` en $O(1)$. Si no existe, se aborta la operación.
2. Recuperamos el `score` de ese registro para ubicar el nodo correspondiente en el RBT.
3. Se elimina el `id` del `HashSet` de dicho nodo. Si el conjunto queda vacío, se remueve el nodo físicamente y se balancea en $O(\log u)$.
4. Finalmente, se elimina el registro de la `HashTable`.

### C. Búsqueda por ID (`findById`)
- Realiza una búsqueda directa y rápida en la `HashTable` en $O(1)$ promedio gracias al rehashing dinámico.

### D. Búsqueda por Score Exacto (`findEqual`)
- Busca el nodo en el RBT en $O(\log u)$. Si existe, recupera los registros de la `HashTable` para cada ID guardado en su `HashSet` en $O(1)$ por elemento.

### E. Búsqueda por Rango de Scores (`findBetween`)
- Aplica una poda recursiva en orden parcial sobre el RBT. Visita únicamente los subárboles que intersectan el rango $[low, high]$ en $O(\log u + k)$, recuperando la información de la `HashTable` para cada ID encontrado.

### F. Conteo en Rango (`countBetween`)
- Resuelve en tiempo logarítmico $O(\log u)$ aplicando:
  $$\text{Resultado} = \text{countLessThanOrEqual}(high) - \text{countLessThan}(low)$$
  Cada llamada desciende por el árbol acumulando instantáneamente el tamaño del hijo izquierdo y el nodo actual cuando decide doblar a la derecha.

### G. Selección, Mediana y Percentiles (`select`, `median`, `percentile`)
- **`select(k)`**: Busca la clave en la posición de orden $k$ en $O(\log u)$ comparando recursivamente $k$ contra los tamaños acumulados izquierdos.
- **`median()`**: Calcula la mediana en $O(\log u)$ invocando `select(N / 2)` (o el promedio de los centrales si $N$ es par).
- **`percentile(p)`**: Calcula la posición $idx = \text{round}(p \times (N - 1) / 100.0)$ y retorna el score llamando a `select(idx)` en $O(\log u)$.

---

## 4. Estructura del Código

El código está estructurado sin dependencias externas:

- **`hash/hashset.h`**: Tabla hash para enteros positivos usada para manejar los IDs duplicados de un score.
- **`hash/hashtable.h`**: Tabla hash asociativa genérica con soporte para rehashing dinámico.
- **`tree/RBTnode.h`**: Definición del nodo con claves genéricas, color, punteros a hijos/padre y la propiedad `subtreeSize`.
- **`tree/redblacktree.h`**: Reglas de rotación, inserción y borrado estándar (CLRS) junto a funciones estadísticas.
- **`database.h`**: Coordinación de índices y clase `NaiveDatabase` (búsqueda lineal) para comparaciones.
- **`main.cpp`**: Aplicación de consola interactiva con generador de datos y pruebas experimentales.

---

## 5. Análisis de Complejidad Comparativo

| Operación | Base de Datos Indexada | Base de Datos Naive (Vector) |
| :--- | :--- | :--- |
| **Insertar Registro** | $O(\log u)$ | $O(N)$ (Verifica unicidad de ID) |
| **Eliminar por ID** | $O(\log u)$ | $O(N)$ (Búsqueda secuencial) |
| **Buscar por ID** | $O(1)$ promedio | $O(N)$ lineal |
| **Buscar por Rango** | $O(\log u + k)$ | $O(N)$ lineal |
| **Contar en Rango** | $O(\log u)$ | $O(N)$ lineal |
| **Cálculo de Mediana** | $O(\log u)$ | $O(N \log N)$ (Ordenamiento) |
| **Cálculo de Percentil** | $O(\log u)$ | $O(N \log N)$ (Ordenamiento) |

---

## 6. Instrucciones de Compilación y Ejecución

### Requisitos
- Compilador de C++ (que soporte C++11 o superior, ej. `g++`).

### Compilación del Programa Principal
```bash
g++ -O3 -Wall main.cpp -o main
```

### Ejecución
```bash
./main
```

### Compilación y Corrida de Pruebas de Unidad
```bash
g++ scratch/test_rbt.cpp -o test_rbt && ./test_rbt
```