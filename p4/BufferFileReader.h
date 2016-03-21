#ifndef P4_BUFFERFILEREADER_H
#define P4_BUFFERFILEREADER_H

#include <cassert>
#include <cstdio>
#include <memory>

class BufferFileReader
{
private:
    FILE* file_;
    bool shared_mode_;
    off64_t file_pos_;
    off64_t file_end_;

    bool auto_update_;

    size_t position_;
    size_t limit_;
    size_t size_;

    std::shared_ptr <char> buffer_;
public:
    BufferFileReader (FILE* file, size_t limit, bool shared_mode = false, off64_t position = 0, off64_t end = -1) :
        file_ (file),
        shared_mode_ (shared_mode),
        file_pos_ (position),
        file_end_ (end),

        auto_update_ (false),

        position_ (0),
        limit_ (limit),
        size_ (0),
        buffer_ ()
    {
        buffer_ = std::shared_ptr<char> (new char [limit]);
    }

    ~BufferFileReader ()
    {}

    operator char *();

    void enable_update (bool state);

    void read (size_t size);
    bool require (size_t size);

    bool refresh ();

    size_t available ();

    template <typename T>
    T get ();

    template <typename T>
    T read ();

    template <typename T>
    bool available ();
};

template<typename T>
T BufferFileReader::get()
{
    assert (require (sizeof (T)));

    char* buffer = (char*)*this;
    return *((T*)buffer);
}

template<typename T>
T BufferFileReader::read ()
{
    T result = get <T> ();

    read (sizeof (T));

    return result;
}

template<typename T>
bool BufferFileReader::available ()
{
    return require (sizeof (T));
}


#endif //P4_BUFFERFILEREADER_H
