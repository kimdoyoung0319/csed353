#include "stream_reassembler.hh"

using namespace std;

const uint64_t inf = UINT64_MAX;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _buf(capacity), _output(capacity), _capacity(capacity), _unassembled(0), _eof(inf) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _eof = index + data.size();
    }

    copy_to_buffer(data, index);
    push_to_stream();
}

// Auxiliary function that copys substring to the internal buffer, silently
// dropping bytes in the substring that are not acceptible within the
// capacity of the reassembler.
void StreamReassembler::copy_to_buffer(const string &data, size_t index) {
    size_t buf_index, data_index;
    uint64_t stream_start = _output.bytes_read();
    uint64_t stream_end = _output.bytes_written();

    data_index = index > stream_end ? 0 : stream_end - index;
    index = index > stream_end ? index : stream_end;

    while (index - stream_start < _capacity && data_index < data.size()) {
        buf_index = index % _capacity;

        if (!_buf[buf_index].has_value())
            _unassembled++;

        _buf[buf_index] = static_cast<byte>(data[data_index]);

        index++;
        data_index++;
    }
}

// Auxiliary function that pushes the continuous prefix of the buffer into
// the stream.
void StreamReassembler::push_to_stream() {
    string to_push;
    uint64_t index = _output.bytes_written();
    size_t buf_index = index % _capacity;

    while (_buf[buf_index].has_value() && index < _eof) {
        to_push.push_back(static_cast<char>(_buf[buf_index].value()));
        _buf[buf_index] = nullopt;

        index++;
        buf_index = index % _capacity;
        _unassembled--;
    }

    if (!to_push.empty()) {
        _output.write(to_push);
    }

    if (index == _eof) {
        _output.end_input();
    }
}
