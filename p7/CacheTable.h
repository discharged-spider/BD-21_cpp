#ifndef P7_HASHTABLE_H
#define P7_HASHTABLE_H

#include <boost/date_time/posix_time/posix_time.hpp>

#include <map>
#include <boost/thread/pthread/shared_mutex.hpp>

#define HASH_SIZE 1000
#define LIFE_TIME   boost::posix_time::seconds (10)
#define UPDATE_TIME boost::posix_time::seconds (2)

using std::string;
using std::map;

using boost::posix_time::ptime;

class CacheTable
{
private:
    struct answer
    {
        ptime creation_time;
        ptime last_access_time;
        string result;
    };

    //prefix-answer list
    map <string, answer> hash_;

    boost::shared_mutex access_;

    ptime time ();

public:

    CacheTable () :
        hash_ ()
    {}

    bool query (const string& query, string& result);
    void put (const string& query, const string& result);
};


#endif //P7_HASHTABLE_H
