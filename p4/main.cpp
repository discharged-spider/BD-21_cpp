//OS: Linux Ubuntu
//Little endian

#include <iostream>
#include "IndexReverser.h"

using namespace std;

int main ()
{
    IndexReverser ir (string ("test.txt"), string ("output.txt"), string ("temp.txt"));

    ir.make ();

    return 0;
}