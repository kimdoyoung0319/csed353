#ifndef SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
#define SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH

#include "ethernet_frame.hh"
#include "tcp_over_ip.hh"
#include "tun.hh"

#include <optional>
#include <queue>
#include <unordered_map>

//! \brief A "network interface" that connects IP (the internet layer, or network layer)
//! with Ethernet (the network access layer, or link layer).

//! This module is the lowest layer of a TCP/IP stack
//! (connecting IP with the lower-layer network protocol,
//! e.g. Ethernet). But the same module is also used repeatedly
//! as part of a router: a router generally has many network
//! interfaces, and the router's job is to route Internet datagrams
//! between the different interfaces.

//! The network interface translates datagrams (coming from the
//! "customer," e.g. a TCP/IP stack or router) into Ethernet
//! frames. To fill in the Ethernet destination address, it looks up
//! the Ethernet address of the next IP hop of each datagram, making
//! requests with the [Address Resolution Protocol](\ref rfc::rfc826).
//! In the opposite direction, the network interface accepts Ethernet
//! frames, checks if they are intended for it, and if so, processes
//! the the payload depending on its type. If it's an IPv4 datagram,
//! the network interface passes it up the stack. If it's an ARP
//! request or reply, the network interface processes the frame
//! and learns or replies as necessary.
class NetworkInterface {
  private:
    static constexpr size_t ARP_REQUEST_TIMEOUT = 5000;    //!< Retransmission timeout for an ARP request.
    static constexpr size_t ARP_MAPPING_LIFETIME = 30000;  //!< Lifetime of a mapping entry from IP to Ethernet address.

    //! \brief An entry of the IP address to Ethernet address mapping.
    struct MappingEntry {
        enum MappingState { PENDING, RESOLVED };

        MappingState state;    //!< Current state of the mapping.
        EthernetAddress addr;  //!< Actual Ethernet address of the mapping.
        size_t expire_at;      //!< The time when the mapping become invalid.

        MappingEntry() : state(PENDING), addr(), expire_at() {};
        MappingEntry(size_t time) : state(PENDING), addr(), expire_at(time + ARP_REQUEST_TIMEOUT) {}
        MappingEntry(const EthernetAddress &ethernet_addr, size_t time)
            : state(RESOLVED), addr(ethernet_addr), expire_at(time + ARP_MAPPING_LIFETIME) {};
    };

    //! Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface.
    EthernetAddress _ethernet_address;

    //! IP (known as internet-layer or network-layer) address of the interface.
    Address _ip_address;

    //! Outbound queue of Ethernet frames that the NetworkInterface wants sent.
    std::queue<EthernetFrame> _frames_out{};

    //! Cached mappings from IP address to Ethernet address.
    std::unordered_map<Address, MappingEntry, Address::Hash> _mappings{};

    //! Queue of outstanding datagrams waiting for its IP address to be resolved.
    std::unordered_map<Address, std::queue<InternetDatagram>, Address::Hash> _outstanding{};

    //! The time passed after the construction of this interface in milliseconds.
    size_t _uptime = 0;

    //! \brief Make a ARP request frame with given address as the target address.
    EthernetFrame make_arp_request_frame(const Address &addr);

    //! \brief Make a ARP reply frame with given address as the target address.
    EthernetFrame make_arp_reply_frame(const Address &ip_addr, const EthernetAddress &ethernet_addr);

    //! \brief Make a frame that holds an actual datagram.
    EthernetFrame make_datagram_frame(const InternetDatagram &dgram, const EthernetAddress &addr);

  public:
    //! \brief Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer) addresses.
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    //! \brief Access queue of Ethernet frames awaiting transmission.
    std::queue<EthernetFrame> &frames_out() { return _frames_out; }

    //! \brief Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination address).

    //! Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next hop.
    //! ("Sending" is accomplished by pushing the frame onto the frames_out queue.)
    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);

    //! \brief Receives an Ethernet frame and responds appropriately.

    //! If type is IPv4, returns the datagram.
    //! If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    //! If type is ARP reply, learn a mapping from the "sender" fields.
    std::optional<InternetDatagram> recv_frame(const EthernetFrame &frame);

    //! \brief Called periodically when time elapses.
    void tick(const size_t ms_since_last_tick);
};

#endif  // SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
