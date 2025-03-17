#include "tcp_receiver.hh"

// Implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

//! \todo Refactor this.
void TCPReceiver::segment_received(const TCPSegment &seg) {
    bool syn = seg.header().syn;
    bool fin = seg.header().fin;
    WrappingInt32 seqno = seg.header().seqno;
    string payload = seg.payload().copy();

    if (_state == ERROR || _state == FIN_RECV) {
        throw runtime_error("the receiver is in invalid state");
    }

    if (syn) {
        _state = SYN_RECV;
        _isn = seqno;
    }

    uint64_t abs_seqno = unwrap(seqno, _isn, _checkpoint) + syn;
    _reassembler.push_substring(payload, abs_seqno - 1, fin);
    _checkpoint = abs_seqno;

    if (stream_out().input_ended()) {
        _state = FIN_RECV;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    WrappingInt32 ackno = wrap(stream_out().bytes_written() + 1, _isn);

    switch (_state) {
        case SYN_RECV:
            return optional(ackno);
            break;
        case FIN_RECV:
            return optional(ackno + 1);
            break;
        default:
            return nullopt;
            break;
    }
}

size_t TCPReceiver::window_size() const {
    size_t stream_start = stream_out().bytes_read();
    size_t stream_end = stream_out().bytes_written();

    return stream_start + _capacity - stream_end;
}
