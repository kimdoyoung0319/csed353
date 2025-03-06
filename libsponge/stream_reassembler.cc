#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _buf(capacity), _output(capacity), _capacity(capacity), _unassembled(0), _eof(UINT64_MAX) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    std::string to_push;
    size_t buf_index, data_index;
    uint64_t stream_index;
    byte_t ch;

    if (eof) {
        _eof = index + data.size();
    }

    if (index > _output.bytes_written()) {
        buf_index = index - _output.bytes_written();
        data_index = 0;
    } else {
        buf_index = 0;
        data_index = _output.bytes_written() - index;
    }

    while (_output.buffer_size() + buf_index < _capacity && data_index < data.size()) {
        if (!_buf[buf_index])
            _unassembled++;

        _buf[buf_index] = data[data_index];

        buf_index++;
        data_index++;
    }

    buf_index = 0;
    stream_index = _output.bytes_written();

    while (_buf[buf_index] && buf_index < _capacity && stream_index < _eof) {
        ch = _buf[buf_index].value();
        to_push.push_back(ch);

        _unassembled--;
        buf_index++;
        stream_index++;
    }

    if (!to_push.empty()) {
        _output.write(to_push);
    }

    _buf.erase(_buf.begin(), _buf.begin() + to_push.size());
    _buf.resize(_capacity);

    if (stream_index >= _eof) {
        _output.end_input();
    }
}
