#include "byte_stream.hh"

#include <utility>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return write_end_;
}

void Writer::push( string data )
{
  if ( Writer::is_closed() or Writer::available_capacity() == 0 or data.empty() ) {
    return;
  }

  if ( data.size() > Writer::available_capacity() ) {
    data.resize( Writer::available_capacity() );
  }
  len_cumulative_bytes_pushed_ += data.size();

  buffer_.emplace( move( data ) );
  return;
}

void Writer::close()
{
  write_end_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( len_cumulative_bytes_pushed_ - len_cumulative_bytes_popped_ );
}

uint64_t Writer::bytes_pushed() const
{
  return len_cumulative_bytes_pushed_;
}

bool Reader::is_finished() const
{
  return write_end_ and buffer_.size() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return len_cumulative_bytes_popped_;
}

string_view Reader::peek() const
{
  return buffer_.empty() ? string_view {} // std::string_view dependents on the initializer through its lifetime.
                         : string_view { buffer_.front() }.substr( removed_prefix_ );
}

void Reader::pop( uint64_t len )
{
  len_cumulative_bytes_popped_ += len;
  while ( len != 0U ) {
    const uint64_t& size { buffer_.front().size() - removed_prefix_ };
    if ( len < size ) {
      removed_prefix_ += len;
      break; // with len = 0;
    }
    buffer_.pop();
    removed_prefix_ = 0;
    len -= size;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return len_cumulative_bytes_pushed_ - len_cumulative_bytes_popped_;
}
