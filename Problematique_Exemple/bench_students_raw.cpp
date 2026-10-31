// bench_students_raw.cpp — MaxHeapRaw sur Student (boursiers)
// Compilation :
//   g++ -std=c++17 -O2 -Wall -Wextra -o bench_students_raw bench_students_raw.cpp
// Exécution :
//   ./bench_students_raw
//
// Problème : N = 1 000 000 étudiants, extraire toujours le plus boursier.
// Opérations mesurées : insert / top / pop (extract-max) / deleteKey (retirer un id).

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
static const std::size_t N            = 1'000'000; // taille du jeu de données
static const std::size_t DEL_COUNT    = 10'000;    // nb de deleteKey mesurés
static const std::size_t TOP_ITERS    = 50'000'000;// itérations de top()
static const unsigned    SEED         = 42;
// ============================================================

// ---------- Domaine métier ----------
struct Student {
    int         id;
    std::string name;
    double      scholarship; // montant de la bourse en €

    // Max-heap : on veut que le plus boursier soit "le plus grand".
    // Départage par id si montants égaux (stable, évite les == ambigus).
    bool operator<(const Student& o) const {
        if (scholarship != o.scholarship) return scholarship < o.scholarship;
        return id < o.id;
    }
    bool operator==(const Student& o) const { return id == o.id; }
};

static std::ostream& operator<<(std::ostream& os, const Student& s) {
    return os << "Student{id=" << s.id
              << ", name=\"" << s.name << "\""
              << ", scholarship=" << s.scholarship << "€}";
}

// ---------- MaxHeapRaw (tableau brut) ----------
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
        while (true) {
            int l = 2*i+1, r = 2*i+2, largest = i;
            // Correction : utiliser '<' (convention C++17, pas de synthèse de '>')
            if (l < sz && buf[largest] < buf[l]) largest = l;
            if (r < sz && buf[largest] < buf[r]) largest = r;
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
        T* nb = new T[n];
        for (int i = 0; i < sz; ++i) nb[i] = buf[i];
        delete[] buf; buf = nb; cap = n;
    }
    void insert(const T& v) {
        if (sz == cap) grow();
        buf[sz++] = v;
        int i = sz - 1;
        while (i != 0 && buf[(i-1)/2] < buf[i]) {
            T t = buf[i]; buf[i] = buf[(i-1)/2]; buf[(i-1)/2] = t;
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
            T t = buf[i]; buf[i] = buf[(i-1)/2]; buf[(i-1)/2] = t;
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

    // ---- Génération (hors mesure) ----
    std::vector<Student> data(N);
    for (std::size_t i = 0; i < N; ++i) {
        data[i].id          = (int)i;
        data[i].name        = "etu" + std::to_string(dist_name(rng));
        data[i].scholarship = dist_sch(rng);
    }

    std::cout << "===============================================================\n";
    std::cout << "  [RAW]   Étudiants boursiers — N = " << fmt_u(N) << "\n";
    std::cout << "  insert / top (le + boursier) / pop (= extract-max) / deleteKey\n";
    std::cout << "===============================================================\n\n";

    MaxHeapRaw<Student> heap;
    heap.reserve((int)N);

    // ---- 1) INSERT ----
    auto t0 = Clock::now();
    for (std::size_t i = 0; i < N; ++i) heap.insert(data[i]);
    auto t1 = Clock::now();
    double ins_ms = ms(t0, t1);
    double ins_ns = ins_ms * 1e6 / (double)N;
    std::cout << "Insert   : " << std::setw(12) << std::fixed << std::setprecision(2)
              << ins_ms << " ms | " << ins_ns << " ns/op\n";

    // ---- 2) TOP ----
    volatile long long sink = 0;
    t0 = Clock::now();
    for (std::size_t i = 0; i < TOP_ITERS; ++i) sink += (long long)heap.top().id;
    t1 = Clock::now();
    double top_ms = ms(t0, t1);
    double top_ns = top_ms * 1e6 / (double)TOP_ITERS;
    std::cout << "Top x" << fmt_u(TOP_ITERS) << " : " << std::setw(12) << top_ms
              << " ms | " << top_ns << " ns/op\n";

    // ---- 3) POP (extraire le plus boursier, moitié) ----
    std::size_t half = heap.size() / 2;
    double sum_pop = 0.0;
    t0 = Clock::now();
    for (std::size_t i = 0; i < half; ++i) {
        Student s = heap.pop();
        sum_pop += s.scholarship;
    }
    t1 = Clock::now();
    double pop_ms = ms(t0, t1);
    double pop_ns = pop_ms * 1e6 / (double)half;
    std::cout << "Pop " << fmt_u(half) << " : " << std::setw(12) << pop_ms
              << " ms | " << pop_ns << " ns/op   (somme bourses extraites = "
              << std::fixed << std::setprecision(0) << sum_pop << " €)\n";

    // ---- 4) DELETE KEY (retirer DEL_COUNT étudiants par id) ----
    std::vector<int> ids_to_delete(DEL_COUNT);
    for (std::size_t i = 0; i < DEL_COUNT; ++i) ids_to_delete[i] = (int)i;
    std::shuffle(ids_to_delete.begin(), ids_to_delete.end(), rng);

    t0 = Clock::now();
    std::size_t del_ok = 0;
    for (int id : ids_to_delete) {
        Student probe; probe.id = id;
        if (heap.deleteKey(probe)) ++del_ok;
    }
    t1 = Clock::now();
    double del_ms = ms(t0, t1);
    double del_ns = del_ms * 1e6 / (double)DEL_COUNT;
    std::cout << "DeleteKey x" << fmt_u(DEL_COUNT) << " : " << std::setw(12) << del_ms
              << " ms | " << del_ns << " ns/op   (" << del_ok << " supprimés)\n\n";

    // ---- Tableau final ----
    std::cout << "----------------------------- RÉCAP -----------------------------\n";
    std::cout << std::left  << std::setw(14) << "Opération"
              << std::right << std::setw(16) << "total (ms)"
              << std::setw(14) << "ns/op"
              << std::setw(20) << "complexité" << "\n";
    std::cout << std::string(64, '-') << "\n";
    std::cout << std::left  << std::setw(14) << "insert"
              << std::right << std::setw(16) << ins_ms
              << std::setw(14) << ins_ns << std::setw(20) << "O(log N)" << "\n";
    std::cout << std::left  << std::setw(14) << "top"
              << std::right << std::setw(16) << top_ms
              << std::setw(14) << top_ns << std::setw(20) << "O(1)" << "\n";
    std::cout << std::left  << std::setw(14) << "pop (extract)"
              << std::right << std::setw(16) << pop_ms
              << std::setw(14) << pop_ns << std::setw(20) << "O(log N)" << "\n";
    std::cout << std::left  << std::setw(14) << "deleteKey"
              << std::right << std::setw(16) << del_ms
              << std::setw(14) << del_ns << std::setw(20) << "O(N)" << "\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "Sink = " << sink << " (empêche l'optimisation)\n";
    // Utilisation de operator<< : supprime le warning -Wunused-function
    std::cout << "Plus boursier restant : " << heap.top() << "\n";
    return 0;
}
/*
Terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ ./bench_students_raw
===============================================================
  [RAW]   Étudiants boursiers — N = 1 000 000
  insert / top (le + boursier) / pop (= extract-max) / deleteKey
===============================================================

Insert   :        66.76 ms | 66.76 ns/op
Top x50 000 000 :        25.78 ms | 0.52 ns/op
Pop 500 000 :       492.19 ms | 984.39 ns/op   (somme bourses extraites = 3751198580 €)
DeleteKey x10 000 :         9354 ms | 935421 ns/op   (5016 supprimés)

----------------------------- RÉCAP -----------------------------
Opération          total (ms)         ns/op         complexité
----------------------------------------------------------------
insert                      67            67            O(log N)
top                         26             1                O(1)
pop (extract)              492           984            O(log N)
deleteKey                 9354        935421                O(N)
----------------------------------------------------------------
Sink = 35075800000000 (empêche l'optimisation)
Plus boursier restant : Student{id=757051, name="etu6826", scholarship=5000€}
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS/Problematique_Exemple$ 

*/