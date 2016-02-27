#include <stdexcept>
#include <string>
#include <vector>
#include <stdio.h>
#include <cstring>
#include "stdio.h"

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

using std::vector;

class Pointer;

class Allocator {
private:
    char *base;
    vector <bool> memory_map;

    vector<char*> pointers;

    void mark_memory (int from, int size, bool value);

public:
    Allocator(void *base_, int size);

    void move_pointers (char *from, char *to);

    Pointer alloc(int N);
    void realloc(Pointer &p, int N);
    void free(Pointer &p, bool to_null = true);

    void defrag();
    std::string dump() { return ""; }

    char *get (int index);
};


class Pointer {
private:
    Allocator* alloc;
    int index;

public:
    Pointer () : Pointer (nullptr, 0) {}
    Pointer (Allocator* alloc_, int index_) : alloc (alloc_), index (index_) {}

    char *get() const { return (alloc)? (alloc -> get(index)) : nullptr; }
};
