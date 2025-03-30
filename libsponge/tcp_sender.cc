#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <algorithm>
#include <random>

// Implementation of a TCP sender

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno; }

void TCPSender::fill_window() {
    if (_next_seqno != 0 && (_window_size == 0 || _stream.buffer_empty())) {
        return;
    }

    //! \todo Modify this not to consider current buffer size of the stream.
    size_t len = min(
        {TCPConfig::MAX_PAYLOAD_SIZE, static_cast<size_t>(_ackno + _window_size - _next_seqno), _stream.buffer_size()});
    string payload = _stream.peek_output(len);
    _stream.pop_output(len);

    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().syn = _next_seqno == 0 ? true : false;
    seg.header().fin = _stream.eof();
    seg.payload() = Buffer(move(payload));

    _segments_out.push(seg);
    _outstanding_segments.push(seg);
    _next_seqno += len + (seg.header().syn ? 1 : 0);

    if (!_alarm.has_value()) {
        _alarm = optional(_retransmission_timeout);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _ackno = unwrap(ackno, _isn, _ackno);
    _window_size = window_size;
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_restransmissions = 0;

    if (_ackno >= _next_seqno) {
        _alarm = nullopt;
    }

    while (!_outstanding_segments.empty()) {
        TCPSegment &front = _outstanding_segments.front();

        if (unwrap(front.header().seqno, _isn, _ackno) + front.payload().size() > _ackno) {
            break;
        }

        _outstanding_segments.pop();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _current_time_in_ms += ms_since_last_tick;

    if (!_alarm.has_value() || _alarm.value() > _current_time_in_ms) {
        return;
    }

    _segments_out.push(_outstanding_segments.front());

    if (_window_size != 0) {
        _consecutive_restransmissions++;
        _retransmission_timeout *= 2;
    }

    _alarm = optional(_current_time_in_ms + _retransmission_timeout);
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().syn = _next_seqno == 0 ? true : false;
    seg.header().fin = false;
    seg.payload() = Buffer();

    _segments_out.push(seg);
}
