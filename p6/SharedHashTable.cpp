#include <unistd.h>
#include "SharedHashTable.h"

#define DEBUG(code) code
//#define DEBUG(code)

bool SharedHashTable::add(item_t& item)
{
    size_t start_index = hash (item);

    size_t i = start_index;
    do
    {
        read_lock (i);

        //add new
        if (memory_[i].empty())
        {
            read_unlock (i);
            set (i, item);
            return true;
        }

        //already_in
        if (memory_[i].test (item.key))
        {
            read_unlock (i);
            set (i, item);
            return true;
        }

        read_unlock (i);

        i ++;
        i %= size_;
    }
    while (i != start_index);

    return false;
}

void SharedHashTable::set(size_t i, item_t &item)
{
    write_lock (i);

    memory_[i] = item;
    memory_ [i].ttl = update_ttl (item.ttl);

    write_unlock (i);
}

bool SharedHashTable::get(item_t &item)
{
    size_t pos = 0;
    return find (item, pos);
}

bool SharedHashTable::find(item_t &item, size_t &pos)
{
    size_t start_index = hash (item);

    size_t i = start_index;
    do
    {
        read_lock (i);

        if (memory_[i].empty())
        {
            read_unlock (i);
            return false;
        }

        if (memory_[i].test (item.key))
        {
            item = memory_ [i];
            read_unlock (i);

            pos = i;
            return true;
        }

        read_unlock (i);

        i ++;
        i %= size_;
    }
    while (i != start_index);

    return false;
}

void SharedHashTable::del(item_t &item)
{
    size_t pos = 0;
    if (!find (item, pos))
    {
        return;
    }

    del (pos);
}

void SharedHashTable::del(size_t pos)
{
    auto j = pos + 1;
    j %= size_;

    read_lock (j);
    while (memory_[j].empty() || hash(memory_[j]) != hash(memory_[pos]))
    {
        if (memory_[j].empty())
        {
            read_unlock (j);

            write_lock (pos);
            memory_[pos].clear();
            write_unlock (pos);

            return;
        }

        read_unlock (j);
        j ++;
        j %= size_;
        read_lock (j);
    }

    set (pos, memory_ [j]);
    read_unlock (j);

    del (j);
}

size_t SharedHashTable::hash(item_t &item)
{
    return hash_ (string (item.key, KEY_SIZE)) % size_;
    //return item.key [0]
}

void SharedHashTable::print()
{
    size_t i = 0;
    while (i < size_)
    {
        if (!memory_ [i].empty()) std::cout << memory_ [i].key << std::endl;

        i ++;
    }
}

ttl_t SharedHashTable::update_ttl (ttl_t added_ttl)
{
    DEBUG (std::cout << "update ttl " << added_ttl << std::endl);

    ttl_t result = 0;

    clean_lock_.read_lock();

    if (clean_ -> need_clean)
    {
        result = added_ttl + clean_ -> wait_t;

        if (result < clean_ -> min_t)
        {
            clean_lock_.read_unlock();

            clean_lock_.write_lock();
            clean_ -> min_t = result;
            clean_lock_.write_unlock();
        }
        else
        {
            clean_lock_.read_unlock();
        }
    }
    else
    {
        clean_lock_.read_unlock();

        clean_lock_.write_lock();
        clean_ -> min_t  = added_ttl;
        clean_ -> need_clean = true;

        clean_lock_.write_unlock();

        result = added_ttl;
    }

    return result;
}

void SharedHashTable::clean ()
{
    clean_lock_.write_lock ();

    DEBUG (std::cout << "clean" << std::endl);

    clean_ -> need_clean = false;

    std::vector<size_t> to_del;

    size_t i = 0;
    while (i < size_)
    {
        read_lock (i);

        if (!memory_ [i].empty())
        {
            read_unlock (i);
            write_lock (i);

            memory_ [i].ttl -= clean_ -> wait_t;
            if (memory_ [i].ttl  <= 0)
            {
                to_del.push_back ((int)i);
            }
            else
            {
                if (! (clean_ -> need_clean))
                {
                    clean_ -> need_clean = true;
                    clean_ -> min_t = memory_ [i].ttl;
                }
                else
                {
                    clean_ -> min_t = std::min (clean_ -> min_t, memory_ [i].ttl);
                }
            }

            write_unlock (i);
        }
        else
        {
            read_unlock (i);
        }

        i ++;
    }

    clean_ -> wait_t = 0;

    while (to_del.size() > 0)
    {
        i = to_del.back ();
        to_del.pop_back ();

        read_lock (i);
        DEBUG (std::cout << "del " << memory_ [i].get() << std::endl);
        read_unlock (i);
        del (i);
    }

    clean_lock_.write_unlock ();
}

void SharedHashTable::tick()
{
    clean_lock_.read_lock();

    if (! (clean_ -> need_clean ))
    {
        DEBUG (std::cout << "tick free" << std::endl);

        clean_lock_.read_unlock();
        return;
    }

    clean_lock_.read_unlock();

    clean_lock_.write_lock();
    (clean_ -> wait_t) ++;

    DEBUG (std::cout << "tick " << clean_ -> wait_t << " / " << clean_ -> min_t << std::endl);

    if (clean_ -> wait_t >= clean_ -> min_t)
    {
        clean_lock_.write_unlock();
        clean ();

        return;
    }

    clean_lock_.write_unlock();
}

void SharedHashTable::init_sem()
{
    clean_lock_.init ();

    //ceil
    auto num = (EL_SIZE + sem_step_ - 1) / sem_step_;

    lock_chunks_.insert (lock_chunks_.begin(), num, lock_chunk_t ());

    for (size_t i = 0; i < lock_chunks_.size(); i ++)
    {
        lock_chunks_ [i].init();
    }
}

size_t SharedHashTable::get_sem_pos(size_t i)
{
    return i / sem_step_;
}

void SharedHashTable::read_lock(size_t i)
{
    auto pos = get_sem_pos (i);

    lock_chunks_ [pos].read_lock();
}
void SharedHashTable::read_unlock(size_t i)
{
    auto pos = get_sem_pos (i);

    lock_chunks_ [pos].read_unlock();
}
void SharedHashTable::write_lock(size_t i)
{
    auto pos = get_sem_pos (i);
    lock_chunks_ [pos].write_lock();
}
void SharedHashTable::write_unlock(size_t i)
{
    auto pos = get_sem_pos (i);
    lock_chunks_ [pos].write_unlock();
}

void SharedHashTable::clear_thread()
{
    while (true)
    {
        sleep (1);
        tick ();
    }
}

























