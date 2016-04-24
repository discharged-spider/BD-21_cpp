//OS: Linux Ubuntu
//Little endian

#include <iostream>
#include "IndexReverser.h"

using namespace std;

int main ()
{
    string input = "";
    cout << "Please, set input file name." << endl;
    cin >> input;

    string output = "";
    //cout << "Please, set output file name. (left empty for default output.bin)" << endl;
    //cin >> output;

    if (output == "")
    {
        output = "output.bin";
    }

    IndexReverser ir (input, output, string ("temp.bin"));

    ir.make ();

    return 0;
}