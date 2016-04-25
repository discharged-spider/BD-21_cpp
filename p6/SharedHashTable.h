#ifndef P6_SHAREDHASHTABLE_H
#define P6_SHAREDHASHTABLE_H

#include <cstddef>

#include <string>
#include <functional>
#include <iostream>
#include <cstring>

#include <cassert>

#include <sys/mman.h>
#include <fcntl.h>
#include <vector>
#include <semaphore.h>

using std::string;

//in bytes
#define KEY_SIZE 4
#define VALUE_SIZE (256 / 8)

#define ALL_SIZE (1024*1024)
#define EL_SIZE (ALL_SIZE / sizeof (item_t))
#define REAL_SIZE (EL_SIZE * sizeof (item_t))

typedef unsigned int ttl_t;

struct item_t
{
    char key [KEY_SIZE];
    ttl_t ttl;
    char value [VALUE_SIZE];

    bool test (char key_ [KEY_SIZE])
    {
        return memcmp (key_, key, KEY_SIZE) == 0;
    }

    bool empty ()
    {
        return key [0] == 0;
    }

    void clear ()
    {
        key [0] = 0;
    }

    void set (const string &key_, const ttl_t ttl_, const string &value_)
    {
        memcpy (key, key_.c_str(), std::min ((size_t)KEY_SIZE, key_.length() + 1));
        ttl = ttl_;
        memcpy (value, value_.c_str(), std::min ((size_t)VALUE_SIZE, value_.length() + 1));
    }
    void set (const string &key_)
    {
        memcpy (key, key_.c_str(), std::min ((size_t)KEY_SIZE, key_.length() + 1));
    }

    size_t str_size (const char* str, size_t max_size)
    {
        for (size_t i = 0; i < max_size; i ++)
        {
            if (str [i] == 0) return i;
        }

        return max_size;
    }

    string get ()
    {
        string result = "";
        result += string (key, str_size (key, KEY_SIZE));
        result += " ";
        result += string (value, str_size (value, VALUE_SIZE));

        return result;
    }
};

struct lock_chunk_t
{
    sem_t resource;

    lock_chunk_t ()
    {
        resource = sem_t ();
    }

    void init ()
    {
        const int shared = 1;

        sem_init (&resource, shared, 1);
    }
    void destroy ()
    {
        sem_destroy (&resource);
    }

    void read_lock    ()
    {
        sem_wait (&resource);
    }
    void read_unlock  ()
    {
        sem_post (&resource);
    }
    void write_lock   ()
    {
        sem_wait (&resource);
    }
    void write_unlock ()
    {
        sem_post (&resource);
    }
};

struct cleaner_data_t
{
    bool need_clean;
    ttl_t wait_t;
    ttl_t min_t;

    cleaner_data_t () :
        need_clean (false),
        wait_t (0),
        min_t (0)
    {}
};

class SharedHashTable
{
private:
    std::hash <string> hash_;
    item_t *memory_;
    size_t size_;

    size_t sem_step_;
    std::vector<lock_chunk_t> lock_chunks_;

    lock_chunk_t clean_lock_;
    cleaner_data_t* clean_;

    ttl_t update_ttl (ttl_t added_ttl);

    void set (size_t i, item_t& item);

    void init_sem ();

    size_t get_sem_pos (size_t i);

    void read_lock    (size_t i);
    void read_unlock  (size_t i);
    void write_lock   (size_t i);
    void write_unlock (size_t i);

public:
    SharedHashTable() :
        hash_(),
        memory_(nullptr),
        size_ (EL_SIZE), //Last el reserved for time

        sem_step_ (EL_SIZE / 100),

        lock_chunks_ ()
    {
        auto descriptor = creat ("/dev/zero", S_IRUSR | S_IWUSR);
        item_t* data = (item_t*)mmap (nullptr, REAL_SIZE + sizeof (cleaner_data_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, descriptor, 0);
        memory_ = data;

        clean_ = (cleaner_data_t*)(data + EL_SIZE);

        assert (memory_ != nullptr);

        //memset (memory_, 0, EL_SIZE * sizeof (item_t));

        init_sem ();
    }

    ~SharedHashTable()
    {
        clean_lock_.destroy();

        for (size_t i = 0; i < lock_chunks_.size(); i ++)
        {
            lock_chunks_ [i].destroy();
        }

        munmap (memory_, REAL_SIZE);
        delete[] memory_;
    }

    bool add(item_t& item);
    bool find(item_t& item, size_t& i);
    bool get(item_t& item);
    void del(item_t& item);
    void del(size_t pos);

    size_t hash (item_t& item);

    void print ();

    void clean ();
    void tick ();

    void clear_thread ();
};

#undef VALUE_SIZE

#undef REAL_SIZE

#endif //P6_SHAREDHASHTABLE_H
