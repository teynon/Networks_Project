1 BIT BIDIRECTIONAL WITHOUT CRC

g++ senderReceiver.c -o senderReceiver
./senderReceiver -m "My Message"
./senderReceiver B -m "My other message"

Similar to 1 BIT UNIDIRECTIONAL except that the message send is formatted as:
[frame #]/[frame total]|[data]@ACK[ACK #]

This also is now one file.