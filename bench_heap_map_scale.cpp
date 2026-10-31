// bench_heap_map_scale.cpp
// Benchmark MaxHeapMap (std::map<int,T>) — 100% self-contained, AUCUN argument CLI.
//
// Compilation :
//   g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_map_scale bench_heap_map_scale.cpp
// Exécution :
//   ./bench_heap_map_scale
//
// Règlages : constantes SIZES / BUDGET_MS / DEL_CAP / TOP_ITERS ci-dessous.
//
// !!! COMPLEXITÉ !!!
//   insert    : O(log^2 N)  (O(log N) accès map x O(log N) niveaux)
//   pop       : O(log^2 N)
//   deleteKey : O(N) recherche + O(log^2 N) remontée/descente
//   top       : O(log N) via tree.at(0)  (et non O(1) comme vector)
//   => on plafonne à 1e6, sinon insert complet = plusieurs minutes.

#include <iostream>
#include <map>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cstdint>
#include <stdexcept>

// ============================================================
//  CONFIGURATION — modifie ici, rien à passer en ligne de commande
// ============================================================
static const std::vector<std::size_t> SIZES = {
    100ULL, 1'000ULL, 10'000ULL, 100'000ULL, 1'000'000ULL
};
static const double BUDGET_MS = 15000.0;   // budget max par phase (ms)
static const std::size_t DEL_CAP = 5'000;  // deleteKey : cap du nombre d'ops
static const std::size_t TOP_ITERS = 5'000'000; // itérations pour top()
static const unsigned SEED = 42;
// ============================================================

// ---------- MaxHeapMap (copie minimale de heap_map.cpp) ----------
template <typename T>
class MaxHeapMap {
    std::map<int, T> tree;

    T&       ref(int i)       { return tree.at(i); }
    const T& ref(int i) const { return tree.at(i); }

    void heapify(int i) {
        int n = (int)tree.size();
        int l = 2*i+1, r = 2*i+2, largest = i;
        if (l < n && ref(l) > ref(largest)) largest = l;
        if (r < n && ref(r) > ref(largest)) largest = r;
        if (largest != i) {
            std::swap(ref(i), ref(largest));
            heapify(largest);
        }
    }
public:
    bool empty() const { return tree.empty(); }
    int  size()  const { return (int)tree.size(); }

    void insert(const T& v) {
        int idx = (int)tree.size();
        tree[idx] = v;
        int i = idx;
        while (i != 0) {
            int p = (i-1)/2;
            if (ref(p) >= ref(i)) break;
            std::swap(ref(i), ref(p));
            i = p;
        }
    }
    const T& top() const {
        if (tree.empty()) throw std::runtime_error("top() sur heap vide");
        return tree.at(0);
    }
    T pop() {
        if (tree.empty()) throw std::runtime_error("pop() sur heap vide");
        T root = tree.at(0);
        int last = (int)tree.size() - 1;
        if (last == 0) { tree.clear(); return root; }
        tree[0] = tree.at(last);
        tree.erase(last);
        heapify(0);
        return root;
    }
    bool deleteKey(const T& key) {
        int idx = -1;
        for (const auto& kv : tree) {
            if (kv.second == key) { idx = kv.first; break; }
        }
        if (idx == -1) return false;
        int last = (int)tree.size() - 1;
        if (idx == last) { tree.erase(last); return true; }
        tree[idx] = tree.at(last);
        tree.erase(last);
        int i = idx;
        while (i != 0) {
            int p = (i-1)/2;
            if (ref(p) >= ref(i)) break;
            std::swap(ref(i), ref(p));
            i = p;
        }
        heapify(i);
        return true;
    }
};

using Clock = std::chrono::high_resolution_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static std::string fmt_u(std::uint64_t v) {
    std::string s = std::to_string(v), out;
    int c = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i) {
        out.push_back(s[i]);
        if (++c == 3 && i != 0) { out.push_back(' '); c = 0; }
    }
    std::reverse(out.begin(), out.end());
    return out;
}

struct Row {
    std::size_t N = 0;
    double ins_ms = 0; std::size_t ins_done = 0; bool ins_abort = false;
    double top_ms = 0; std::size_t top_iters = 0;
    double pop_ms = 0; std::size_t pop_done = 0; bool pop_abort = false;
    double del_ms = 0; std::size_t del_done = 0; bool del_abort = false;
};

static Row run_one(std::size_t N, std::mt19937_64& rng) {
    Row r; r.N = N;
    std::uniform_int_distribution<int> dist(0, 10'000'000);

    std::vector<int> data(N);
    for (auto& x : data) x = dist(rng);

    // ---- INSERT (budget) ----
    MaxHeapMap<int> h;
    {
        auto t0 = Clock::now();
        for (std::size_t i = 0; i < N; ++i) {
            h.insert(data[i]);
            ++r.ins_done;
            if ((i & 0x3F) == 0 && ms(t0, Clock::now()) > BUDGET_MS) {
                r.ins_abort = true; break;
            }
        }
        r.ins_ms = ms(t0, Clock::now());
    }

    // ---- TOP ----
    if (!h.empty()) {
        r.top_iters = TOP_ITERS;
        volatile long long sink = 0;
        auto t0 = Clock::now();
        for (std::size_t i = 0; i < r.top_iters; ++i) sink += h.top();
        r.top_ms = ms(t0, Clock::now());
        (void)sink;
    }

    // ---- POP (moitié, budget) ----
    {
        std::size_t half = h.size() / 2;
        volatile long long sink = 0;
        auto t0 = Clock::now();
        for (std::size_t i = 0; i < half; ++i) {
            sink += h.pop();
            ++r.pop_done;
            if ((i & 0x3F) == 0 && ms(t0, Clock::now()) > BUDGET_MS) {
                r.pop_abort = true; break;
            }
        }
        r.pop_ms = ms(t0, Clock::now());
        (void)sink;
    }

    // ---- DELETE KEY (cap + budget) ----
    std::size_t del_count = std::min<std::size_t>(N, DEL_CAP);
    if (del_count > 0) {
        MaxHeapMap<int> hd;
        std::vector<int> keys(del_count);
        for (std::size_t i = 0; i < del_count; ++i) { int v = (int)i; hd.insert(v); keys[i] = v; }
        std::shuffle(keys.begin(), keys.end(), rng);

        auto t0 = Clock::now();
        for (std::size_t i = 0; i < del_count; ++i) {
            hd.deleteKey(keys[i]);
            ++r.del_done;
            if ((i & 0x0F) == 0 && ms(t0, Clock::now()) > BUDGET_MS) {
                r.del_abort = true; break;
            }
        }
        r.del_ms = ms(t0, Clock::now());
    }
    return r;
}

int main() {
    std::mt19937_64 rng(SEED);

    std::cout << "================================================================\n";
    std::cout << "  Benchmark MaxHeapMap (std::map) — self-contained\n";
    std::cout << "  Budget/phase = " << BUDGET_MS << " ms | DEL_CAP = " << DEL_CAP
              << " | TOP_ITERS = " << fmt_u(TOP_ITERS) << "\n";
    std::cout << "  /!\\ insert/pop = O(log^2 N), top = O(log N), deleteKey = O(N)\n";
    std::cout << "================================================================\n\n";

    std::vector<Row> rows;
    rows.reserve(SIZES.size());

    for (std::size_t N : SIZES) {
        std::cout << ">>> Exécution N = " << fmt_u(N) << " ... " << std::flush;
        Row r = run_one(N, rng);
        rows.push_back(r);
        std::cout << "OK";
        if (r.ins_abort) std::cout << " [insert ABORT]";
        if (r.pop_abort) std::cout << " [pop ABORT]";
        if (r.del_abort) std::cout << " [deleteKey ABORT]";
        std::cout << "\n";
    }

    std::cout << "\nRÉSULTATS DÉTAILLÉS\n-------------------\n\n";
    std::cout << std::fixed << std::setprecision(2);

    for (const auto& r : rows) {
        double ins_ns = r.ins_done ? r.ins_ms * 1e6 / (double)r.ins_done : 0.0;
        double top_ns = r.top_iters ? r.top_ms * 1e6 / (double)r.top_iters : 0.0;
        double pop_ns = r.pop_done ? r.pop_ms * 1e6 / (double)r.pop_done : 0.0;
        double del_ns = r.del_done ? r.del_ms * 1e6 / (double)r.del_done : 0.0;

        std::cout << "N = " << fmt_u(r.N) << "\n";
        std::cout << "  Insert   : " << std::setw(12) << r.ins_ms << " ms | "
                  << std::setw(14) << ins_ns << " ns/op ("
                  << fmt_u(r.ins_done) << " / " << fmt_u(r.N)
                  << (r.ins_abort ? ", ABORT" : "") << ")\n";
        std::cout << "  Top      : " << std::setw(12) << r.top_ms << " ms | "
                  << std::setw(14) << top_ns << " ns/op ("
                  << fmt_u(r.top_iters) << " iters)\n";
        std::cout << "  Pop      : " << std::setw(12) << r.pop_ms << " ms | "
                  << std::setw(14) << pop_ns << " ns/op ("
                  << fmt_u(r.pop_done) << (r.pop_abort ? ", ABORT" : "") << ")\n";
        std::cout << "  DeleteKey: " << std::setw(12) << r.del_ms << " ms | "
                  << std::setw(14) << del_ns << " ns/op ("
                  << fmt_u(r.del_done) << (r.del_abort ? ", ABORT" : "") << ")\n\n";
    }

    // Tableau récap
    std::cout << "TABLEAU RÉCAPITULATIF (ns/op)\n-----------------------------\n";
    std::cout << std::left  << std::setw(12) << "N"
              << std::right << std::setw(16) << "insert"
              << std::setw(14) << "top"
              << std::setw(16) << "pop"
              << std::setw(16) << "deleteKey" << "\n";
    std::cout << std::string(74, '-') << "\n";
    auto nso = [](const Row& r, int kind) -> double {
        switch (kind) {
            case 0: return r.ins_done ? r.ins_ms * 1e6 / (double)r.ins_done : 0.0;
            case 1: return r.top_iters ? r.top_ms * 1e6 / (double)r.top_iters : 0.0;
            case 2: return r.pop_done ? r.pop_ms * 1e6 / (double)r.pop_done : 0.0;
            default:return r.del_done ? r.del_ms * 1e6 / (double)r.del_done : 0.0;
        }
    };
    for (const auto& r : rows) {
        std::cout << std::left  << std::setw(12) << fmt_u(r.N)
                  << std::right << std::setw(16) << nso(r,0)
                  << std::setw(14) << nso(r,1)
                  << std::setw(16) << nso(r,2)
                  << std::setw(16) << nso(r,3) << "\n";
    }

    // Ratios
    std::cout << "\nRATIOS ns/op entre N et N/10\n-----------------------------\n";
    std::cout << "Attendu MaxHeapMap : top ~log(N) (≈1.2-1.5 entre N et N/10)\n";
    std::cout << "                     insert/pop ~log^2 => ratio ≈ 1.5-2\n";
    std::cout << "                     deleteKey ~O(N) => ratio ≈ 10\n\n";
    std::cout << std::left  << std::setw(18) << "N"
              << std::right << std::setw(14) << "insert"
              << std::setw(14) << "top"
              << std::setw(14) << "pop"
              << std::setw(14) << "deleteKey" << "\n";
    std::cout << std::string(74, '-') << "\n";
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const auto& a = rows[i-1];
        const auto& b = rows[i];
        auto ratio = [](double x, double y) -> double { return (x > 0.0 && y > 0.0) ? y / x : 0.0; };
        std::cout << std::left  << std::setw(18) << (fmt_u(b.N) + " / " + fmt_u(a.N))
                  << std::right << std::setw(14) << ratio(nso(a,0), nso(b,0))
                  << std::setw(14) << ratio(nso(a,1), nso(b,1))
                  << std::setw(14) << ratio(nso(a,2), nso(b,2))
                  << std::setw(14) << ratio(nso(a,3), nso(b,3)) << "\n";
    }

    std::cout << "\nTerminé.\n";
    return 0;
}

/*
resultat terminal:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./bench_heap_map_scale
================================================================
  Benchmark MaxHeapMap (std::map) — self-contained
  Budget/phase = 15000 ms | DEL_CAP = 5000 | TOP_ITERS = 5 000 000
  /!\ insert/pop = O(log^2 N), top = O(log N), deleteKey = O(N)
================================================================

>>> Exécution N = 100 ... OK
>>> Exécution N = 1 000 ... OK
>>> Exécution N = 10 000 ... OK
>>> Exécution N = 100 000 ... OK
>>> Exécution N = 1 000 000 ... OK

RÉSULTATS DÉTAILLÉS
-------------------

N = 100
  Insert   :         0.05 ms |         460.40 ns/op (100 / 100)
  Top      :        33.16 ms |           6.63 ns/op (5 000 000 iters)
  Pop      :         0.07 ms |        1431.06 ns/op (50)
  DeleteKey:         0.10 ms |        1002.77 ns/op (100)

N = 1 000
  Insert   :         0.63 ms |         632.48 ns/op (1 000 / 1 000)
  Top      :        56.06 ms |          11.21 ns/op (5 000 000 iters)
  Pop      :         0.82 ms |        1640.60 ns/op (500)
  DeleteKey:         1.88 ms |        1876.17 ns/op (1 000)

N = 10 000
  Insert   :         5.34 ms |         533.95 ns/op (10 000 / 10 000)
  Top      :        66.75 ms |          13.35 ns/op (5 000 000 iters)
  Pop      :        17.74 ms |        3548.13 ns/op (5 000)
  DeleteKey:        39.78 ms |        7956.91 ns/op (5 000)

N = 100 000
  Insert   :        97.91 ms |         979.11 ns/op (100 000 / 100 000)
  Top      :       149.44 ms |          29.89 ns/op (5 000 000 iters)
  Pop      :       364.86 ms |        7297.13 ns/op (50 000)
  DeleteKey:        43.86 ms |        8772.63 ns/op (5 000)

N = 1 000 000
  Insert   :      1479.81 ms |        1479.81 ns/op (1 000 000 / 1 000 000)
  Top      :       241.95 ms |          48.39 ns/op (5 000 000 iters)
  Pop      :      9579.44 ms |       19158.87 ns/op (500 000)
  DeleteKey:        53.25 ms |       10649.40 ns/op (5 000)

TABLEAU RÉCAPITULATIF (ns/op)
-----------------------------
N                     insert           top             pop       deleteKey
--------------------------------------------------------------------------
100                   460.40          6.63         1431.06         1002.77
1 000                 632.48         11.21         1640.60         1876.17
10 000                533.95         13.35         3548.13         7956.91
100 000               979.11         29.89         7297.13         8772.63
1 000 000            1479.81         48.39        19158.87        10649.40

RATIOS ns/op entre N et N/10
-----------------------------
Attendu MaxHeapMap : top ~log(N) (≈1.2-1.5 entre N et N/10)
                     insert/pop ~log^2 => ratio ≈ 1.5-2
                     deleteKey ~O(N) => ratio ≈ 10

N                         insert           top           pop     deleteKey
--------------------------------------------------------------------------
1 000 / 100                 1.37          1.69          1.15          1.87
10 000 / 1 000              0.84          1.19          2.16          4.24
100 000 / 10 000            1.83          2.24          2.06          1.10
1 000 000 / 100 000          1.51          1.62          2.63          1.21

Terminé.
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ 
*/