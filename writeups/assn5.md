Assignment 5 Writeup
=============

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 7 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:

1. Storing mapping from IP addresses to Ethernet addresses

To store the mapping from IP addresses to Ethernet addresses, `NetworkInterface`
class has member field `_mappings`. It is an unordered map from `Address`, which
represents an IP address, to `MappingEntry`. 

`MappingEntry` is a class that stores an Ethernet address along with its 
expiration timeout (`expire_at`) and state (`state`). An entry of the mapping is
either pending or resolved. `PENDING` state represents the state where an ARP
request has been sent but not been replied yet. `RESOLVED` state represents 
a mapping that has been fully resolved, by receiving reply message from the 
peer. `expire_at` field of an entry stores the lifespan of a mapping when it is
in `PENDING` state. If it is in `RESOLVED` state, `expire_at` stores the 
retransmission timeout at which the ARP request must be transmitted again.

When the interface needs to learn a mapping from IP address to the corresponding
Ethernet address, it sends an ARP request message to the broadcast address, and
make an entry in `_mappings` of `PENDING` state, setting the retransmission 
timeout to 5 sec. After, the interface sets the mapping to `RESOLVED` state
and stores the Ethernet address, with the lifespan of 30 sec when it receives 
the mapping from its peer. Notice that `NetworkInterface` object has `_uptime` 
field which stores the time passes after its construction to know passage of 
time.

There is a small caveat that one needs to write a hash function object in order
to use a class as the key type of `std::unordered_map`. Hence, `Address` class
is also slightly modified to proivied a hash value corresponding to the address.
it is computed as the result of XOR operation between the numeric IPv4 address
and the port number shifted by 1. i.e. 
`hash(address) = address.ipv4_numeric() ^ (address.port() << 1)`

2. Managing outstanding datagrams

To manage outstanding datagrams, or the datagrams waiting for its destination 
address to be resolved, `NetworkInterface` class maintains `_outstanding` field
as a private member variable. `_outstanding` is also an unordered map between
`Address` and `std::queue<InternetDatagram>`. When a datagram's destination 
address has not yet been fully resolve, the interface store it to the queue 
associated with the destination address. After resolving the address, the 
interface pops datagrams from the queue and sends it to the peer.

3. Learning and handling passage of time

When a `NetworkInterface` learns passage of time by `NetworkInterface::tick()`,
it first `_uptime` field mentioned above. Then, it iterates over the entries
of mapping and erases entries that is outdated (the pending entry whose 
retransmission timeout is passes, or resolved entry whose lifetime is over).

Implementation Challenges:

Finding out an appropriate data structure to store mappings and outstanding 
segments was the hardest challenge when implementing this. There are several 
resonable choices for such data structure. `std::map` or some linear data 
structure such as `std::list` or `std::vector` is also a possible candidate for
this. The reason why I chose `std::unordered_map` over the other structures is
because it provides O(1) time complexity for insertion/deletion/lookup while 
`std::map` needs O(log n) time to do such operations. Contigous structures like 
`std::vector` is not an ideal choice since the set of mappings is 'sparse'. In
other words, there can be up to 2^32 * 2^16 unresolved mappings, which makes
it nearly impossible and inefficient to implement this with such structures.
`std::list` is not the best choice since its worst-case time complexity for
basic operations is O(n).

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
