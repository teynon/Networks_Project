GO BACK N WITH CRC

g++ senderReceiver.c -o senderReceiver
./senderReceiver -m "My Message"
./senderReceiver B -m "My other message"

Same as 1 BIT with CRC except ACK / NAK frame expanded to two bits

// Decode message based on defined rules.
// 
// - Message Length (NOT INCLUDING CRC) - 32 bits 
// - Frame Number - 32 bits
// - Frame Total - 32 bits
// - ACK / NAK - 2 bits (0 = IGNORE, 1 = ACK)
// - Message - N characters * 32 bits - while position < message length
// - CRC - Remainder of message