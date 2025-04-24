#include "tcp_connection.hh"

#include <iostream>

// Implementation of a TCP connection.

using namespace std;

void TCPConnection::send_rst_segment() {
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }

    TCPSegment seg = move(_sender.segments_out().front());
    _sender.segments_out().pop();

    seg.header().rst = true;

    _segments_out.push(move(seg));
}

void TCPConnection::send_segments() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = move(_sender.segments_out().front());
        TCPHeader &header = seg.header();
        _sender.segments_out().pop();

        if (_receiver.ackno().has_value()) {
            header.ackno = _receiver.ackno().value();
            header.ack = true;
        }

        header.win = _receiver.window_size();

        _segments_out.push(move(seg));
    }
}

void TCPConnection::check_activeness() {
    if (_receiver.unassembled_bytes() != 0 || !inbound_stream().eof()) {
        return;
    }

    if (!_sender.stream_in().eof() || (_sender.bytes_in_flight() != 0)) {
        return;
    }

    if (_linger_after_streams_finish && !_linger_timer.has_value()) {
        _linger_timer = 10 * _cfg.rt_timeout;
    } else if (!_linger_after_streams_finish) {
        _active = false;
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    _time_since_last_segment_received = 0;

    if (header.rst) {
        inbound_stream().set_error();
        _sender.stream_in().set_error();
        _active = false;

        return;
    }

    _receiver.segment_received(seg);

    if (header.ack) {
        _sender.ack_received(header.ackno, header.win);
    }

    if (seg.length_in_sequence_space() != 0) {
        if (_sender.stream_in().buffer_empty()) {
            _sender.send_empty_segment();
        } else {
            _sender.fill_window();
        }
    } else if (_receiver.ackno().has_value() && header.seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    if (inbound_stream().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    send_segments();
    check_activeness();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t written = _sender.stream_in().write(data);

    _sender.fill_window();
    send_segments();

    return written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;

    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        send_rst_segment();

        inbound_stream().set_error();
        _sender.stream_in().set_error();
        _active = false;
    }

    if (_linger_timer.has_value()) {
        if (_linger_timer.value() <= ms_since_last_tick) {
            _active = false;
        } else {
            _linger_timer.value() -= ms_since_last_tick;
        }
    }

    send_segments();
    check_activeness();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();

    send_segments();
    check_activeness();
}

void TCPConnection::connect() {
    if (_sender.stream_in().buffer_empty()) {
        _sender.send_empty_segment();
    } else {
        _sender.fill_window();
    }

    send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            send_rst_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
