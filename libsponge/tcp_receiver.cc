#include "tcp_receiver.hh"

// Implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    const Buffer &payload = seg.payload();

    if (header.syn) {
        _isn = optional{header.seqno};
    }

    if (!_isn.has_value()) {
        throw runtime_error{"initial sequence number is not yet set"};
    }

    uint64_t index = unwrap(header.seqno, _isn.value(), _checkpoint);
    _reassembler.push_substring(payload.copy(), index, header.fin);
    _checkpoint = index;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value()) {
        return nullopt;
    }

    // The stream is closed when it consumes the conceptual FIN byte. Since this FIN takes
    // another sequence number, we should add 2 when the stream is closed.
    WrappingInt32 ackno = _isn.value() + stream_out().bytes_read() + (stream_out().input_ended() ? 2 : 1);
    return optional{ackno};
}

size_t TCPReceiver::window_size() const {
    size_t stream_start = stream_out().bytes_read();
    size_t stream_end = stream_out().bytes_written();

    return stream_start + _capacity - stream_end;
}
