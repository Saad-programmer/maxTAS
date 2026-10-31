// bench_students_map.cpp — MaxHeapMap sur Student (boursiers)
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_map bench_students_map.cpp
// Exécution   : ./bench_students_map
//
// ATTENTION : MaxHeapMap = O(log² N) par insert/pop. À N = 1e6, l'insert complet
// prend plusieurs secondes, mais reste faisable (~5-15 s selon machine).

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

// ============================================================
static const std::size_t N         = 1'000'000;
static const std::size_t DEL_COUNT = 5'000;      // deleteKey O(N) → cap plus bas
static const std::size_t TOP_ITERS = 5'000'000;  // top O(log N) → moins d'iters
static const unsigned    SEED      = 42;
// ============================================================

struct Student {
    int         id;
    std::string name;
    double      scholarship;
    bool operator<(const Student& o) const {
        if (scholarship != o.scholarship) return scholarship < o.scholarship;
        return id < o.id;
    }
    bool operator==(const Student& o) const { return id == o.id; }
};

// ---------- MaxHeapMap ----------
template <typename T>
class MaxHeapMap {
    std::map<int, T> tree;
    T&       ref(int i)       { return tree.at(i); }
    const T& ref(int i) const { return tree.at(i); }
    void heapify(int i) {
        while (true) {
            int n = (int)tree.size();
            int l = 2*i+1, r = 2*i+2, largest = i;
            // Correction : '<' au lieu de '>'
            if (l < n && ref(largest) < ref(l)) largest = l;
            if (r < n && ref(largest) < ref(r)) largest = r;
            if (largest == i) break;
            std::swap(ref(i), ref(largest));
            i = largest;
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
            // Correction : !(a < b) équivaut à (a >= b) avec seulement '<'
            if (!(ref(p) < ref(i))) break;
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
        for (const auto& kv : tree) if (kv.second == key) { idx = kv.first; break; }
        if (idx == -1) return false;
        int last = (int)tree.size() - 1;
        if (idx == last) { tree.erase(last); return true; }
        tree[idx] = tree.at(last);
        tree.erase(last);
        int i = idx;
        while (i != 0) {
            int p = (i-1)/2;
            // Correction : '<' au lieu de '>='
            if (!(ref(p) < ref(i))) break;
            std::swap(ref(i), ref(p)); i = p;
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
    std::string s = std::to_string(v), out; int c = 0;
    for (int i = (int)s.size()-1; i >= 0; --i) {
        out.push_back(s[i]);
        if (++c == 3 && i != 0) { out.push_back(' '); c = 0; }
    }
    std::reverse(out.begin(), out.end());
    return out;
}

int main() {
    std::mt19937_64 rng(SEED);
    std::uniform_int_distribution<int>    dist_name(0, 9999);
    std::uniform_real_distribution<double> dist_sch(0.0, 10'000.0);

    std::vector<Student> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        data[i].id          = (int)i;
        data[i].name        = "etu" + std::to_string(dist_name(rng));
        data[i].scholarship = dist_sch(rng);
    }

    std::cout << "===============================================================\n";
    std::cout << "  [MAP]   Étudiants boursiers — N = " << fmt_u(N) << "\n";
    std::cout << "  /!\\ O(log^2 N) par insert/pop — plus lent\n";
    std::cout << "===============================================================\n\n";

    MaxHeapMap<Student> heap;

    auto t0 = Clock::now();
    for (std::size_t i = 0; i < N; ++i) heap.insert(data[i]);
    auto t1 = Clock::now();
    double ins_ms = ms(t0, t1);
    double ins_ns = ins_ms * 1e6 / (double)N;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Insert   : " << std::setw(12) << ins_ms << " ms | " << ins_ns << " ns/op\n";

    volatile long long sink = 0;
    t0 = Clock::now();
    for (std::size_t i = 0; i < TOP_ITERS; ++i) sink += (long long)heap.top().id;
    t1 = Clock::now();
    double top_ms = ms(t0, t1);
    double top_ns = top_ms * 1e6 / (double)TOP_ITERS;
    std::cout << "Top x" << fmt_u(TOP_ITERS) << " : " << std::setw(12) << top_ms
              << " ms | " << top_ns << " ns/op\n";

    std::size_t half = heap.size() / 2;
    double sum_pop = 0.0;
    t0 = Clock::now();
    for (std::size_t i = 0; i < half; ++i) sum_pop += heap.pop().scholarship;
    t1 = Clock::now();
    double pop_ms = ms(t0, t1);
    double pop_ns = pop_ms * 1e6 / (double)half;
    std::cout << "Pop " << fmt_u(half) << " : " << std::setw(12) << pop_ms
              << " ms | " << pop_ns << " ns/op   (somme = "
              << std::setprecision(0) << sum_pop << " €)\n";

    std::vector<int> ids_to_delete(DEL_COUNT);
    for (std::size_t i = 0; i < DEL_COUNT; ++i) ids_to_delete[i] = (int)i;
    std::shuffle(ids_to_delete.begin(), ids_to_delete.end(), rng);

    t0 = Clock::now();
    std::size_t del_ok = 0;
    for (int id : ids_to_delete) { Student p; p.id = id; if (heap.deleteKey(p)) ++del_ok; }
    t1 = Clock::now();
    double del_ms = ms(t0, t1);
    double del_ns = del_ms * 1e6 / (double)DEL_COUNT;
    std::cout << "DeleteKey x" << fmt_u(DEL_COUNT) << " : " << std::setw(12) << del_ms
              << " ms | " << del_ns << " ns/op   (" << del_ok << " supprimés)\n\n";

    std::cout << "----------------------------- RÉCAP -----------------------------\n";
    std::cout << std::left  << std::setw(14) << "Opération"
              << std::right << std::setw(16) << "total (ms)"
              << std::setw(14) << "ns/op"
              << std::setw(20) << "complexité" << "\n";
    std::cout << std::string(64, '-') << "\n";
    std::cout << std::left  << std::setw(14) << "insert"       << std::right
              << std::setw(16) << ins_ms << std::setw(14) << ins_ns << std::setw(20) << "O(log^2 N)" << "\n";
    std::cout << std::left  << std::setw(14) << "top"          << std::right
              << std::setw(16) << top_ms << std::setw(14) << top_ns << std::setw(20) << "O(log N)"   << "\n";
    std::cout << std::left  << std::setw(14) << "pop (extract)"<< std::right
              << std::setw(16) << pop_ms << std::setw(14) << pop_ns << std::setw(20) << "O(log^2 N)" << "\n";
    std::cout << std::left  << std::setw(14) << "deleteKey"    << std::right
              << std::setw(16) << del_ms << std::setw(14) << del_ns << std::setw(20) << "O(N)"       << "\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "Sink = " << sink << "\n";
    std::cout << "Plus boursier restant : " << heap.top().scholarship << " € (id "
              << heap.top().id << ")\n";
    return 0;
}

/*
Terminal Result sur ma machine:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ ./bench_students_map
===============================================================
  [MAP]   Étudiants boursiers — N = 1 000 000
  /!\ O(log^2 N) par insert/pop — plus lent
===============================================================

Insert   :      1380.65 ms | 1380.65 ns/op
Top x5 000 000 :       225.68 ms | 45.14 ns/op
Pop 500 000 :     10689.63 ms | 21379.27 ns/op   (somme = 3751198580 €)
DeleteKey x5 000 :        34953 ms | 6990642 ns/op   (2492 supprimés)

----------------------------- RÉCAP -----------------------------
Opération          total (ms)         ns/op         complexité
----------------------------------------------------------------
insert                    1381          1381          O(log^2 N)
top                        226            45            O(log N)
pop (extract)            10690         21379          O(log^2 N)
deleteKey                34953       6990642                O(N)
----------------------------------------------------------------
Sink = 3507580000000
Plus boursier restant : 5000 € (id 757051)
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ 
*/