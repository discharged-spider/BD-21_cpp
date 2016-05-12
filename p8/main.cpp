#include <iostream>
#include <omp.h>
#include <vector>
#include <numeric>
#include <algorithm>

//#define OPEN_MP

using std::cout;
using std::endl;

using std::vector;

template <typename T>
void qsort (T* array, size_t size);

int main()
{
    const size_t size = 20000000;

    vector <double> array (size);
    std::iota (array.begin(), array.end(), 0);

    cout << "created" << endl;

    std::random_shuffle (array.begin(), array.end());

    cout << "shuffled" << endl;

    auto start = time (NULL);

    auto arr = &array [0];

    #ifndef OPEN_MP
        qsort (arr, array.size());
    #else
        #pragma omp parallel shared(arr)
        {
            #pragma omp single nowait
            {
                qsort (arr, array.size());
            }
        }
    #endif

    auto end = time (NULL);

    cout << "sorted in " << (end - start) << " sec" << endl;

    for (long i = 0; i < array.size(); i ++)
    {
        if (array [i] != i)
        {
            cout << "bad" << endl;

            return 1;
        }
    }

    cout << "ok" << endl;

    return 0;
}

#define SMALL 100

template <typename T>
void qsort (T* array, size_t size)
{
    if (size <= 1)
    {
        return;
    }

    size_t l = 0, r = size - 1;
    T median = array [size / 2];

    while (l < r)
    {
        while (array [l] < median) l ++;
        while (array [r] > median) r --;

        if (l == r) break;

        std::swap (array [l], array [r]);
    }

    if (size < SMALL)
    {
        if (r > 0)    qsort (array, r);
        if (size > l) qsort (array + l, size - l);

        return;
    }

#ifdef OPEN_MP
    #pragma omp task shared(array)
#endif

    if (r > 0)    qsort (array, r);

#ifdef OPEN_MP
    #pragma omp task shared(array)
#endif

    if (size > l) qsort (array + l, size - l);

#ifdef OPEN_MP
    #pragma omp taskwait
#endif
}