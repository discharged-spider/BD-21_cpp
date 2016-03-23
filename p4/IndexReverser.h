#ifndef P4_INDEXREVERSER_H
#define P4_INDEXREVERSER_H

#include <string>
#include <vector>

//in bytes
#define MEMORY_LIMIT (1024 * 1024) //1GB
#define FIRST_STEP_LIMIT  (MEMORY_LIMIT / 2)
#define SECOND_STEP_LIMIT (MEMORY_LIMIT)

using std::string;

using std::vector;
using std::pair;

class IndexReverser
{
private:
    string input_;
    string output_;
    string temp_;

    vector <long> words_;
    vector <off64_t > parts_;

    off64_t input_size_;

    void reverse_parts (FILE* input, FILE* output);
    void create_index (FILE* input, FILE* output);

public:
    IndexReverser (string input, string output, string temp):
        input_ (input),
        output_ (output),
        temp_ (temp),

        words_ (),
        parts_ (),

        input_size_ (0)
    {}

    void make ();

    string print_memory (off64_t memory);
};


#endif //P4_INDEXREVERSER_H
