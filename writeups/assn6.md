# Assignment 6 Writeup

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 10 hours to do (including the time on studying,
designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission
numbers that you used (e.g., 1, 2): []

-   **Caution**: If you have no idea about above best-submission item, please
    refer the Assignment PDF for detailed description.

Program Structure and Design of the Router:

1. `ForwardingTable` class

-   An abstraction of forwarding tables.
-   Implemented by radix tree (trie) to provide fast lookup.
-   O(n) time for the string of length n. Since addresses are limited in their
    length by 32 bits, it provides O(1) time lookup and insertion.
-   `ForwardingTable::insert()`
    -   Inserts an entry into the forwarding table.
    -   Updates the entry if such entry already exists.
-   `ForwardingTable::find()`
    -   Finds an entry from the forwarding table.
    -   Returns the longest prefix matching entry.
    -   Returns `nullopt` if such entry does not exist.

2. `Router` class

-   `Router::route_one_diagram()`

    -   Finds an entry from the forwarding table.
    -   If such entry does not exists or the TTL value is less than 2, drop the
        datagram.
    -   Decreases TTL, passes the datagram to proper next-hop network node.

-   `Router::add_route()`
    -   Simple wrapper for `ForwardingTable::insert()` method.

Implementation Challenges:

-   Implementing `ForwardingTable` class using trie data structure.
-   Correctly manipulating address in bitwise manner.

Remaining Bugs:

-   There's no bug found, but I think this implementation can be improved by:

    -   Futher optimizing `ForwardingTable` - In current implementation, too
        much dynamic allocation happens to store an entry.

-   Optional: I had unexpected difficulty with: [describe]

-   Optional: I think you could make this lab better by: [describe]

-   Optional: I was surprised by: [describe]

-   Optional: I'm not sure about: [describe]
