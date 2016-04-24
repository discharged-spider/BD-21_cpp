#include <algorithm>
#include <iostream>
#include <unistd.h>
#include "IndexReverser.h"
#include "BufferFileReader.h"

void IndexReverser::make ()
{
    FILE* input = fopen64 (input_.c_str(), "rb");
    FILE* temp  = fopen64 (temp_.c_str(),  "w+b");

    assert (input != nullptr);
    assert (temp != nullptr);

    fseeko64 (input, 0L, SEEK_END);
    input_size_ = ftell (input);
    fseeko64 (input, 0L, SEEK_SET);

    reverse_parts (input, temp);

    fclose (input);

    FILE* output = fopen (output_.c_str(),  "wb");
    assert (output != nullptr);

    input_size_ = ftell (temp);
    fseeko64 (temp, 0, SEEK_SET);

    create_index (temp, output);

    fclose (temp);

    fclose (output);
}

void IndexReverser::reverse_parts(FILE *input, FILE *output)
{
    BufferFileReader reader (input, FIRST_STEP_LIMIT);

    assert (sizeof (long) == 8);
    assert (sizeof (int) == 4);

    typedef pair<long, long> dict_pair;
    vector <dict_pair> words;

    while (reader.available () > FIRST_STEP_LIMIT / 3 || reader.refresh () || reader.available () > 0)
    {
        std::cout << "First step progress: " << print_memory (ftello64 (input)) << " from " << print_memory (input_size_) << std::endl;

        parts_.push_back (ftello64 (output));

        //for each data chunk
        while (reader.available <long> ())
        {
            bool end = false;

            auto doc_id = reader.read <long> ();

            if (!reader.available <int> ())
            {
                reader.refresh ();
                end = true;
            }
            auto n = reader.read<int> ();

            for (int i = 0; i < n; i ++)
            {
                if (!reader.available<long> ())
                {
                    reader.refresh ();
                    end = true;
                }
                auto word_id = reader.read<long> ();

                words.push_back (dict_pair (word_id, doc_id));
            }

            if (end) break;
        }

        std::cout << "First step progress: " << words.size() << " words" << std::endl;

        auto comp = [] (dict_pair a, dict_pair b)
        {
            if (a.first != b.first) return a.first < b.first;
            return a.second < b.second;
        };

        std::sort (words.begin(), words.end (), comp);

        std::cout << "First step progress: sorted" << std::endl;

        int last_pos = 0;
        for (int i = 0; i <= words.size(); i ++)
        {
            if (i == words.size () || words [last_pos].first != words [i].first)
            {
                words_.insert (words [last_pos].first);

                fwrite (&words [last_pos].first, sizeof (long), 1, output);

                int size = i - last_pos;
                fwrite (&size, sizeof (int), 1, output);

                for (int j = last_pos; j < i; j ++)
                {
                    assert (words [j].first == words [last_pos].first);
                    fwrite (&words [j].second, sizeof (long), 1, output);
                }

                if (i == words.size ()) break;

                last_pos = i;
            }
        }

        words.clear ();
    }
}

void IndexReverser::create_index (FILE *input, FILE *output)
{
    vector <BufferFileReader> readers;
    vector <int> readers_data;

    vector <long> offsets;

    const auto limit = SECOND_STEP_LIMIT / parts_.size ();
    assert (limit >= 8);

    assert (parts_.size () > 0);

    for (int i = 0; i < parts_.size (); i ++)
    {
        BufferFileReader reader (input, limit, true, parts_ [i], (i < parts_.size () - 1)? parts_ [i + 1] : 0);
        reader.enable_update (true);
        readers.push_back (std::move (reader));

        readers_data.push_back (0);
    }

    auto comp = [] (BufferFileReader& reader_a, BufferFileReader& reader_b)
    {
        if (!reader_a.available<long> ()) return false;
        if (!reader_b.available<long> ()) return true;
        return reader_a.get <long> () < reader_b.get <long> ();
    };

    for (auto word : words_)
    {
        fwrite (&word, sizeof (long), 1, output);
        //reserved
        fwrite (&word, sizeof (long), 1, output);
    }

    long zero = 0;
    fwrite (&zero, sizeof (long), 1, output);
    fwrite (&zero, sizeof (long), 1, output);

    int previous_p = -1;
    for (int doc_i = 0; doc_i < words_.size(); doc_i ++)
    {
        auto p = (int) ((double)doc_i / words_.size() * 100);
        if (p != previous_p)
        {
            previous_p = p;
            std::cout << "Second step progress: " << (p + 1) << "%" << std::endl;
        }

        std::sort (readers.begin(), readers.end(), comp);

        assert (readers [0].available<long> ());

        auto current_word = readers [0].read<long> ();
        int size = 1;
        for (; size < readers.size (); size ++)
        {
            if (!readers [size].available<long> ()) break;
            if (readers [size].get<long> () != current_word) break;

            assert (readers [size].read<long> () == current_word);
        }

        //assert (current_word == words_ [doc_i]);

        for (int i = 0; i < size; i ++)
        {
            //get words number
            readers_data [i] = readers [i].read <int> ();
        }

        vector <long> documents;
        while (true)
        {
            bool find = false;
            long min = 0;
            int min_i = -1;
            for (int i = 0; i < size; i++)
            {
                if (readers_data [i] > 0)
                {
                    if (!find || readers [i].get<long> () < min)
                    {
                        find = true;
                        min = readers [i].get<long> ();
                        min_i = i;
                    }
                }
            }

            if (!find) break;

            readers [min_i].read <long> ();
            readers_data [min_i] --;

            //or write to file
            documents.push_back (min);
        }

        offsets.push_back (ftello64 (output));

        //std::cout << "word: " << current_word << " count: " << documents.size() << std::endl;

        int doc_num = (int)documents.size();
        fwrite (&doc_num, sizeof (int), 1, output);
        for (auto document : documents)
        {
            fwrite (&document, sizeof (long), 1, output);
        }
    }

    fseek (output, 0, SEEK_SET);

    assert (offsets.size() == words_.size());

    auto words_pos = words_.begin();
    for (int i = 0; i < words_.size(); i ++)
    {
        fwrite (&(*words_pos), sizeof (long), 1, output);
        fwrite (&offsets [i],  sizeof (long), 1, output);

        ++words_pos;
    }
}

string IndexReverser::print_memory (off64_t bytes)
{
    if (bytes == 0) return string ("0 Byte");
    long k = 1000; // or 1024 for binary
    char sizes [][6] = {"Bytes", "KB", "MB", "GB", "TB", "PB"};
    int i = (int) (log(bytes) / log(k));

    return std::to_string ((int) (bytes / pow (k, i))) + " " + sizes [i];
}






