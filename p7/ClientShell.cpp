#include "ClientShell.h"

void ClientShell::on_connect()
{
    add_read ();
}

void ClientShell::add_read()
{
    DEBUG (cout << "add_read" << endl;)

    client_.async_read_some (buffer (read_buf_, BUFF_SIZE), MAP_2(read_complete, _1, _2));
}

void ClientShell::read_complete (const error_code &err, size_t bytes)
{
    DEBUG (cout << "read_complete" << endl;)

    if (bytes > 0)
    {
        DEBUG (cout << "read_ok " << bytes << endl;)

        input_.append (read_buf_, bytes);

        string& str = input_;

        while (true)
        {
            auto pos = str.find('\n');
            if (pos == std::string::npos) break;

            string message = str.substr (0, pos);
            boost::algorithm::trim (message);

            str.erase(str.begin(), str.begin() + pos + 1);

            DEBUG (cout << '{' << message << '}' << endl;)

            //if (message == "") continue;

            output_ += process_command (message);
        }

        if (output_ != "")
        {
            add_write ();
        }
    }

    if (err)
    {
        DEBUG (cout << "read_error_end" << endl;)

        client_.close ();

        return;
    }

    add_read ();
}

void ClientShell::add_write ()
{
    DEBUG (cout << "add_write" << endl;)

    if (!write_free_ || output_ == "") return;

    DEBUG (cout << "writing [" << output_.size () << "]" << endl;)

    auto size = std::min (output_.size (), (long unsigned int)BUFF_SIZE);
    for (int i = 0; i < size; i ++)
    {
        write_buf_ [i] = output_ [i];
    }

    output_.erase (output_.begin(), output_.begin() + size);

    write_free_ = false;

    client_.async_write_some (buffer (write_buf_, size), MAP_2(write_complete, _1, _2));
}

void ClientShell::write_complete (const error_code &err, size_t bytes)
{
    DEBUG (cout << "write_complete " << bytes << endl;)

    if (err)
    {
        DEBUG (cout << "write_error_end" << endl;)

        client_.close ();

        return;
    }

    write_free_ = true;

    if (output_ != "") add_write();
}

string ClientShell::process_command (string prefix)
{
    string result = "";

    auto& cache = data_ -> cache ();
    if (cache.query (prefix, result))
    {
        DEBUG (cout << "using cached result" << endl;)
        return result;
    }

    auto data = ifstream (data_ -> getData(), ifstream::in | ifstream::binary);

    typedef std::pair <int, string> item_t;
    std::vector <item_t> top_n;

    string name;
    while (data >> name)
    {
        int weight = 0;
        if (!(data >> weight))
        {
            return "Bad data file.\n";
        }
        data.ignore (std::numeric_limits<std::streamsize>::max(), '\n');

        if (boost::starts_with (name, prefix))
        {
            if (top_n.size () < TOP_SIZE)
            {
                top_n.push_back (item_t (weight, name));
            }
            else
            {
                bool ok = true;

                int min = 0;
                for (int i = 0; i < TOP_SIZE; i ++)
                {
                    if (top_n [i].first < top_n [min].first) min = i;
                    if (weight > top_n [i].first) ok = false;
                }

                if (!ok)
                {
                    top_n [min].first = weight;
                    top_n [min].second = name;
                }
            }
        }
    }

    auto comp = [] (const item_t& a, const item_t& b) -> bool {return a.first != b.first? a.first > b.first : a.second.length () < b.second.length ();};
    std::sort (top_n.begin(), top_n.end(), comp);

    for (auto& item : top_n)
    {
        result += item.second;

        result += ' ';
        result += std::to_string (item.first);

        result += '\n';
    }

    result += '\n';

    DEBUG (cout << "put result to cache" << endl;)

    cache.put (prefix, result);

    return result;
}











