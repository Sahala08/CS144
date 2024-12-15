#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // uint32_t seqno = 0;
  // if(n < zero_point.raw_value_) {
  //   seqno = static_cast<uint32_t>(UINT32_MAX + 1ULL - zero_point.raw_value_ + n);
  // }
  // else {
  //   seqno = static_cast<uint32_t>((n - zero_point.raw_value_) % (UINT32_MAX + 1ULL));
  // }

  // return Wrap32 { seqno };
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t n_low32 { this->raw_value_ - zero_point.raw_value_ };
  const uint64_t c_low32 { checkpoint & MASK_LOW_32 };
  const uint64_t res { ( checkpoint & MASK_HIGH_32 ) | n_low32 };
  if ( res >= INT32MAX_PLUS_ONE and n_low32 > c_low32 and ( n_low32 - c_low32 ) > ( INT32MAX_PLUS_ONE / 2 ) ) {
    return res - INT32MAX_PLUS_ONE;
  }
  if ( res < MASK_HIGH_32 and c_low32 > n_low32 and ( c_low32 - n_low32 ) > ( INT32MAX_PLUS_ONE / 2 ) ) {
    return res + INT32MAX_PLUS_ONE;
  }
  return res;
}
