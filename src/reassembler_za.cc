#include "reassembler.hh"

using namespace std;

std::map<uint64_t, std::string>::iterator Reassembler::split(uint64_t pos) noexcept
{
  auto it {reassem_.lower_bound(pos)};                // 找到第一个键不小于给定值 pos 的元素
  if(it != reassem_.end() and it->first == pos) {
    return it;
  }
  if(it == reassem_.begin()) {                        // if reassem_.empty() then begin() == end()
    return it;
  }
  if(const auto pit {prev(it)}; pit->first + size(pit->second) > pos) {
    const auto res = reassem_.emplace_hint(it, pos, pit->second.substr(pos - pit->first));
    pit->second.resize(pos - pit->first);
    return res;
  }
  return it;
}

void Reassembler::try_close() noexcept
{
  if(end_index_.has_value() and end_index_.value() == writer().bytes_pushed()) {
    output_.writer().close();
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // 空数据
  if(data.empty()) {
    if(not end_index_.has_value() and is_last_substring) {
      end_index_.emplace(first_index);
    }
    return try_close();
  }

  // 写端已经关闭
  if(writer().is_closed() or writer().available_capacity() == 0U) {
    return;
  }

  // 中间状态：[unassembled_index, unacceptable_index)
  const uint64_t unassembled_index { writer().bytes_pushed() };
  const uint64_t unacceptable_index { unassembled_index + writer().available_capacity() };
  if(first_index + size(data) <= unassembled_index or first_index >= unacceptable_index) {
    return;  // Out of range
  }
  if(first_index + size(data) > unacceptable_index) { // Remove unacceptable bytes
    data.resize(unacceptable_index - first_index);
    is_last_substring = false;
  }
  if(first_index < unassembled_index) {               // Remove poped/buffered bytes
    data.erase(0, unassembled_index - first_index);
    first_index = unassembled_index;
  }
  if(not end_index_.has_value() and is_last_substring) {
    end_index_.emplace(first_index + size(data));
  }

  const auto upper {split(first_index + size(data))};
  const auto lower {split(first_index)};
  // upper, lower;
  for (auto it = lower; it != upper; it++) {
    len_bytes_pending_ -= it->second.size();
  }
  len_bytes_pending_ += size(data);
  auto position = reassem_.erase(lower, upper);
  reassem_.emplace_hint(position, first_index, move(data));

  while(not reassem_.empty()) {
    auto&& [index, payload] = *reassem_.begin();
    if(index != writer().bytes_pushed()) {
      break;
    }
    len_bytes_pending_ -=size(payload);
    output_.writer().push(move(payload));
    reassem_.erase(reassem_.begin());
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return len_bytes_pending_;
}
