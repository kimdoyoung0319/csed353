#ifndef SPONGE_LIBSPONGE_FORWARDING_TABLE_HH
#define SPONGE_LIBSPONGE_FORAWRDING_TABLE_HH

#include "address.hh"

#include <cstdint>
#include <memory>
#include <optional>

//! \brief An abstraction for forwarding tables,
//! implemented by a compressed trie. Provides lookup and
//! update within O(n) time where n is the length of the
//! address.
class ForwardingTable {
  public:
    struct Entry {
        std::optional<Address> next_hop;  //!< The IP address for the next-hop network node.
        size_t interface_num;             //!< The index for the mapped network interface.
    };

    //! Insert an entry to the forwarding table.
    void insert(const uint32_t route_prefix,
                const uint8_t prefix_length,
                const std::optional<Address> next_hop,
                const size_t interface_num);

    //! Find the longest prefix match entry in the forwarding table.
    std::optional<Entry> find(const uint32_t addr) const;

  private:
    //! A node of the trie.
    struct Node {
        std::unique_ptr<Node> children[2];  //!< Child nodes.
        std::optional<Entry> entry;         //!< Entry for the node.

        Node() : children{nullptr, nullptr}, entry() {};
    };

    //! A pointer that refers to the root node of the trie.
    std::unique_ptr<Node> _root = std::make_unique<Node>();
};

#endif  // SPONGE_LIBSPONGE_FORWARDING_TABLE_HH