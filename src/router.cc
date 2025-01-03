#include "router.hh"

#include <iostream>
#include <limits>
#include <bit>
#include <ranges>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routing_table_[prefix_length][rotr(route_prefix, 32 - prefix_length)] = { interface_num, next_hop};
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto & interface : _interfaces ) {
    // std::queue<InternetDatagram> datagrams_received { interface->datagrams_received() };
    auto&& datagrams_received { interface->datagrams_received() };

    while ( not datagrams_received.empty() ) {
      InternetDatagram dgram { move( datagrams_received.front() )};
      datagrams_received.pop();

      if (dgram.header.ttl <= 1) {
        continue;
      }
      dgram.header.ttl -= 1;
      dgram.header.compute_checksum();
      optional<info>match_res { match(dgram.header.dst) };
      if ( not match_res.has_value() ) {
        continue;
      }
      const auto& [num, next_hop] { match_res.value() };
      _interfaces[num]->send_datagram( dgram, next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) ) );
    }
  }
}

[[nodiscard]] auto Router::match( uint32_t addr) const noexcept ->optional<info>
// [[nodiscard]] optional<info> Router::match( uint32_t addr ) const noexcept 
{
  auto adaptor = views::filter( [&addr]( const auto& mp ) { return mp.contains( addr >>= 1); } )
                  | views::transform( [&addr]( const auto& mp ) { return mp.at(addr); } );
  auto res { routing_table_ | views::reverse | adaptor | views::take( 1 )};
  return res.empty() ? nullopt : optional<info> { res.front() };
}