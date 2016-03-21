#include <cstring>
#include <unistd.h>
#include "BufferFileReader.h"

BufferFileReader::operator char *()
{
    return buffer_.get() + position_;
}

void BufferFileReader::read (size_t size)
{
    assert (size >= 0);

    position_ += size;

    assert (position_ <= size_);
}

bool BufferFileReader::require (size_t size)
{
    assert (size <= limit_);

    if (auto_update_ && available () < size) refresh ();

    return available () >= size;
}

bool BufferFileReader::refresh ()
{
    if (file_ == nullptr) return false;

    char* buffer = buffer_.get ();
    char* end = (char*) *this;
    char* start = buffer + available ();
    memmove (buffer, end, available ());

    if (shared_mode_) fseeko64 (file_, file_pos_, SEEK_SET);
    auto count = fread (start, sizeof (char), limit_ - available (), file_);

    if (shared_mode_ && file_end_ > 0 && file_end_ - file_pos_ < count) count = file_end_ - file_pos_;

    if (count < limit_ - available ()) file_ = nullptr;

    size_ = available () + count;

    if (shared_mode_) file_pos_ += count;

    position_ = 0;

    return count > 0;
}

void BufferFileReader::enable_update (bool state)
{
    auto_update_ = state;
}

size_t BufferFileReader::available ()
{
    return size_ - position_;
}








