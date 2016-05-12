#include "Data.h"

const char* Data::getData()
{
    return "data.txt";
}

CacheTable &Data::cache()
{
    return table_;
}




