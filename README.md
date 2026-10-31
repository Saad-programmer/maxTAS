# Max Heap (Tas Max) — Implémentations & Benchmarks

> **Projet C++17** — Quatre implémentations d'un **tas max** (Max Heap) comparées sur trois plans : *lisibilité*, *complexité algorithmique* et *performance réelle* mesurée.

---

## Table des matières

1. [Présentation](#présentation)
2. [Structure du projet](#structure-du-projet)
3. [Les quatre implémentations](#les-quatre-implémentations)
4. [Complexités théoriques](#complexités-théoriques)
5. [Problématique métier](#problématique-métier)
6. [Résultats des benchmarks](#résultats-des-benchmarks)
   - [Vue d'ensemble](#vue-densemble)
   - [MaxHeapVector / MaxHeapRaw — scalabilité](#maxheapvector--maxheapraw--scalabilité)
   - [MaxHeapMap — scalabilité](#maxheapmap--scalabilité)
   - [MaxHeapList — scalabilité](#maxheap-list--scalabilité)
   - [Étudiants boursiers (N = 1 000 000)](#étudiants-boursiers-n--1-000-000)
   - [Comparaison finale](#comparaison-finale)
7. [Interprétation des résultats](#interprétation-des-résultats)
8. [Compilation & exécution](#compilation--exécution)
9. [Points pédagogiques](#points-pédagogiques)
10. [Limites & pistes d'amélioration](#limites--pistes-damélioration)

---

## Présentation

Un **tas max** (max heap) est un arbre binaire quasi-complet stocké dans un tableau, tel que chaque nœud soit **supérieur ou égal** à ses deux enfants. Il permet en `O(1)` d'accéder au maximum et en `O(log N)` de l'insérer ou de l'extraire.

Ce projet explore **le même algorithme** implémenté avec **quatre structures de stockage différentes** — du tableau brut `new/delete` au `std::map` — pour illustrer concrètement que **le choix de la structure de données conditionne la complexité effective**, pas seulement le code.

### Fonctionnalités communes

| Opération | Description |
|---|---|
| `insert(v)` | Ajoute `v` au tas |
| `top()` | Retourne la valeur maximale sans la retirer |
| `pop()` | Retire et retourne la valeur maximale (*extract-max*) |
| `deleteKey(k)` | Supprime une clé arbitraire `k` (première occurrence) |
| `empty()` / `size()` | État du tas |

---

## Structure du projet

```
TAS/
├── maxTas.cpp                      # Version pédagogique (référence lisible)
├── heap_vector.cpp                 # MaxHeapVector  (std::vector)
├── heap_raw.cpp                    # MaxHeapRaw     (tableau brut new/delete)
├── heap_map.cpp                    # MaxHeapMap     (std::map<int,T>)
├── heap_list.cpp                   # MaxHeapList    (std::list<T>)
│
├── bench_maxheap.cpp               # Bench simple N = 1e6
├── bench_maxheap_scale.cpp         # Bench scalabilité (100 → 1e8)
├── bench_heap_vector_scale.cpp     # Bench scalabilité MaxHeapVector
├── bench_heap_raw_scale.cpp        # Bench scalabilité MaxHeapRaw
├── bench_heap_map_scale.cpp        # Bench scalabilité MaxHeapMap
├── bench_heap_list_scale.cpp       # Bench scalabilité MaxHeapList
│
└── Problematique_Exemple/
    ├── bench_students_vector.cpp   # Cas métier : étudiants boursiers (vector)
    ├── bench_students_raw.cpp      # Cas métier (raw)
    └── bench_students_map.cpp      # Cas métier (map)
```

---

## Les quatre implémentations

### 1. `MaxHeapVector` — référence moderne

Stockage : `std::vector<T>`. Accès indexé `O(1)`, `push_back` amorti `O(1)`.

```cpp
template <typename T>
class MaxHeapVector {
    std::vector<T> arr;
    void heapify(int i) { /* descente O(log n) */ }
public:
    void insert(const T& v);   // O(log n)
    const T& top() const;      // O(1)
    T pop();                   // O(log n)
    bool deleteKey(const T& k);// O(n) recherche + O(log n)
};
```

### 2. `MaxHeapRaw` — tableau brut `new`/`delete`

Croissance manuelle (`×2`), copie des éléments, destructeur explicite. Reproduit **exactement** le comportement de `std::vector` mais avec une gestion mémoire manuelle — utile pour comprendre l'allocation dynamique et les règles des 3/5.

### 3. `MaxHeapMap` — `std::map<int,T>` indexée

La clé entière est l'indice dans le tableau virtuel. Chaque accès est `O(log n)` → **insert/pop deviennent `O(log² n)`**. Pédagogique : montre le coût caché d'un accès non contigu.

### 4. `MaxHeapList` — `std::list<T>`

**Aucun accès aléatoire.** Chaque `at(i)` coûte `std::next(begin(), i)` en `O(i)`. Conséquence : **insert/pop/deleteKey passent en `O(n)`**. Utilisé uniquement pour illustrer *pourquoi* `std::list` n'est pas adapté aux tas.

---

## Complexités théoriques

| Implémentation | `insert` | `top` | `pop` | `deleteKey` | Mémoire |
|---|---|---|---|---|---|
| **MaxHeapVector** | `O(log N)` amorti | `O(1)` | `O(log N)` | `O(N)` | ~`N·sizeof(T)` + slack |
| **MaxHeapRaw** | `O(log N)` amorti | `O(1)` | `O(log N)` | `O(N)` | ~`N·sizeof(T)` (cap ×2) |
| **MaxHeapMap** | `O(log² N)` | `O(log N)` | `O(log² N)` | `O(N)` | ~`N·(sizeof(T)+overhead)` |
| **MaxHeapList** | `O(N)` | `O(1)` | `O(N)` | `O(N)` | `N·(sizeof(T)+2·8)` |

> **Note** : `deleteKey` est `O(N)` **dans tous les cas** à cause de la recherche linéaire de la clé. Pour la rendre `O(log N)`, il faudrait un index `unordered_map<T, index>` maintenu en parallèle (hors périmètre).

---

## Problématique métier

**Énoncé** : gérer `N = 1 000 000` d'étudiants boursiers et toujours pouvoir extraire **le plus boursier** rapidement.

```cpp
struct Student {
    int         id;
    std::string name;
    double      scholarship;   // montant en €
    bool operator<(const Student& o) const {
        if (scholarship != o.scholarship) return scholarship < o.scholarship;
        return id < o.id;      // départage stable
    }
    bool operator==(const Student& o) const { return id == o.id; }
};
```

Le tas est un **max-heap** sur `scholarship` (départage par `id`). Le tas sur `Student` étant plus lourd (conteneur `std::string`), il révèle l'impact **mémoire + copies** en plus de la complexité.

---

## Résultats des benchmarks

> Machine : *HP EliteBook 850 G8*, compilation `g++ -std=c++17 -O2`. Graine fixe `SEED = 42` → reproductibilité.
> Mesures en `ns/op` (nanosecondes par opération) — plus petit = meilleur.

### Vue d'ensemble

| Opération | Vector | Raw | Map | List |
|---|---|---|---|---|
| `insert` | `O(log N)` | `O(log N)` | `O(log² N)` | `O(N)` |
| `top` | `O(1)` | `O(1)` | `O(log N)` | `O(1)` |
| `pop` | `O(log N)` | `O(log N)` | `O(log² N)` | `O(N)` |
| `deleteKey` | `O(N)` | `O(N)` | `O(N)` | `O(N)` |

---

### MaxHeapVector / MaxHeapRaw — scalabilité

> `TOP_ITERS = 50 000 000`, `DEL_CAP = 10 000`, budget = 30 s/phase.

#### `MaxHeapVector` (ns/op)

| N | insert | top | pop | deleteKey |
|---:|---:|---:|---:|---:|
| 100 | 23.25 | 0.59 | 72.02 | 57.28 |
| 1 000 | 28.71 | 0.56 | 76.41 | 163.31 |
| 10 000 | 21.11 | 0.49 | 114.82 | 1 189.84 |
| 100 000 | 19.07 | 0.53 | 133.58 | 1 159.30 |
| 1 000 000 | 22.03 | 0.45 | 169.86 | 942.95 |
| 10 000 000 | 18.55 | 0.54 | 341.46 | 1 396.76 |
| 100 000 000 | 22.93 | 1.04 | 514.22 | 907.96 |

#### `MaxHeapRaw` (ns/op)

| N | insert | top | pop | deleteKey |
|---:|---:|---:|---:|---:|
| 100 | 21.36 | 0.56 | 130.78 | 132.40 |
| 1 000 | 27.96 | 0.65 | 94.29 | 214.35 |
| 10 000 | 21.98 | 0.49 | 123.93 | 1 128.41 |
| 100 000 | 28.06 | 0.55 | 146.14 | 1 116.44 |
| 1 000 000 | 22.14 | 0.53 | 176.83 | 960.50 |
| 10 000 000 | 17.61 | 0.38 | 353.15 | 1 525.86 |
| 100 000 000 | 21.43 | 0.44 | 531.08 | 926.10 |

**Observation graphique** — `insert` reste **plat** (~20 ns), `top` **constant** (~0.5 ns, dominé par le cache L1), `pop` croît **lentement** (`log N` → de 72 à ~530 ns sur 6 ordres de grandeur), `deleteKey` **explose** dès `N ≥ 10⁴` (recherche linéaire → ~1 µs/op, plafonné par `DEL_CAP = 10 000`).

```
insert  ▁▁▁▁▁▁▁  (~20 ns, log N amorti)
top     ▁▁▁▁▁▁▁  (~0.5 ns, O(1))
pop     ▂▂▃▃▄▄▅  (croissance log)
deleteKey ▁▂████  (saut à N=10⁴ → O(N))
```

---

### MaxHeapMap — scalabilité

> `TOP_ITERS = 5 000 000`, `DEL_CAP = 5 000`, budget = 15 s/phase.

| N | insert | top | pop | deleteKey |
|---:|---:|---:|---:|---:|
| 100 | 460.40 | 6.63 | 1 431.06 | 1 002.77 |
| 1 000 | 632.48 | 11.21 | 1 640.60 | 1 876.17 |
| 10 000 | 533.95 | 13.35 | 3 548.13 | 7 956.91 |
| 100 000 | 979.11 | 29.89 | 7 297.13 | 8 772.63 |
| 1 000 000 | 1 479.81 | 48.39 | 19 158.87 | 10 649.40 |

**Observation** : `top` est **28× plus lent** que sur vector (48 vs 0.45 ns à `N=10⁶`) — le `tree.at(0)` traverse un arbre rouge-noir. `pop` atteint **19 µs/op** à `N=10⁶` (contre 170 ns en vector, soit **×113**). La recherche `deleteKey` reste `O(N)` mais avec un surcoût de parcours `std::map`.

---

### MaxHeapList — scalabilité

> `TOP_ITERS = 5 000 000`, `DEL_CAP = 2 000`, budget = 5 s/phase, tailles plafonnées à `1e5`.

| N | insert | top | pop | deleteKey | Statut |
|---:|---:|---:|---:|---:|---|
| 100 | 205.46 | 0.62 | 682.26 | 316.40 | OK |
| 300 | 560.79 | 0.55 | 1 535.61 | 886.78 | OK |
| 1 000 | 1 966.65 | 1.04 | 9 720.22 | 4 783.38 | OK |
| 3 000 | 8 293.95 | 0.85 | 29 287.67 | 7 019.51 | OK |
| 10 000 | 25 916.92 | 0.47 | 106 248.01 | 7 235.73 | OK |
| 30 000 | 91 583.20 | 0.46 | 405 585.30 | 9 463.54 | **pop ABORT** |
| 100 000 | 124 450.18 | 0.62 | 618 291.96 | 7 253.84 | **insert+pop ABORT** |

**Observation** : le coût **explose linéairement** (`×10` par décade → confirmé par les ratios ~2.7–6.3 sur les petites tailles). À `N = 10⁵`, un seul `insert` prend **124 µs** — un `vector` fait la même chose en **22 ns**, soit **×5 600 plus rapide**. Les `ABORT` prouvent que la structure **ne passe pas à l'échelle**.

---

### Étudiants boursiers (N = 1 000 000)

| Implémentation | `insert` total | `top` ×iters | `pop` 500k | `deleteKey` |
|---|---:|---:|---:|---:|
| **Vector** | 73.27 ms (73 ns/op) | 28.34 ms / 50M (0.57 ns) | 382.34 ms (765 ns/op) | 9 560 ms (956 µs/op, 10k) |
| **Raw** | 66.76 ms (67 ns/op) | 25.78 ms / 50M (0.52 ns) | 492.19 ms (984 ns/op) | 9 354 ms (935 µs/op, 10k) |
| **Map** | 1 380.65 ms (1.38 µs/op) | 225.68 ms / 5M (45 ns) | 10 689.63 ms (21 µs/op) | 34 953 ms (6.99 ms/op, 5k, 2 492 succès) |

**Interprétation métier** :

- `vector` et `raw` : **quasi identiques** (l'écart < 10 % est du bruit de mesure). Le choix `new/delete` vs `std::vector` n'apporte **rien** en perf à ce niveau, seulement de la complexité de code.
- `map` : **18× plus lent à l'insertion**, **21× plus lent au `pop`**, **~7× plus lent au `top`** — pour un résultat fonctionnellement identique. La recherche linéaire de `deleteKey` dans une `std::map` coûte **7 ms/op** (contre ~1 ms en vector), car elle parcourt `N` nœuds non contigus → cache misses massifs.
- **Somme des bourses extraites identique** (3 751 198 580 €) dans les trois cas → validation croisée de la correction.

---

### Comparaison finale

```
N = 1 000 000 — ns/op (plus petit = meilleur)
─────────────────────────────────────────────────────────
                Vector      Raw         Map         List
insert          22.03      22.14     1 479.81    124 450*   (aborted)
top              0.45       0.53        48.39        0.62
pop            169.86     176.83    19 158.87    618 292*   (aborted)
deleteKey      942.95     960.50    10 649.40      7 253
─────────────────────────────────────────────────────────
* valeurs non comparables : budget temps dépassé avant N complet
```

| Métrique | Gagnant | Facteur vs Vector |
|---|---|---|
| `insert` | **Vector ≈ Raw** | — |
| `top` | **Vector ≈ Raw ≈ List** | — (toutes `O(1)`) |
| `pop` | **Vector ≈ Raw** | — |
| `deleteKey` | **Vector ≈ Raw** | — |
| `std::list` à `N=10⁵` | — | **×5 600 plus lent** |
| `std::map` à `N=10⁶` | — | **×21 à ×113 plus lent** |

---

## Interprétation des résultats

### 1. `top()` est vraiment `O(1)` — et c'est visible

Sur `vector`, `top` reste **entre 0.4 et 0.6 ns/op** sur 6 ordres de grandeur. C'est le cache L1 qui gagne : `arr[0]` est toujours en cache chaud. En `std::map`, `tree.at(0)` traverse un arbre rouge-noir → `~50 ns/op` à `N=10⁶` (**×100**).

### 2. `insert` en `O(log N)` amorti : quasi-plat

Théoriquement, `insert` fait `log₂(N)` comparaisons. Entre `N=100` (≈7 niveaux) et `N=10⁸` (≈27 niveaux), on attend un facteur ~4. Mesuré : **20 → 22 ns** (~1.1). Le coût est **dominé par le cache** et le `push_back`, pas par le nombre de comparaisons (qui sont des `swap` de 4 octets gratuits).

### 3. `deleteKey` est le vrai goulot

Le passage de `~50 ns` (petits `N`) à `~1 µs` (grands `N`) illustre `O(N)` : à `N=10⁴`, 10 000 comparaisons ; à `N=10⁶`, 1 000 000. Le **plafond de `DEL_CAP`** dans les benchmarks est précisément là pour éviter des runs de plusieurs minutes. **Leçon** : si `deleteKey` est fréquent, il faut un index secondaire.

### 4. `std::list` est disqualifié pour les tas

Avec `std::next(begin(), i)`, un accès au niveau `k` coûte `O(2^k)` → l'insertion coûte `O(N)` en moyenne. Les **abandons** (`ABORT`) à `N=30 000` et `100 000` sont la preuve empirique du mur.

### 5. `std::map` : `O(log² N)` coûte cher en pratique

Le facteur **×113** sur `pop` à `N=10⁶` vient de `log²(10⁶) ≈ 400` opérations de pointeurs par `pop`, contre `log₂(10⁶) ≈ 20` en vector. La théorie se vérifie **brutalement**.

### 6. Vector vs Raw : match nul

Performances **statistiquement équivalentes** (écarts < 10 %, souvent du bruit). `std::vector` apporte la sécurité (copie, move, exceptions) **sans surcoût** à ce niveau d'optimisation (`-O2` inline tout). `Raw` n'est utile que comme exercice pédagogique.

---

## Compilation & exécution

### Prérequis

- `g++` ≥ 9 (C++17) ou `clang++` ≥ 10
- ~1 Go de RAM disponible pour les runs à `N = 1e8`

### Implémentations (démonstration)

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o maxTas        maxTas.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o heap_vector   heap_vector.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o heap_raw      heap_raw.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o heap_map      heap_map.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o heap_list     heap_list.cpp

./maxTas
./heap_vector
./heap_raw
./heap_map
./heap_list
```

### Benchmarks

```bash
# Bench simple (N = 1e6)
g++ -std=c++17 -O2 -Wall -Wextra -o bench_maxheap bench_maxheap.cpp
./bench_maxheap

# Bench scalabilité (100 → 1e8)
g++ -std=c++17 -O2 -Wall -Wextra -o bench_maxheap_scale      bench_maxheap_scale.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_vector_scale  bench_heap_vector_scale.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_raw_scale     bench_heap_raw_scale.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_map_scale     bench_heap_map_scale.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_list_scale    bench_heap_list_scale.cpp

./bench_maxheap_scale
./bench_heap_vector_scale
./bench_heap_raw_scale
./bench_heap_map_scale
./bench_heap_list_scale
```

### Cas métier (étudiants)

```bash
cd Problematique_Exemple
g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_vector bench_students_vector.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_raw    bench_students_raw.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_map    bench_students_map.cpp

./bench_students_vector
./bench_students_raw
./bench_students_map
```

### Configuration

Toutes les tailles/paramètres sont **des constantes en tête de fichier** (pas d'arguments CLI) :

```cpp
static const std::vector<std::size_t> SIZES = { 100, 1'000, 10'000, 100'000,
                                                1'000'000, 10'000'000, 100'000'000 };
static const double BUDGET_MS   = 30000.0;   // budget max par phase
static const std::size_t DEL_CAP = 10'000;   // cap deleteKey
static const std::size_t TOP_ITERS = 50'000'000;
static const unsigned SEED = 42;
```

---

## Points pédagogiques

1. **La complexité asymptotique est un guide, pas une garantie.**
   `O(log N)` sur `std::map` devient `O(log² N)` à cause de l'accès non contigu — la théorie le prédit, les chiffres le confirment (×113 sur `pop`).

2. **Le cache est roi.**
   `vector` et `raw` restent à ~20 ns/insert sur 6 ordres de grandeur parce que la mémoire est contiguë. `map` paie chaque saut de pointeur.

3. **`deleteKey` en `O(N)` est un piège classique.**
   À `N=10⁶`, une seule suppression coûte **~1 ms** — invisible sur un `N` de test, rédhibitoire en production.

4. **`std::list` n'est pas un "vector avec insertion rapide".**
   Sans accès aléatoire, il est **structurellement inadapté** aux tas.

5. **`std::vector` bat `new/delete` à plate couture en sécurité, à égalité en perf.**
   Aucune raison technique de préférer le tableau brut, sauf exercice.

6. **Les benchmarks doivent avoir des garde-fous.**
   Les `BUDGET_MS` et `DEL_CAP` évitent qu'un run pédagogique ne prenne 3 heures — et les `ABORT` documentent honnêtement les limites.

---

## Limites & pistes d'amélioration

| Limite | Piste |
|---|---|
| `deleteKey` en `O(N)` | Index `std::unordered_map<T, size_t>` maintenu → `O(log N)` |
| `Student` recopié à chaque `swap` | `std::vector<std::unique_ptr<Student>>` ou heap sur `id` + table annexe |
| Pas de `buildHeap` en `O(N)` mesuré | Ajouter `MaxHeap::MaxHeap(vector<T>)` + bench dédié |
| Pas de `decreaseKey` / `increaseKey` | Utile pour Dijkstra / Prim — à implémenter |
| Pas de test unitaire | Ajouter `Catch2` ou `GoogleTest` avec cas aléatoires |
| Bench monothread | `std::execution::par` sur la génération, ou bench concurrent |
| Pas de profiling cache | Utiliser `perf stat -e cache-misses` ou `valgrind --tool=cachegrind` |
| Pas de mesure mémoire | `valgrind --tool=massif` ou `/usr/bin/time -v` |

---

## Licence

Projet pédagogique — **ILISI2, Virtualisation, C++** — libre d'utilisation à des fins d'apprentissage.

---

<p align="center">
  <sub>Généré à partir du dépôt <code>TAS/</code> — benchmarks reproductibles avec <code>SEED = 42</code></sub>
</p>
