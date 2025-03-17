Assignment 2 Writeup
=============

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 7 hours to do (including the time on studying, 
designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission 
numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please 
refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:

# Implementation of WrappingInt32
The `wrap()` function is implemented as one can trivially expect; it just add 
the offset into the initial sequence number(`isn`). 

`unwrap()` function is a bit tricky; it first calculates offset of the relative 
sequence number(RSN) from the ISN, and calculates two candidates, namely `lower` 
and `upper`.

```

             Lower base  Upper base
                  |          |
                  V          V
    ---|------L---|--C---U---|----------|----> Number line (of 64-bit numbers)
       A       <->        <->
       |      offset     offset
       |
  These bars denote the
boundaries for every 2^32 bytes

```

These candidates (represented as `L` and `U` above) are calculated by adding 
offset to 'bases'. The lower base is the largest multiple of 2^32 that is 
smaller than `checkpoint` (represented as `C` above). The upper base is the 
smallest multiple of 2^32 that is greater than `checkpoint`. Or, you may think 
these bases are the result of sticking `checkpoint` toward multiple of 2^32.

The function picks the candidate that has smaller distance toward the 
checkpoint.

# Implementation of TCPReceiver
The `TCPReceiver` now has two fields, namely `_isn` and `_checkpoint`. `_isn` is
the ISN, which is initiallize as the RSN of the SYN segment. `_checkpoint` holds 
the RSN of last received segment. This is used for index translation.

The `segment_received()` method initializes `_isn` if `seg` is the SYN segment,
and translate the RSN into the index for output stream, stripping conceptual SYN
and FIN bytes. It lastly pushes the payload into the reassembler with FIN flag,
updating the ASN.

It just ignores the segment if it receives a segment that is not SYN before 
receiving the SYN segment.

Implementation Challenges:
Translating a RSN into stream index was a biggest challenge for me. More 
specifically, devising a working heuristic for setting checkpoint was hardest.

There can be two possible option for checkpoint; one is the index of first
unassembled byte, and the other is the RSN of last segment. The first option
may cause problem since it may translate the RSN into stream index BEFORE the
first unassembled byte - which will make the reassembler to ignore the payload.

However, the second option, which is taken in current implementation may also 
cause a problem since after pushing multiple, non-prefix segments, the prefix 
segment may be pushed with wrong index ((right index) + 2^32). I used the term
prefix segment and non-prefix segment to refer the segment that does not hold
the first unassembled byte and vice versa.

I took second option since there is no reasonable choice left for me. Also,
I think the test suite does not examines this situation.

Remaining Bugs:
The bug mentioned above may happen when several non-prefix segments are 
received.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
