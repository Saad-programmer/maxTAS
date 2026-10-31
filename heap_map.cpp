// heap_map.cpp — Max Heap avec std::map<int, T> (clé = index dans le tableau virtuel)
// Justification : std::map fournit un accès par clé en O(log n).
// On stocke le heap comme un tableau indexé 0..n-1 dans une map.
// Résultat : insert/pop en O(log² n) (O(log n) par accès × O(log n) niveaux).
// Moins rapide que vector mais conserve une complexité logarithmique.
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o heap_map heap_map.cpp

#include <iostream>
#include <map>
#include <stdexcept>

template <typename T>
class MaxHeapMap {
    std::map<int, T> tree;   // index -> valeur

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

    void print(std::ostream& os = std::cout) const {
        os << "MaxHeapMap: [ ";
        for (const auto& kv : tree) os << kv.second << ' ';
        os << "]\n";
    }
};

int main() {
    MaxHeapMap<int> h;
    for (int v : {88, 1, 10, 2, 99, 100, 102, 7}) h.insert(v);
    h.print();
    std::cout << "top = " << h.top() << "\n";
    std::cout << "pop = " << h.pop() << "\n";
    h.print();
    h.deleteKey(99);
    h.print();
    return 0;
}

/*
terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./heap_map
MaxHeapMap: [ 102 88 100 7 2 10 99 1 ]
top = 102
pop = 102
MaxHeapMap: [ 100 88 99 7 2 10 1 ]
MaxHeapMap: [ 100 88 10 7 2 1 ]
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ 


*/