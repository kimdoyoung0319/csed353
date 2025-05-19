#include "router.hh"

#include <iostream>

using namespace std;

// Implementation of an IP router.

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

optional<Router::TableEntry> Router::find_forwarding_table_entry(const uint32_t dest_addr) const {
    auto max_entry = _forwarding_table.end();
    size_t max_length = 0;

    for (auto it = _forwarding_table.begin(); it != _forwarding_table.end(); it++) {
        if (prefix(it->prefix, it->length) == prefix(dest_addr, it->length)) {
            max_entry = (max_length <= it->length) ? it : max_entry;
            max_length = max_entry->length;
        }
    }

    if (max_entry == _forwarding_table.end()) {
        return nullopt;
    } else {
        return *max_entry;
    }
}

inline uint32_t Router::prefix(const uint32_t addr, const uint8_t length) const {
    if (length == 0) {
        return 0;
    } else {
        return addr >> (32 - length);
    }
}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    auto it = _forwarding_table.begin();

    for (; it != _forwarding_table.end(); it++) {
        if (it->prefix == route_prefix && it->length == prefix_length) {
            break;
        }
    }

    if (it == _forwarding_table.end()) {
        _forwarding_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
    } else {
        it->next_hop = next_hop;
        it->interface_num = interface_num;
    }
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    IPv4Header &header = dgram.header();
    optional<TableEntry> entry = find_forwarding_table_entry(header.dst);

    if (not entry.has_value() || header.ttl <= 1) {
        return;
    }

    header.ttl--;

    size_t interface_num = entry.value().interface_num;
    optional<Address> next_hop = entry.value().next_hop;

    if (not next_hop.has_value()) {
        Address direct_dest = Address::from_ipv4_numeric(header.dst);
        _interfaces[interface_num].send_datagram(dgram, direct_dest);
    } else {
        _interfaces[interface_num].send_datagram(dgram, next_hop.value());
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
