# TRX4_TLOCK
Digispark attiny85 controller for Traxxas T-Lock system (platformio project).

Controller is used to read pulse from one channel and control two locking diffs servos.

Controller can switch between only front or only rear locking on the way.
Underclock to 8MHz for power saving. 
Pinout: P0 - First Servo, P1 - Second servo, P4 - Pulse Data from RX.
Controllre has advanced mod: fast switch 2->1->2 position. This will be change rear diff lock to front diff lock.

Feel free to email me: brutevinch@gmail.com
