// heap_vector.cpp — Max Heap avec std::vector (implémentation de référence)
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o heap_vector heap_vector.cpp

#include <iostream>
#include <vector>
#include <stdexcept>

template <typename T>
class MaxHeapVector {
    std::vector<T> arr;

    void heapify(int i) {
        int n = (int)arr.size();
        int l = 2*i + 1, r = 2*i + 2, largest = i;
        if (l < n && arr[l] > arr[largest]) largest = l;
        if (r < n && arr[r] > arr[largest]) largest = r;
        if (largest != i) {
            std::swap(arr[i], arr[largest]);
            heapify(largest);
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

    void print(std::ostream& os = std::cout) const {
        os << "MaxHeapVector: [ ";
        for (const auto& v : arr) os << v << ' ';
        os << "]\n";
    }
};

int main() {
    MaxHeapVector<int> h;
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
Terminal resultat:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./heap_vector
MaxHeapVector: [ 102 88 100 7 2 10 99 1 ]
top = 102
pop = 102
MaxHeapVector: [ 100 88 99 7 2 10 1 ]
MaxHeapVector: [ 100 88 10 7 2 1 ]
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$

*/