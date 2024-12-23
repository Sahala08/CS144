#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"
#include "parser.hh"

using namespace std;

EthernetFrame NetworkInterface::make_frame( const EthernetAddress& dst,
                                            const uint16_t type,
                                            vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = move( payload );
  return frame;
}

ARPMessage NetworkInterface::make_arp( const uint16_t opcode,
                                      const EthernetAddress target_ethernet_address,
                                      const uint32_t target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const AddressNumeric next_hop_numeric { next_hop.ipv4_numeric() };
  if ( ARP_cache_.contains(next_hop_numeric) ) {
    const EthernetFrame transmit_frame { make_frame(ARP_cache_[next_hop_numeric].first, EthernetHeader::TYPE_IPv4, serialize( dgram )) };
    return transmit( transmit_frame );
  } 
  dgrams_waiting_[next_hop_numeric].emplace_back( dgram );
  if ( waiting_timer_.contains( next_hop_numeric )) {
    return;
  }
  waiting_timer_.emplace( next_hop_numeric, NetworkInterface::Timer {});

  const ARPMessage arp_request { make_arp( ARPMessage::OPCODE_REQUEST, {}, next_hop_numeric)};
  const EthernetFrame transmit_frame { make_frame(ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serialize( arp_request )) };
  transmit( transmit_frame );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  const EthernetAddress target_eth { frame.header.dst };
  if ( (target_eth != ETHERNET_BROADCAST) and (target_eth != ethernet_address_) ) {
    return;
  } 

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram ipv4_datagram;
    if ( parse( ipv4_datagram, frame.payload ) ) {
      datagrams_received_.emplace( move( ipv4_datagram ) );
    }
    return;
  }
  
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    if ( not parse( arp, frame.payload )) {
      return;
    }

    const AddressNumeric sender_ip { arp.sender_ip_address };
    const EthernetAddress sender_eth { arp.sender_ethernet_address };
    ARP_cache_[sender_ip] = { sender_eth, Timer {} };

    if ( arp.opcode == ARPMessage::OPCODE_REQUEST and arp.target_ip_address == ip_address_.ipv4_numeric() ) {
      const ARPMessage arp_reply { make_arp( ARPMessage::OPCODE_REPLY, sender_eth, sender_ip) };
      const EthernetFrame transmit_frame { make_frame(sender_eth, EthernetHeader::TYPE_ARP, serialize( arp_reply )) };
      transmit( transmit_frame );
    }

    if ( dgrams_waiting_.contains( sender_ip ) ) {
      for ( const InternetDatagram& dgram : dgrams_waiting_[sender_ip] ) {
        const EthernetFrame transmit_frame { make_frame(sender_eth, EthernetHeader::TYPE_IPv4, serialize( dgram )) };
        transmit( transmit_frame );
      }
      dgrams_waiting_.erase( sender_ip ); 
      waiting_timer_.erase( sender_ip ); 
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  erase_if( waiting_timer_, [&]( auto&& item ) noexcept -> bool {
    return item.second.tick( ms_since_last_tick ).expired( ARP_RESPONSE_TTL_ms );
  }); 

  erase_if( ARP_cache_, [&]( auto&& item ) noexcept -> bool {
    return item.second.second.tick( ms_since_last_tick ).expired( ARP_ENTRY_TTL_ms );
  }); 
}
