#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "CacheTable.h"

void CacheTable::put (const string &query, const string &result)
{
    boost::unique_lock <boost::shared_mutex> lock (access_);

    auto current_time = time ();

    auto hash_iter = hash_.find (query);
    if (hash_iter != hash_.end ())
    {
        (*hash_iter).second.last_access_time = current_time;
        (*hash_iter).second.result = result;

        return;
    }

    if (hash_.size () >= HASH_SIZE)
    {
        auto min = hash_.begin ();
        for (auto i = ++hash_.begin (); i != hash_.end (); ++ i)
        {
            if (current_time - (*min).second.creation_time > LIFE_TIME) break;

            if (current_time - (*i).second.creation_time > LIFE_TIME || (*i).second.last_access_time < (*min).second.last_access_time)
            {
                min = i;
            }
        }

        hash_.erase (min);
    }

    hash_ [query] = answer {current_time, current_time, result};
}

bool CacheTable::query (const string &query, string &result)
{
    boost::shared_lock <boost::shared_mutex> lock (access_);

    auto hash_iter = hash_.find (query);

    if (hash_iter == hash_.end ()) return false;

    auto current_time = time ();
    if (current_time - (*hash_iter).second.creation_time >= LIFE_TIME) return false;

    result = (*hash_iter).second.result;

    if (current_time - (*hash_iter).second.last_access_time > UPDATE_TIME)
    {
        lock.unlock ();
        //update access_time
        put (query, result);
    }

    return true;
}

ptime CacheTable::time()
{
    return boost::posix_time::second_clock::local_time ();
}





