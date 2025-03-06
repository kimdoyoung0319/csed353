#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include "byte.hh"

#include <deque>
#include <string>

//! \brief An in-order byte stream.

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.
class ByteStream {
  private:
    size_t _remaining;        //! Remaining capacity of the buffer.
    size_t _written = 0;      //! Total number of bytes written so far.
    size_t _read = 0;         //! Total number of bytes read so far.
    std::deque<byte_t> _buf;  //! Buffer to store written bytes in.
    bool _error = false;      //!< Flag indicating that the stream suffered an error.
    bool _end = false;        //!< Flag indicating that the stream has reached its ending.

  public:
    //! Construct a stream with room for `capacity` bytes.
    ByteStream(const size_t capacity) : _remaining(capacity), _buf() {}

    //! \name "Input" interface for the writer
    //!@{

    //! Write a string of bytes into the stream. Write as many
    //! as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    size_t remaining_capacity() const { return _remaining; };

    //! Signal that the byte stream has reached its ending
    void end_input() { _end = true; }

    //! Indicate that the stream suffered an error.
    void set_error() { _error = true; }
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! Peek at next "len" bytes of the stream
    //! \returns a string
    std::string peek_output(const size_t len) const;

    //! Remove bytes from the buffer
    void pop_output(const size_t len);

    //! Read (i.e., copy and then pop) the next "len" bytes of the stream
    //! \returns a string
    std::string read(const size_t len);

    //! \returns `true` if the stream input has ended
    bool input_ended() const { return _end; };

    //! \returns `true` if the stream has suffered an error
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    size_t buffer_size() const { return _buf.size(); };

    //! \returns `true` if the buffer is empty
    bool buffer_empty() const { return _buf.empty(); };

    //! \returns `true` if the output has reached the ending
    bool eof() const { return _end && _buf.empty(); };
    //!@}

    //! \name General accounting
    //!@{

    //! Total number of bytes written
    size_t bytes_written() const { return _written; };

    //! Total number of bytes popped
    size_t bytes_read() const { return _read; };
    //!@}
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
