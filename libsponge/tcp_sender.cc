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
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno; }

unsigned int TCPSender::retransmission_timeout() const {
    return _initial_retransmission_timeout << _consecutive_retransmissions;
}

void TCPSender::fill_window() {
    TCPSegment seg;
    string payload;
    size_t len = min(_remaining_window_size, TCPConfig::MAX_PAYLOAD_SIZE);

    //! \todo Modify this to conform with the condition that the initial remaining window size must be 1.
    if (_next_seqno != 0 && _remaining_window_size == 0) {
        return;
    }

    if (_next_seqno != 0 && !_stream.eof() && _stream.buffer_empty()) {
        return;
    }

    if (_stream.eof() && _next_seqno == _stream.bytes_written() + 2) {
        return;
    }

    payload = _stream.peek_output(len);
    _stream.pop_output(len);

    seg.payload() = Buffer(move(payload));
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().syn = (_next_seqno == 0);
    seg.header().fin = _stream.eof() && seg.length_in_sequence_space() < _remaining_window_size;

    _next_seqno += seg.length_in_sequence_space();
    //! \todo Possible overflow?
    _remaining_window_size -= seg.length_in_sequence_space();

    if (!_timer_remaining.has_value()) {
        _timer_remaining = retransmission_timeout();
    }

    _segments_out.push(seg);
    _outstanding.push(seg);

    fill_window();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t received_ackno = unwrap(ackno, _isn, _ackno);

    if (window_size == 0) {
        _remaining_window_size = received_ackno + 1 - _next_seqno;
        _is_window_size_zero = true;
    } else {
        _remaining_window_size = received_ackno + window_size - _next_seqno;
        _is_window_size_zero = false;
    }

    if (received_ackno > _next_seqno || received_ackno <= _ackno) {
        return;
    }

    if (received_ackno == _next_seqno) {
        _timer_remaining = nullopt;
    }

    while (!_outstanding.empty()) {
        TCPSegment &seg = _outstanding.front();
        uint64_t end_seqno = unwrap(seg.header().seqno + seg.length_in_sequence_space(), _isn, _ackno);

        if (end_seqno > received_ackno) {
            break;
        }

        _outstanding.pop();
    }

    if (received_ackno > _ackno) {
        _consecutive_retransmissions = 0;

        if (!_outstanding.empty()) {
            _timer_remaining = retransmission_timeout();
        }
    }

    _ackno = received_ackno;

    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_remaining.has_value()) {
        return;
    }

    if (_timer_remaining.value() > ms_since_last_tick) {
        _timer_remaining.value() -= ms_since_last_tick;
        return;
    }

    TCPSegment &seg = _outstanding.front();
    _segments_out.push(seg);

    if (!_is_window_size_zero) {
        _consecutive_retransmissions++;
    }

    _timer_remaining = retransmission_timeout();
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().syn = (_next_seqno == 0);
    seg.header().fin = _stream.eof();

    _next_seqno += seg.length_in_sequence_space();
    _segments_out.push(seg);
}
