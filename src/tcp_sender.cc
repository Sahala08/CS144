#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return total_outstanding_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return total_retrainsmission_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  while( (window_size_ == 0 ? 1 : window_size_) > total_outstanding_ ) {
    auto msg { make_empty_message() };
    if ( not SYN_sent_ ) {
      msg.SYN = true;
      SYN_sent_ = true;
    }

    // 获取不超过window_size的数据到payload
    const uint64_t remaining { (window_size_ == 0 ? 1 : window_size_) - total_outstanding_ };
    const size_t len { min( TCPConfig::MAX_PAYLOAD_SIZE, remaining - msg.sequence_length() ) };
    auto&& payload { msg.payload };
    while( reader().bytes_buffered() != 0U and payload.size() < len ) {
      string_view view { reader().peek() };
      view = view.substr( 0, len - payload.size() );
      payload += view;
      input_.reader().pop( view.size() );
    }

    // 剩余空间足够装下 并且 reader读取完成了
    if( not FIN_sent_ and remaining > msg.sequence_length() and reader().is_finished() ) {
      msg.FIN = true;
      FIN_sent_ = true;
    }

    if( msg.sequence_length() == 0 ) {
      break;
    }
    
    transmit( msg );
    if( not timer_.is_active() ) {
      timer_.start();
    }
    next_abs_seqno_ += msg.sequence_length();
    total_outstanding_ += msg.sequence_length();
    outstanding_message_.emplace( move( msg ) );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap( next_abs_seqno_, isn_ ), false, {}, false, input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if( msg.RST ) {
    input_.set_error();
    return;
  }

  window_size_ = msg.window_size;

  if( not msg.ackno.has_value() ) {
    return;
  }
  const uint64_t recv_ack_abs_seqo { msg.ackno->unwrap( isn_, next_abs_seqno_) };
  if( recv_ack_abs_seqo > next_abs_seqno_ ) {
    return; // 为啥
  }
  bool has_acknowledge { false };
  while( not outstanding_message_.empty() ) {
    const TCPSenderMessage& message { outstanding_message_.front() };
    if( ack_abs_seqno_ + message.sequence_length() > recv_ack_abs_seqo) {
      break;
    }
    has_acknowledge = true;
    ack_abs_seqno_ += message.sequence_length();
    total_outstanding_ -= message.sequence_length();
    outstanding_message_.pop();
  }
  if ( has_acknowledge ) {
    timer_.reload(initial_RTO_ms_);
    outstanding_message_.empty() ? timer_.stop() : timer_.start();
    total_retrainsmission_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if( timer_.tick(ms_since_last_tick).is_expired() ) {
    transmit(outstanding_message_.front());
    if( window_size_ != 0) {
      total_retrainsmission_ += 1;
      timer_.exponential_backoff();
    }
    timer_.reset();
  }
}
