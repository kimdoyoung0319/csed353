// TODO: Add comments for methods.
#include "byte_stream.hh"

#include <iterator>
#include <stdexcept>

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _buf(capacity), _capacity(capacity) {
    if (capacity < 0) {
        throw runtime_error("invalid stream capacity:" + to_string(capacity));
    }
}

size_t ByteStream::write(const string &data) {
    if (_end)
        throw runtime_error("stream input has already been ended");

    string::const_iterator it;

    for (it = data.begin(); it != data.end() && _capacity > 0; it++) {
        _buf.push_back(*it);
        _capacity--;
        _written++;
    }

    return std::distance(data.begin(), it);
}

//! \param[in] len bytes will be copied from the output side of the buffer
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
        _capacity++;
        _read++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
string ByteStream::read(const size_t len) {
    string result = peek_output(len);
    pop_output(len);

    return result;
}
