// bench_students_vector.cpp — MaxHeapVector sur Student (boursiers)
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_vector bench_students_vector.cpp
// Exécution   : ./bench_students_vector
//
// Identique au RAW mais avec std::vector<Student> comme stockage.

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

// ============================================================
static const std::size_t N            = 1'000'000;
static const std::size_t DEL_COUNT    = 10'000;
static const std::size_t TOP_ITERS    = 50'000'000;
static const unsigned    SEED         = 42;
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

// ---------- MaxHeapVector ----------
template <typename T>
class MaxHeapVector {
    std::vector<T> arr;
    void heapify(int i) {
        while (true) {
            int n = (int)arr.size();
            int l = 2*i+1, r = 2*i+2, largest = i;
            // Correction : utiliser '<' (convention C++17)
            if (l < n && arr[largest] < arr[l]) largest = l;
            if (r < n && arr[largest] < arr[r]) largest = r;
            if (largest == i) break;
            std::swap(arr[i], arr[largest]);
            i = largest;
        }
    }
public:
    void reserve(std::size_t n) { arr.reserve(n); }
    bool empty() const { return arr.empty(); }
    int  size()  const { return (int)arr.size(); }
    void insert(const T& v) {
        arr.push_back(v);
        int i = (int)arr.size() - 1;
        while (i != 0 && arr[(i-1)/2] < arr[i]) {
            std::swap(arr[i], arr[(i-1)/2]);
            i = (i-1)/2;
        }
    }
    const T& top() const {
        if (arr.empty()) throw std::runtime_error("top() sur heap vide");
        return arr[0];
    }
    T pop() {
        if (arr.empty()) throw std::runtime_error("pop() sur heap vide");
        T root = arr[0];
        arr[0] = arr.back();
        arr.pop_back();
        if (!arr.empty()) heapify(0);
        return root;
    }
    bool deleteKey(const T& key) {
        int n = (int)arr.size(), idx = -1;
        for (int i = 0; i < n; ++i) if (arr[i] == key) { idx = i; break; }
        if (idx == -1) return false;
        if (idx == n-1) { arr.pop_back(); return true; }
        arr[idx] = arr.back(); arr.pop_back();
        int i = idx;
        while (i != 0 && arr[(i-1)/2] < arr[i]) {
            std::swap(arr[i], arr[(i-1)/2]); i = (i-1)/2;
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
    std::uniform_int_distribution<int>    dist_id(0, 10'000'000);
    std::uniform_real_distribution<double> dist_sch(0.0, 10'000.0);
    std::uniform_int_distribution<int>    dist_name(0, 9999);

    std::vector<Student> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        data[i].id          = (int)i;
        data[i].name        = "etu" + std::to_string(dist_name(rng));
        data[i].scholarship = dist_sch(rng);
    }

    std::cout << "===============================================================\n";
    std::cout << "  [VECTOR]   Étudiants boursiers — N = " << fmt_u(N) << "\n";
    std::cout << "===============================================================\n\n";

    MaxHeapVector<Student> heap;
    heap.reserve(N);

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
    std::cout << std::left  << std::setw(14) << "insert"      << std::right
              << std::setw(16) << ins_ms << std::setw(14) << ins_ns << std::setw(20) << "O(log N)" << "\n";
    std::cout << std::left  << std::setw(14) << "top"         << std::right
              << std::setw(16) << top_ms << std::setw(14) << top_ns << std::setw(20) << "O(1)"     << "\n";
    std::cout << std::left  << std::setw(14) << "pop (extract)"<< std::right
              << std::setw(16) << pop_ms << std::setw(14) << pop_ns << std::setw(20) << "O(log N)" << "\n";
    std::cout << std::left  << std::setw(14) << "deleteKey"   << std::right
              << std::setw(16) << del_ms << std::setw(14) << del_ns << std::setw(20) << "O(N)"     << "\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "Sink = " << sink << "\n";
    std::cout << "Plus boursier restant : " << heap.top().scholarship << " € (id "
              << heap.top().id << ")\n";
    return 0;
}

/*
Terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ ./bench_students_vector
===============================================================
  [VECTOR]   Étudiants boursiers — N = 1 000 000
===============================================================

Insert   :        73.27 ms | 73.27 ns/op
Top x50 000 000 :        28.34 ms | 0.57 ns/op
Pop 500 000 :       382.34 ms | 764.68 ns/op   (somme = 3751198580 €)
DeleteKey x10 000 :         9560 ms | 955957 ns/op   (5016 supprimés)

----------------------------- RÉCAP -----------------------------
Opération          total (ms)         ns/op         complexité
----------------------------------------------------------------
insert                      73            73            O(log N)
top                         28             1                O(1)
pop (extract)              382           765            O(log N)
deleteKey                 9560        955957                O(N)
----------------------------------------------------------------
Sink = 35075800000000
Plus boursier restant : 5000 € (id 757051)
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ 
*/