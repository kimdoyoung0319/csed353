Assignment 1 Writeup
=============

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 8 hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:
StreamReassembler mainly consists of the buffer that stores yet unassembled 
substrings, and the output stream.

The main point within the implementation of StreamReassembler that is worth
paying attention to is that the buffer is implmented in 'circular' manner.
Since the prefix of the buffer is frequently popped from it, one must handle
popped bytes and shifted index of the buffer.

One naive way to do it is to shift the contents of the buffer as much as the 
length of popped prefix. This can be done by using `erase()` and `resize()` 
method of `std::vector`. However, doing so can be expensive since it takes 
additional O(n) time.

Hence, current implementation does not shift its element. Instead, it calculates
the position to store next byte in by performing modular operation on the index
of the byte. More specifically, if the global index of a byte is `i` and the 
capacity of the buffer is `c`, it stores the byte into index `i % c` of the 
buffer. Also, it uses `std::optional` type to distinguish invalid elements of 
the buffer with the ones that holds actual byte.

Implementation Challenges:
One challenge I met while implementing this was treating the buffer in the 
circular manner, as mentioned above. Another challenge was to handle the
end-of-file flag supplied to `push_substring()` method. This implementation
stores global index of EOF in the member variable `_eof` if `eof` flag is set. 
Also, it checks whether pushed byte's index exceeds this index, and closes the
output stream if a byte within the supplied substring is beyond the limit.

Remaining Bugs:

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
