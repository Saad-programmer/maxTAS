// heap_raw.cpp — Max Heap sans STL : tableau dynamique géré à la main (new/delete)
// Compilation : g++ -std=c++17 -O2 -Wall -Wextra -o heap_raw heap_raw.cpp

#include <iostream>
#include <stdexcept>

template <typename T>
class MaxHeapRaw {
    T*   buf;        // tableau dynamique brut
    int  sz;         // taille actuelle
    int  cap;        // capacité allouée

    void grow() {
        int newCap = (cap == 0) ? 8 : cap * 2;
        T* newBuf = new T[newCap];
        for (int i = 0; i < sz; ++i) newBuf[i] = buf[i];
        delete[] buf;
        buf = newBuf;
        cap = newCap;
    }

    void heapify(int i) {
        int l = 2*i+1, r = 2*i+2, largest = i;
        if (l < sz && buf[l] > buf[largest]) largest = l;
        if (r < sz && buf[r] > buf[largest]) largest = r;
        if (largest != i) {
            T tmp = buf[i]; buf[i] = buf[largest]; buf[largest] = tmp;
            heapify(largest);
        }
    }

public:
    MaxHeapRaw() : buf(nullptr), sz(0), cap(0) {}

    // Interdiction de la copie par simplicité (règle des 3/5 non traitée ici)
    MaxHeapRaw(const MaxHeapRaw&) = delete;
    MaxHeapRaw& operator=(const MaxHeapRaw&) = delete;

    ~MaxHeapRaw() { delete[] buf; }

    bool empty() const { return sz == 0; }
    int  size()  const { return sz; }

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

    void print(std::ostream& os = std::cout) const {
        os << "MaxHeapRaw: [ ";
        for (int i = 0; i < sz; ++i) os << buf[i] << ' ';
        os << "]\n";
    }
};

int main() {
    MaxHeapRaw<int> h;
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
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ ./heap_raw
MaxHeapRaw: [ 102 88 100 7 2 10 99 1 ]
top = 102
pop = 102
MaxHeapRaw: [ 100 88 99 7 2 10 1 ]
MaxHeapRaw: [ 100 88 10 7 2 1 ]
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/TAS$ 


*/