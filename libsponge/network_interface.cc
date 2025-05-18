#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>
#include <stdexcept>

// Implementation of a network interface.
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

using namespace std;

//! \param[in] addr IP address to request to peers.
//! \returns an ARP request frame.
EthernetFrame NetworkInterface::make_arp_request_frame(const Address &addr) {
    EthernetFrame frame;

    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.header().src = _ethernet_address;
    frame.header().dst = ETHERNET_BROADCAST;

    ARPMessage request;

    request.opcode = ARPMessage::OPCODE_REQUEST;
    request.sender_ethernet_address = _ethernet_address;
    request.sender_ip_address = _ip_address.ipv4_numeric();
    request.target_ip_address = addr.ipv4_numeric();

    frame.payload() = request.serialize();

    return frame;
}

//! \param[in] ip_addr IP address of the peer to response.
//! \param[in] ethernet_addr Ethernet address of the peer to response.
//! \returns an ARP reply frame.
EthernetFrame NetworkInterface::make_arp_reply_frame(const Address &ip_addr, const EthernetAddress &ethernet_addr) {
    EthernetFrame frame;

    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.header().src = _ethernet_address;
    frame.header().dst = ethernet_addr;

    ARPMessage request;

    request.opcode = ARPMessage::OPCODE_REPLY;
    request.sender_ip_address = _ip_address.ipv4_numeric();
    request.sender_ethernet_address = _ethernet_address;
    request.target_ip_address = ip_addr.ipv4_numeric();
    request.target_ethernet_address = ethernet_addr;

    frame.payload() = request.serialize();

    return frame;
}

//! \param[in] dgram the datagram to be stored in the Ethernet frame.
//! \param[in] addr Ethernet address of the peer.
//! \returns an datagram frame.
EthernetFrame NetworkInterface::make_datagram_frame(const InternetDatagram &dgram, const EthernetAddress &addr) {
    EthernetFrame frame;

    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.header().src = _ethernet_address;
    frame.header().dst = addr;
    frame.payload() = dgram.serialize();

    return frame;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // The Ethernet address associated with next_hop is not found. Send an ARP request message and push the datagram to
    // the outstanding queue.
    if (_mappings.find(next_hop) == _mappings.end()) {
        EthernetFrame frame = make_arp_request_frame(next_hop);

        _mappings[next_hop] = MappingEntry{_uptime};
        _frames_out.push(frame);
        _outstanding[next_hop].push(dgram);

        return;
    }

    // The entry is in pending state. Push the datagram to outstanding queue and send an ARP request again if the
    // entry's retransmission timeout is over.
    if (_mappings[next_hop].state == MappingEntry::PENDING) {
        if (_mappings[next_hop].expire_at < _uptime) {
            EthernetFrame frame = make_arp_request_frame(next_hop);

            _mappings[next_hop].expire_at = _uptime + ARP_REQUEST_TIMEOUT;
            _frames_out.push(frame);
        }

        _outstanding[next_hop].push(dgram);
        return;
    }

    // The mapping from next_hop to its Ethernet address is fully known. Send the datagram to the peer.
    EthernetAddress addr = _mappings[next_hop].addr;
    EthernetFrame frame = make_datagram_frame(dgram, addr);

    _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const EthernetHeader &header = frame.header();
    const BufferList &payload = frame.payload();

    // Ignore the frames that is not related to this interface.
    if (header.dst != ETHERNET_BROADCAST && header.dst != _ethernet_address) {
        return nullopt;
    }

    // If the frame is an IPv4 frame, unwrap it and return it to the upper layer.
    if (header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;

        if (dgram.parse(Buffer(payload)) != ParseResult::NoError) {
            return nullopt;
        }

        return dgram;
    }

    // The frame is possibly an ARP message frame. Parse it as an ARP message and extract the addresses.
    ARPMessage message;

    if (message.parse(Buffer(payload)) != ParseResult::NoError) {
        return nullopt;
    }

    Address ip_addr = Address::from_ipv4_numeric(message.sender_ip_address);
    EthernetAddress ethernet_addr = message.sender_ethernet_address;

    // Learn the mapping from sender IP address to sender ethernet address, and send outstanding datagrams for the IP
    // address.
    _mappings[ip_addr] = MappingEntry(ethernet_addr, _uptime);

    if (_outstanding.find(ip_addr) != _outstanding.end()) {
        while (not _outstanding[ip_addr].empty()) {
            InternetDatagram dgram = move(_outstanding[ip_addr].front());
            EthernetFrame dgram_frame = make_datagram_frame(dgram, ethernet_addr);

            _frames_out.push(dgram_frame);
            _outstanding[ip_addr].pop();
        }
    }

    // If the message is request message for this interface, send a reply message to the requester.
    if (message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == _ip_address.ipv4_numeric()) {
        EthernetFrame reply = make_arp_reply_frame(ip_addr, ethernet_addr);
        _frames_out.push(reply);
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _uptime += ms_since_last_tick;

    // Remove mappings that are outdated or need to be retransmitted.
    for (auto it = _mappings.begin(); it != _mappings.end();) {
        MappingEntry &entry = it->second;

        if (entry.expire_at < _uptime) {
            it = _mappings.erase(it);
        } else {
            it++;
        }
    }
}
