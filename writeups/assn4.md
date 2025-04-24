Assignment 4 Writeup
=============

My name: Doyoung Kim

My POVIS ID: d0319

My student ID (numeric): 20200429

This assignment took me about 24 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [0.74, 0.72]

Program Structure and Design of the TCPConnection:
The TCPConnection class represents a complete TCP endpoint, managing both 
sending and receiving logic. It uses a TCPSender and TCPReceiver to implement 
the transport logic. The implementation cleanly separates concerns:

* `_segments_out` queue stores outgoing segments.
* `connect()` initiates a connection with a SYN.
* `write()` writes application data to the send buffer and attempts to send it.
* `segment_received()` handles incoming segments, acknowledges them, and 
handles stream states (e.g. FIN, RST).
* `tick()` tracks time, handles retransmission logic, and manages lingering 
state after both sides finish.
* `check_activeness()` and `active()` help determine when a connection should 
shut down or remain alive (e.g., during lingering).
* The design aims to stay faithful to TCP semantics such as orderly close, 
timeout retransmissions, and robust error handling (e.g., RST handling and 
connection shutdown on excessive retransmissions).

Implementation Challenges:
* Properly coordinating the lifecycle between sender and receiver was 
tricky—especially making sure that `active()` only returns false when all 
conditions (inbound EOF, outbound EOF, no lingering, etc.) are met.
* Handling reordering or duplicate segments correctly in `segment_received()` 
required careful design to ensure correct acknowledgment and that no extra 
segments are sent unnecessarily.
* Managing retransmissions and respecting `_cfg.MAX_RETX_ATTEMPTS` demanded 
precise state tracking, or it could result in premature or missed shutdowns.
* Ensuring that `send_empty_segment()` is only called at the right times (e.g., 
after receiving data or SYN) to keep the protocol efficient without flooding.

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
