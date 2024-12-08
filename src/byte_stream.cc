#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_(capacity) {}

bool Writer::is_closed() const
{
  return write_end_;
}

void Writer::push( string data )
{
  uint64_t length = data.length();

  if(length > available_capacity()) length = available_capacity();

  len_cumulative_bytes_pushed += length;

  buffer_.emplace_back(move(data));
  return;
}

void Writer::close()
{
  write_end_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - (len_cumulative_bytes_pushed - len_cumulative_bytes_popped);
}

uint64_t Writer::bytes_pushed() const
{
  return len_cumulative_bytes_pushed;
}

bool Reader::is_finished() const
{
  return write_end_ && buffer_.size() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return len_cumulative_bytes_popped;
}

string_view Reader::peek() const
{
  if(buffer_.empty()) {
    return string_view {};
  }
  string temp = buffer_.front().substr(removed_prefix_);;
  return string_view(temp);
}

void Reader::pop( uint64_t len )
{
  len_cumulative_bytes_popped += len;
  
  while( len != 0U ){
    const uint64_t& size = buffer_.front().size() - removed_prefix_;
    if( len < size ) {
      removed_prefix_ += len;
      break;
    }
    buffer_.pop_front();
    removed_prefix_ = 0;
    len -= size;
  }
}

uint64_t Reader::bytes_buffered() const
{
  return len_cumulative_bytes_pushed - len_cumulative_bytes_popped;
}
