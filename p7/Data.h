#ifndef P7_DATA_H
#define P7_DATA_H

#include <cstdio>
#include "CacheTable.h"

#define TOP_SIZE 10

class Data
{
private:
    CacheTable table_;
public:
    Data () :
        table_ ()
    {}

    const char* getData ();
    CacheTable& cache ();
};

#endif //P7_DATA_H
