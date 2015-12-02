1 BIT BIDIRECTIONAL WITH CRC

g++ senderReceiver.c -o senderReceiver
./senderReceiver -m "My Message"
./senderReceiver B -m "My other message"

Revamped the message structure and added CRC generator and verifier.

// Decode message based on defined rules.
// 
// - Message Length (NOT INCLUDING CRC) - 32 bits 
// - Frame Number - 32 bits
// - Frame Total - 32 bits
// - ACK / NAK - 1 bit (0 = IGNORE, 1 = ACK)
// - Message - N characters * 32 bits - while position < message length
// - CRC - Remainder of message