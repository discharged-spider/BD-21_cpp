#include "allocator.h"

Allocator::Allocator (void *base_, int size) :
    base ((char*)base_),
    memory_map (size, false),
    pointers ()
{}

void Allocator::mark_memory (int from, int size, bool value)
{
    for (int i = from; i < from + size; i ++) memory_map [i] = value;
}

Pointer Allocator::alloc (int N)
{
    int len = 0;
    for (int i = 0; i < memory_map.size (); i ++)
    {
        if (memory_map [i] == true)
        {
            len = 0;

            continue;
        }

        len ++;

        if (len == N + sizeof (int))
        {
            i ++;

            i -= N;

            mark_memory (i - sizeof (int), len, true);

            ((int*)(base + i)) [-1] = N;

            pointers.push_back (base + i);

            return Pointer (this, pointers.size () - 1);
        }
    }

    throw AllocError (AllocErrorType::NoMemory, "no memory while alloc");
}

void Allocator::move_pointers(char *from, char *to)
{
    for (int i = 0; i < pointers.size (); i ++)
    {
        if (pointers [i] == from) pointers [i] = to;
    }
}

void Allocator::free(Pointer &p, bool to_null)
{
    char *addr = (char*)p.get();
    if (!memory_map [addr - base]) throw AllocError (AllocErrorType::InvalidFree, "invalid free");

    int N = ((int*)addr) [-1];

    mark_memory (addr - base - sizeof (int), N + sizeof (int), false);

    if (to_null) move_pointers (addr, nullptr);
}

void Allocator::realloc (Pointer &p, int N)
{
    if (p.get() == nullptr)
    {
        p = alloc (N);

        return;
    }

    char *addr = (char*)p.get();
    int oldN = ((int*)addr) [-1];

    free (p, false);

    Pointer new_p = alloc (N);

    if (new_p.get() != p.get())
    {
        std::memcpy (new_p.get(), p.get(), oldN);
    }

    move_pointers ((char*)p.get (), (char*)new_p.get());
}

char *Allocator::get (int index)
{
    return pointers [index];
}

void Allocator::defrag ()
{
    int left = 0;

    int j = 0;
    for (int i = 0; i < memory_map.size (); i ++)
    {
        if (memory_map [i] == true)
        {
            if (left == 0)
            {
                char* addr = base + i;
                left = ((int*)(base + i)) [0] + sizeof (int);

                move_pointers (base + i + sizeof (int), base + j + sizeof (int));
            }

            base [j] = base [i];

            j ++;
            left --;
        }
    }

    mark_memory (0, j, true);
    mark_memory (j, memory_map.size () - j, false);
}
