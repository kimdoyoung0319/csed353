//! \todo Revise comments.
#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <optional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! Our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! Outbound queue of segments that the TCPSender wants sent.
    std::queue<TCPSegment> _segments_out{};

    //! Outstanding segments that has not been fully acknowledged.
    std::queue<TCPSegment> _outstanding{};

    //! Retransmission timer for the connection.
    unsigned int _initial_retransmission_timeout;

    //! The number of consecutive retransmissions done so far.
    unsigned int _consecutive_retransmissions{0};

    //! Window size advertised by the receiver.
    size_t _window_size{1};

    //! Outgoing stream of bytes that have not yet been sent.
    ByteStream _stream;

    //! The absolute sequence number for the next byte to be sent.
    uint64_t _next_seqno{0};

    //! The acknowledge number translated into the absolute sequence number.
    //! \note Must be always less than or equal to _next_seqno.
    uint64_t _ackno{0};

    //! Remaining value of the timer. nullopt when the timer is not set.
    std::optional<size_t> _timer_remaining{std::nullopt};

    //! Was the window size advertised by the receiver initially zero?
    bool _is_initial_window_size_zero{false};

    //! \returns the current retransmission timeout.
    inline unsigned int retransmission_timeout() const;

    //! \returns the remaining window size.
    inline size_t remaining_window_size() const;

    //! \returns if the next segment is the SYN segment.
    inline bool is_syn() const;

    //! \returns if the next segment is the FIN segment.
    inline bool is_fin() const;

    //! \returns if there's nothing to send, including SYN and FIN bytes.
    inline bool is_transmission_empty() const;

    //! \returns if a FIN byte has been sent or not.
    inline bool has_fin_sent() const;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@_{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const { return _consecutive_retransmissions; };

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
