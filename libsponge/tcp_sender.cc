#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <algorithm>
#include <random>

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

inline unsigned int TCPSender::retransmission_timeout() const {
    return _initial_retransmission_timeout << _consecutive_retransmissions;
}

inline size_t TCPSender::remaining_window_size() const {
    return (_ackno + _window_size > _next_seqno) ? (_ackno + _window_size - _next_seqno) : 0;
}

inline bool TCPSender::is_syn() const { return _next_seqno == 0; }

inline bool TCPSender::is_fin() const {
    return _stream.input_ended() && (_stream.buffer_size() < remaining_window_size()) &&
           (_stream.buffer_size() <= TCPConfig::MAX_PAYLOAD_SIZE) && !has_fin_sent();
}

inline bool TCPSender::is_transmission_empty() const { return !is_syn() && !_stream.eof() && _stream.buffer_empty(); }

inline bool TCPSender::has_fin_sent() const { return _stream.eof() && _next_seqno == _stream.bytes_written() + 2; }

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ackno; }

void TCPSender::fill_window() {
    while (remaining_window_size() > 0 && !is_transmission_empty() && !has_fin_sent()) {
        size_t payload_size = min(remaining_window_size(), TCPConfig::MAX_PAYLOAD_SIZE);

        TCPSegment seg;
        seg.header().seqno = next_seqno();
        seg.header().syn = is_syn();
        seg.header().fin = is_fin();

        string payload = _stream.read(payload_size);
        seg.payload() = Buffer(move(payload));

        _next_seqno += seg.length_in_sequence_space();

        if (!_timer_remaining.has_value()) {
            _timer_remaining = retransmission_timeout();
        }

        _segments_out.push(seg);
        _outstanding.push(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t received_ackno = unwrap(ackno, _isn, _ackno);

    // If the window size advertised by the receiver is zero, then we should give a room for FIN segment.
    _is_initial_window_size_zero = (window_size == 0);
    _window_size = window_size == 0 ? 1 : window_size;

    if (received_ackno > _next_seqno || received_ackno <= _ackno) {
        return;
    }

    if (received_ackno == _next_seqno) {
        _timer_remaining = nullopt;
    }

    while (!_outstanding.empty()) {
        TCPSegment &seg = _outstanding.front();
        uint64_t last_seqno = unwrap(seg.header().seqno + seg.length_in_sequence_space(), _isn, _ackno);

        if (last_seqno > received_ackno) {
            break;
        }

        _outstanding.pop();
    }

    _consecutive_retransmissions = 0;
    _ackno = received_ackno;

    if (!_outstanding.empty()) {
        _timer_remaining = retransmission_timeout();
    }

    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_remaining.has_value()) {
        return;
    }

    if (_timer_remaining > ms_since_last_tick) {
        _timer_remaining = _timer_remaining.value() - ms_since_last_tick;
        return;
    }

    TCPSegment &seg = _outstanding.front();
    _segments_out.push(seg);

    if (_window_size != 0 && !_is_initial_window_size_zero) {
        _consecutive_retransmissions++;
    }

    _timer_remaining = retransmission_timeout();
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    seg.header().syn = is_syn();
    seg.header().fin = is_fin();

    _next_seqno += seg.length_in_sequence_space();

    _segments_out.push(seg);

    // An ACK segment for a SYN segment must be retransmitted if lost.
    if (seg.header().syn) {
        _timer_remaining = retransmission_timeout();
        _outstanding.push(seg);
    }
}
