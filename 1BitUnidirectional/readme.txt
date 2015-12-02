1 BIT UNIDIRECTIONAL Without CRC

Simple sender / receiver model
g++ sender.c -o sender
g++ receiver.c -o receiver

./receiver
./sender

Sender sends message like [frame #]/[total frames]|[messagedata]
Receiver responds with ACK[Frame#]