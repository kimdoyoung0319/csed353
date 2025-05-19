#include "forwarding_table.hh"

using namespace std;

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
//! \note This updates the entry in the forwarding table if it already exists.
void ForwardingTable::insert(const uint32_t route_prefix,
                             const uint8_t prefix_length,
                             const optional<Address> next_hop,
                             const size_t interface_num) {
    if (prefix_length > 32) {
        return;
    }

    Node *cur = _root.get();
    uint8_t pos = 0;

    while (pos < prefix_length) {
        bool bit = (route_prefix >> (31 - pos)) & 1;

        if (cur->children[bit] == nullptr) {
            cur->children[bit] = make_unique<Node>();
        }

        cur = cur->children[bit].get();
        pos++;
    }

    cur->entry = {next_hop, interface_num};
}

//! \param[in] addr The numeric IPv4 address to find in the forwarding table.
//! \returns the entry if it exists in the forwarding table.
optional<ForwardingTable::Entry> ForwardingTable::find(const uint32_t addr) const {
    Node *cur = _root.get();
    optional<Entry> result = nullopt;

    for (int pos = 0; pos < 32; pos++) {
        bool bit = (addr >> (31 - pos)) & 1;

        if (cur->entry.has_value()) {
            result = cur->entry;
        }

        if (cur->children[bit] == nullptr) {
            break;
        }

        cur = cur->children[bit].get();
    }

    return result;
}