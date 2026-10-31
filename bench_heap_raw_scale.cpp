// bench_heap_raw_scale.cpp
// Benchmark MaxHeapRaw (tableau brut new/delete) — self-contained, AUCUN argument CLI.
//
// Compilation :
//   g++ -std=c++17 -O2 -Wall -Wextra -o bench_heap_raw_scale bench_heap_raw_scale.cpp
// Exécution :
//   ./bench_heap_raw_scale
//
// Règlages : constantes SIZES / BUDGET_MS / DEL_CAP / TOP_ITERS ci-dessous.
//
// Complexités :
//   insert    : O(log N) amorti (grow ×2 → O(1) amorti)
//   pop       : O(log N)
//   top       : O(1)
//   deleteKey : O(N) (recherche linéaire) + O(log N) remontée/descente
//   => on peut monter à 1e8 (prévoir ~1 Go de RAM : data + buffer).

#include <iostream>
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
    100ULL, 1'000ULL, 10'000ULL, 100'000ULL,
    1'000'000ULL, 10'000'000ULL, 100'000'000ULL
};
static const double BUDGET_MS = 30000.0;   // budget max par phase (ms)
static const std::size_t DEL_CAP = 10'000; // deleteKey : cap du nombre d'ops
static const std::size_t TOP_ITERS = 50'000'000; // itérations pour top()
static const unsigned SEED = 42;
// ============================================================

// ---------- MaxHeapRaw (copie minimale de heap_raw.cpp) ----------
template <typename T>
class MaxHeapRaw {
    T*   buf;
    int  sz;
    int  cap;

    void grow() {
        int newCap = (cap == 0) ? 8 : cap * 2;
        T* newBuf = new T[newCap];
        for (int i = 0; i < sz; ++i) newBuf[i] = buf[i];
        delete[] buf;
        buf = newBuf;
        cap = newCap;
    }
    void heapify(int i) {
        // Version itérative pour éviter la récursion profonde (sécurité à 1e8)
        while (true) {
            int l = 2*i+1, r = 2*i+2, largest = i;
            if (l < sz && buf[l] > buf[largest]) largest = l;
            if (r < sz && buf[r] > buf[largest]) largest = r;
            if (largest == i) break;
            T tmp = buf[i]; buf[i] = buf[largest]; buf[largest] = tmp;
            i = largest;
        }
    }
public:
    MaxHeapRaw() : buf(nullptr), sz(0), cap(0) {}
    MaxHeapRaw(const MaxHeapRaw&) = delete;
    MaxHeapRaw& operator=(const MaxHeapRaw&) = delete;
    ~MaxHeapRaw() { delete[] buf; }

    bool empty() const { return sz == 0; }
    int  size()  const { return sz; }

    void reserve(int n) {
        if (n <= cap) return;
        T* newBuf = new T[n];
        for (int i = 0; i < sz; ++i) newBuf[i] = buf[i];
        delete[] buf;
        buf = newBuf;
        cap = n;
    }

    void insert(const T& v) {
        if (sz == cap) grow();
        buf[sz++] = v;
        int i = sz - 1;
        while (i != 0 && buf[(i-1)/2] < buf[i]) {
            T tmp = buf[i]; buf[i] = buf[(i-1)/2]; buf[(i-1)/2] = tmp;
            i = (i-1)/2;
        }
    }
    const T& top() const {
        if (sz == 0) throw std::runtime_error("top() sur heap vide");
        return buf[0];
    }
    T pop() {
        if (sz == 0) throw std::runtime_error("pop() sur heap vide");
        T root = buf[0];
        buf[0] = buf[--sz];
        if (sz > 0) heapify(0);
        return root;
    }
    bool deleteKey(const T& key) {
        int idx = -1;
        for (int i = 0; i < sz; ++i) if (buf[i] == key) { idx = i; break; }
        if (idx == -1) return false;
        if (idx == sz-1) { --sz; return true; }
        buf[idx] = buf[--sz];
        int i = idx;
        while (i != 0 && buf[(i-1)/2] < buf[i]) {
            T tmp = buf[i]; buf[i] = buf[(i-1)/2]; buf[(i-1)/2] = tmp;
            i = (i-1)/2;
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
    MaxHeapRaw<int> h;
    h.reserve((int)N); // évite les realloc : on mesure le vrai insert, pas grow()
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
        MaxHeapRaw<int> hd;
        hd.reserve((int)del_count);
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
    std::cout << "  Benchmark MaxHeapRaw (tableau brut new/delete) — self-contained\n";
    std::cout << "  Budget/phase = " << BUDGET_MS << " ms | DEL_CAP = " << DEL_CAP
              << " | TOP_ITERS = " << fmt_u(TOP_ITERS) << "\n";
    std::cout << "  /!\\ insert/pop = O(log N), top = O(1), deleteKey = O(N)\n";
    std::cout << "  /!\\ N = 1e8 : ~1 Go de RAM (data + buffer)\n";
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
    std::cout << "Attendu MaxHeapRaw : top ~1 (O(1)) | insert/pop ~1.1-1.3 (log N) |\n";
    std::cout << "                     deleteKey ~10 (O(N))\n\n";
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
Terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./bench_heap_raw_scale
================================================================
  Benchmark MaxHeapRaw (tableau brut new/delete) — self-contained
  Budget/phase = 30000 ms | DEL_CAP = 10000 | TOP_ITERS = 50 000 000
  /!\ insert/pop = O(log N), top = O(1), deleteKey = O(N)
  /!\ N = 1e8 : ~1 Go de RAM (data + buffer)
================================================================

>>> Exécution N = 100 ... OK
>>> Exécution N = 1 000 ... OK
>>> Exécution N = 10 000 ... OK
>>> Exécution N = 100 000 ... OK
>>> Exécution N = 1 000 000 ... OK
>>> Exécution N = 10 000 000 ... OK
>>> Exécution N = 100 000 000 ... OK

RÉSULTATS DÉTAILLÉS
-------------------

N = 100
  Insert   :         0.00 ms |          21.36 ns/op (100 / 100)
  Top      :        27.79 ms |           0.56 ns/op (50 000 000 iters)
  Pop      :         0.01 ms |         130.78 ns/op (50)
  DeleteKey:         0.01 ms |         132.40 ns/op (100)

N = 1 000
  Insert   :         0.03 ms |          27.96 ns/op (1 000 / 1 000)
  Top      :        32.32 ms |           0.65 ns/op (50 000 000 iters)
  Pop      :         0.05 ms |          94.29 ns/op (500)
  DeleteKey:         0.21 ms |         214.35 ns/op (1 000)

N = 10 000
  Insert   :         0.22 ms |          21.98 ns/op (10 000 / 10 000)
  Top      :        24.75 ms |           0.49 ns/op (50 000 000 iters)
  Pop      :         0.62 ms |         123.93 ns/op (5 000)
  DeleteKey:        11.28 ms |        1128.41 ns/op (10 000)

N = 100 000
  Insert   :         2.81 ms |          28.06 ns/op (100 000 / 100 000)
  Top      :        27.26 ms |           0.55 ns/op (50 000 000 iters)
  Pop      :         7.31 ms |         146.14 ns/op (50 000)
  DeleteKey:        11.16 ms |        1116.44 ns/op (10 000)

N = 1 000 000
  Insert   :        22.14 ms |          22.14 ns/op (1 000 000 / 1 000 000)
  Top      :        26.51 ms |           0.53 ns/op (50 000 000 iters)
  Pop      :        88.42 ms |         176.83 ns/op (500 000)
  DeleteKey:         9.60 ms |         960.50 ns/op (10 000)

N = 10 000 000
  Insert   :       176.05 ms |          17.61 ns/op (10 000 000 / 10 000 000)
  Top      :        18.89 ms |           0.38 ns/op (50 000 000 iters)
  Pop      :      1765.75 ms |         353.15 ns/op (5 000 000)
  DeleteKey:        15.26 ms |        1525.86 ns/op (10 000)

N = 100 000 000
  Insert   :      2143.14 ms |          21.43 ns/op (100 000 000 / 100 000 000)
  Top      :        22.16 ms |           0.44 ns/op (50 000 000 iters)
  Pop      :     26554.19 ms |         531.08 ns/op (50 000 000)
  DeleteKey:         9.26 ms |         926.10 ns/op (10 000)

TABLEAU RÉCAPITULATIF (ns/op)
-----------------------------
N                     insert           top             pop       deleteKey
--------------------------------------------------------------------------
100                    21.36          0.56          130.78          132.40
1 000                  27.96          0.65           94.29          214.35
10 000                 21.98          0.49          123.93         1128.41
100 000                28.06          0.55          146.14         1116.44
1 000 000              22.14          0.53          176.83          960.50
10 000 000             17.61          0.38          353.15         1525.86
100 000 000            21.43          0.44          531.08          926.10

RATIOS ns/op entre N et N/10
-----------------------------
Attendu MaxHeapRaw : top ~1 (O(1)) | insert/pop ~1.1-1.3 (log N) |
                     deleteKey ~10 (O(N))

N                         insert           top           pop     deleteKey
--------------------------------------------------------------------------
1 000 / 100                 1.31          1.16          0.72          1.62
10 000 / 1 000              0.79          0.77          1.31          5.26
100 000 / 10 000            1.28          1.10          1.18          0.99
1 000 000 / 100 000          0.79          0.97          1.21          0.86
10 000 000 / 1 000 000          0.80          0.71          2.00          1.59
100 000 000 / 10 000 000          1.22          1.17          1.50          0.61

Terminé.
*/