#include "byte_stream.hh"

#include <iterator>
#include <stdexcept>

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] data is data to write on the stream
//! \returns the number of bytes that are actually written
//! \throws runtime_error when the stream is already been closed
size_t ByteStream::write(const string &data) {
    if (_end) {
        throw runtime_error("stream input has already been ended");
    }

    string::const_iterator it;

    for (it = data.begin(); it != data.end() && _remaining > 0; it++) {
        _buf.push_back(*it);
        _remaining--;
        _written++;
    }

    return std::distance(data.begin(), it);
}

//! \param[in] len bytes will be copied from the output side of the buffer
//! \returns the contents of the stream of the length len
string ByteStream::peek_output(const size_t len) const {
    if (len > _buf.size()) {
        throw runtime_error("invalid peek length:" + to_string(len));
    }

    return string(_buf.begin(), _buf.begin() + len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > _buf.size()) {
        throw runtime_error("invalid peek length:" + to_string(len));
    }

    for (size_t i = 0; i < len; i++) {
        _buf.pop_front();
        _remaining++;
        _read++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string that are actually read
string ByteStream::read(const size_t len) {
    string result = peek_output(len);
    pop_output(len);

    return result;
}
