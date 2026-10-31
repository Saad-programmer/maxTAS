// bench_maxheap.cpp
// Benchmark : insert / pop / top / deleteKey sur max heap (vector)
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o bench_maxheap bench_maxheap.cpp

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <string>

// ---------- MaxHeap minimal (même logique que maxheap_simple) ----------
template <typename T>
class MaxHeap {
    std::vector<T> arr;
    void heapify(int i) {
        int n = (int)arr.size();
        int l = 2*i+1, r = 2*i+2, largest = i;
        if (l < n && arr[l] > arr[largest]) largest = l;
        if (r < n && arr[r] > arr[largest]) largest = r;
        if (largest != i) { std::swap(arr[i], arr[largest]); heapify(largest); }
    }
public:
    void reserve(std::size_t n) { arr.reserve(n); }
    void insert(const T& v) {
        arr.push_back(v);
        int i = (int)arr.size() - 1;
        while (i != 0 && arr[(i-1)/2] < arr[i]) {
            std::swap(arr[i], arr[(i-1)/2]);
            i = (i-1)/2;
        }
    }
    T pop() {
        T root = arr[0];
        arr[0] = arr.back(); arr.pop_back();
        if (!arr.empty()) heapify(0);
        return root;
    }
    const T& top() const { return arr[0]; }
    int size() const { return (int)arr.size(); }
    bool empty() const { return arr.empty(); }
    bool deleteKey(const T& k) {
        int n = (int)arr.size(), idx = -1;
        for (int i = 0; i < n; ++i) if (arr[i] == k) { idx = i; break; }
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

int main(int argc, char** argv) {
    std::size_t N = (argc > 1) ? std::stoull(argv[1]) : 1'000'000;

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int> dist(0, 10'000'000);

    std::vector<int> data(N);
    for (auto& x : data) x = dist(rng);

    std::cout << "=== Benchmark MaxHeap (N = " << N << ") ===\n";

    // 1) INSERT
    MaxHeap<int> h; h.reserve(N);
    auto t0 = Clock::now();
    for (int v : data) h.insert(v);
    auto t1 = Clock::now();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Insert total : " << ms(t0,t1) << " ms ("
              << ms(t0,t1)/N*1e6 << " ns/op)\n";

    // 2) TOP (répété 10M fois pour mesurer)
    const int TOP_ITERS = 10'000'000;
    t0 = Clock::now();
    volatile long long sink = 0;
    for (int i = 0; i < TOP_ITERS; ++i) sink += h.top();
    t1 = Clock::now();
    std::cout << "Top x" << TOP_ITERS << " : " << ms(t0,t1) << " ms ("
              << ms(t0,t1)/TOP_ITERS*1e6 << " ns/op)\n";

    // 3) POP (vide la moitié)
    std::size_t half = h.size() / 2;
    t0 = Clock::now();
    for (std::size_t i = 0; i < half; ++i) sink += h.pop();
    t1 = Clock::now();
    std::cout << "Pop " << half << " éléments : " << ms(t0,t1) << " ms ("
              << ms(t0,t1)/half*1e6 << " ns/op)\n";

    // 4) DELETE KEY (1000 suppressions aléatoires)
    std::uniform_int_distribution<int> pick(0, (int)h.size()-1);
    // On ne connait pas la position -> on tire une valeur via un tableau temporaire
    // (simplification : on supprime des valeurs présentes)
    MaxHeap<int> h2; h2.reserve(N/4);
    for (std::size_t i = 0; i < N/4; ++i) h2.insert(data[i]);

    std::vector<int> toDelete;
    toDelete.reserve(1000);
    // On récupère 1000 valeurs existantes en les pioquant par pop puis re-insertion
    // (ceci est un artefact de mesure, ne pas reproduire en prod)
    // Alternative simple : deleteKey sur des valeurs injectées connues
    MaxHeap<int> h3; h3.reserve(1000);
    std::vector<int> keys;
    for (int i = 0; i < 1000; ++i) { h3.insert(i); keys.push_back(i); }
    std::shuffle(keys.begin(), keys.end(), rng);

    t0 = Clock::now();
    for (int k : keys) h3.deleteKey(k);
    t1 = Clock::now();
    std::cout << "DeleteKey x1000 : " << ms(t0,t1) << " ms\n";

    // 5) BUILD (vector -> heap en O(n) implicite via insert=O(n log n) déjà mesuré)
    // On mesure aussi buildHeap explicite pour comparaison
    // (non inclus ici par souci de concision, voir maxheap_simple pour buildHeap)

    std::cout << "Sink = " << sink << " (empêche l'optimisation)\n";
    return 0;
}

/*
Terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./bench_maxheap
=== Benchmark MaxHeap (N = 1000000) ===
Insert total : 85.42 ms (85.42 ns/op)
Top x10000000 : 88.80 ms (8.88 ns/op)
Pop 500000 éléments : 500.60 ms (1001.20 ns/op)
DeleteKey x1000 : 1.49 ms
Sink = 103751644997586 (empêche l'optimisation)
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ 



*/