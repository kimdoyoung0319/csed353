Assignment 3 Writeup
=============

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 12 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:
The `TCPSender` now has four more fields, namly `_outbound`, 
`_consecutive_retransmissions`, `_window_size, _ackno`, `_timer_remaining`, 
and `_is_initial_window_size_zero`.

Also, is has several helper classes to check necessary conditions and compute 
some values without adding verbose, cryptic routines to the main methods.
Since their names suggest the funcionalities of them, so there's no need to explain each of them here.

`fill_window()` checks if there's a room in the receiver's window and if 
there're some data or control bytes (SYN and FIN) to be sent. If there's a
room in the receiver's window and the sender should send something, it makes a 
new segment with the current sequence number (`_seqno`) and payload popped from
the input stream. Then, it sets the timer if it hasn't set one, and push the
segment into output queue and queue of outbound segments (`_outbound`). It
iteratively repeats this process until the window runs out or there's nothing
to send.

`ack_received()` receives an acknowledge number and window size, and sets the
internal states of `TCPSender`. It adjusts the window size by the received
one, considering that there's an exception for window size of zero. In this
case, it sets the window size to one and sets special flag 
`_is_initial_window_size_zero`. This flag is maintained to check if the window
size advertised by the receiver is zero. If it is, the sender is allowed to
send one byte, but should not double the retransmission timeout (RTO). There
might be another solution to conform with this requirement, but I could not 
find any better solution.

`ack_received()` ignores the ACK segment if the `ackno` is duplicated or 
errorneous (i.e. an acknowlege number that is greater than sequence number 
that has been sent so far.). After that, it turns off the timer if the receiver
acknowledged the last segment. Lastly, it pops off the segments from the queue 
of outstanding segments, and fills the window according to the updated 
internal states.

`tick()` takes the time in milliseconds, and updates the internal states 
according to this. More specifically, it advances the internal timer, if it has
not expired. If the timer has expired, it retransmits the oldest outstanding 
segment, while increasing the consecutive retranmission counter and reset the
timer.

Implementation Challenges:
One of the hardest challenge I faced was to come up with the loop invariant of
the `fill_window()` method. In other words, deciding if we should push onto
the output queue or not was the hardest job. Initially, the loop checked only 
if there is some room in the window. However, this had a problem of filling
window even if there's nothing to send. 

So, I made the loop to check the other condition, namely 
`is_transmission_empty()`, which checks if there's some data or control signals
to be sent. This second approach was also faulty since it keeps sending FIN 
segment even if it has alreay sent the FIN segment.

Hence, the current implementation also checks if the FIN segment was already 
sent or not. This let us to keep filling window if possible, while preventing
the `TCPSender` to sent unnecessary control bytes.

Also, there's an exception that even if the advertised window size is zero, the
sender should allow user to send one additional byte - while not doubling up
the retransmission timeout. The initial approach to solve this problem was to
set window size to one if the advertised window size is zero. The problem is
that the sender keeps sending one byte from its input queue. So, I added 
another flag field to `TCPSender` class to store if the advertised window size
was zero or not. 

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
