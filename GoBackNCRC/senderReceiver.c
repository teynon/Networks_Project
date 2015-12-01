#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <vector>
#include <math.h>
#include <stdexcept>
#include <iostream>     /* UNIX domain header */

#define BUFFER 1024
static int sckIn;
static int sckOut;
const char *sckInAddr = "./sender_soc";
const char *sckOutAddr = "./receiver_soc";
static struct sockaddr_un in;
static struct sockaddr_un out;
static int incomingFramesTotal = -1;
static int incomingLastFrame = -1;
static bool isA = true;
static std::string generatorPoly = "1001101";


void cleanup()
{
	close(sckIn);
	close(sckOut);
	unlink(in.sun_path);
}

// Generate the CRC.
std::string generator(std::string message) {
	std::string check = "";
	int j = 0;
	int size = message.size() + generatorPoly.size();
	
	for (int y = 0; y < generatorPoly.size(); y++) {
		message += '0';
	}
	
	for (j = 0; j < generatorPoly.size(); j++) {
		check += message[j];
	}
	
	do {
		if (check[0] == '1') {
			for (int k = 1; k < generatorPoly.size(); k++) {
				check[k] = (check[k] == generatorPoly[k]) ? '0' : '1';
			}
		}
		int l = 0;
		for (l = 0; l < generatorPoly.size() - 1; l++) {
			check[l] = check[l + 1];
		}
		
		check[l] = message[j++];
	}
	while (j < size);
	
	return check;
}

// Verify the received message & crc.
bool verifier(std::string message, std::string poly) {
	std::string polynomial = generator(message);
	//printf("Message: %s Poly1: %s Poly2: %s\n", message.c_str(), polynomial.c_str(), poly.c_str());
	if (polynomial.compare(poly) == 0) {
		return true;
	}
	else {
		return false;
	}
}

// Merge two bool vectors.
std::vector<bool> MergeVectors(std::vector<bool> left, std::vector<bool> right) {
	std::vector<bool> AB;
	AB.reserve(left.size() + right.size());
	AB.insert(AB.end(), left.begin(), left.end());
	AB.insert(AB.end(), right.begin(), right.end());
	return AB;
}

// Convert an integer to a vector<bool> binary representation.
std::vector<bool> ToBits(int val) {
	int numBits = 32;
	std::vector<bool> bits(numBits);
	int32_t v = val;

	for (int i = numBits - 1; i >= 0; i--) {
		int power = pow(2, i);
		bits[i] = v & power;
	}

	return bits;
}

// Convert a string to binary.
std::vector<bool> StringToBits(std::string msg) {
	std::vector<bool> results;
	for (int i = 0; i < msg.size(); i++) {
		std::vector<bool> data = ToBits((int)msg[i]);
		results = MergeVectors(results, data);
	}
	
	return results;
}

// Convert binary to integer.
int ToInt(std::vector<bool> bits) 
{
	int val = 0;
	for (int i = 0; i < bits.size(); i++) {
		if (bits[i]) {
			val |= 1 << i;
		}
	}

	return val;
}

// Convert a string of binary to integer.
int ToInt(std::string bits) {
	int val = 0;
	for (int i = 0; i < bits.size(); i++) {
		if (bits[i] == '1') {
			val |= 1 << i;
		}
	}
	
	return val;
}

// Convert an integer to a bit string.
std::string ToBitString(int val) {
	std::vector<bool> bits = ToBits(val);
	std::string result = "";
	for (int i = 0; i < bits.size(); i++) {
		if (bits[i]) result += "1";
		else result += "0";
	}
	
	return result;
}

// Decode message based on defined rules.
void DecodeMessage(std::string &message, int &frameNumber, int &frameTotal, std::string &data, int &ackFrame, bool &ack, std::string &crc) {
	// First 4 bytes = message length;
	try {
		int index = 0;
		
		//printf("Message Size: %d\n", message.size());
		
		int msgLength = ToInt(message.substr(index, 32));
		
		if (msgLength >= message.size()) {
			crc = "000000";
			return;
		}
		
		//printf("Message Length: %d\n", msgLength);
		
		data = "";
		index += 32;
		frameNumber = ToInt(message.substr(index, 32));
		index += 32;
		frameTotal = ToInt(message.substr(index, 32));
		index += 32;
		if (message.substr(index, 2) == "01") {
			ackFrame = ToInt(message.substr(index + 2, 32));
			ack = true;
		}
		else if (message.substr(index, 2) == "10") {
			ackFrame = ToInt(message.substr(index + 2, 32));
			ack = false;
		}
		else {
			//ackFrame = -1;
		}
		
		index += 34;
		while (index < msgLength) {
			// There should be x 32 bits left.
			data += (char)ToInt(message.substr(index, 32));
			index += 32;
		}
		
		
		// The CRC polynomial is the remaining code.
		crc = message.substr(index, message.size() - index);
		
		message = message.substr(0, msgLength);
	}
	catch (const std::out_of_range& e) {
		crc = "000000";
		return;
	}
}

// Encode message based on defined rules.
std::string EncodeMessage(std::string buf, int frame, int total, int ackFrame, bool ack) {
	// First 4 bytes = message length
	// Second 4 bytes = frame #
	// Third 4 Bytes = frame total
	// 2 Bit = Ack Frame? (Yes / No / NAK)
	// Fourth 4 Bytes = ack frame value
	// Remaining = message
	// 16 bytes + 1 bit + (message length * 32)
	std::string result = "";
	result.append(ToBitString((16 * 8) + 2 + (buf.size() * 32)));
	result.append(ToBitString(frame));
	result.append(ToBitString(total));
	if (ackFrame == -1 || ack) {
		result.append((ackFrame == -1) ? "00" : "01");
	}
	else {
		result.append("10");
	}
	result.append(ToBitString(ackFrame));
	
	for (int i = 0; i < buf.size(); i++) {
		result.append(ToBitString(buf[i]));
	}
	
	// Now append crc.
	std::string poly = generator(result);
	result += poly;
	
	return result;
}

// Initialize listener socket.
void InitListener(bool last) {
	in.sun_family = AF_UNIX;
	
	if (isA) {
		strcpy(in.sun_path, sckOutAddr);
	}
	else {
		strcpy(in.sun_path, sckInAddr);
	}
	
	sckIn = socket(AF_UNIX, SOCK_DGRAM, 0);
	int n;
	n = bind(sckIn, (const struct sockaddr *)&in, sizeof(in));
	if (n < 0)
	{
		cleanup();
		if (!last) {
			InitListener(true);
		}
		else {
			fprintf(stderr, "bind failed\n");
			cleanup();
			exit(1);
		}
	}
}

// Initialize sockets.
void InitSockets() {
	// Init socket.    
	// ---------------------------
	out.sun_family = AF_UNIX;
	
	if (isA) {
		strcpy(out.sun_path, sckInAddr);
	}
	else {
		strcpy(out.sun_path, sckOutAddr);
	}
	
	sckOut = socket(AF_UNIX, SOCK_DGRAM, 0);

	InitListener(false);
	// ---------------------------
}

// Send the message & occasionally simulate an error.
void Send(std::string buf, int frame, int total, int ackFrame, bool ack) {
	std::string msg = EncodeMessage(buf, frame, total, ackFrame, ack);
	//printf("Sending %s\nFrame: %d/%d, AckFrame: %d\n", buf.c_str(), frame, total, ackFrame);
	// Send an error frame? Because the application is not 
	// continuous, the last frame can not be an error or the 
	// receiver will not finish. If this application were to 
	// continuously run, this would not be necessary.
	// Additionally, since rand is based on time, it appears that
	// the errors send at the same time. To eliminate confusion,
	// only A will be sending errors.
	if (isA && rand() % 1000 == 0) {
		// Flip a bit.
		int flipBit = rand() % msg.size();
		msg[flipBit] = (msg[flipBit] == '1') ? '0' : '1';
		printf("Sending error at frame %d\n", frame);
	}
	
	if (access(out.sun_path, F_OK) > -1) {
		sendto(sckOut, msg.c_str(), strlen(msg.c_str()), 0, (struct sockaddr *)&out, sizeof(out));
	}
}

// Listen for a response.
std::string Listen() {
	char buf[BUFFER];
	int n;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 400;
	if (setsockopt(sckIn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		printf("Error setting timeout.\n");
	}
	
	socklen_t len;
	n = recvfrom(sckIn, buf, BUFFER, 0, (struct sockaddr *)&in, &len);
	if (n < 0)
	{
		return "";
	}
	else {
		buf[n] = '\0';
		std::string msg(buf);
		return msg;
	}
}

void SendFrame(std::vector<bool> frames, int frame, int pendingAck, bool ack) {
	std::string outgoingData;
	if (frames[frame]) outgoingData = "1";
	else outgoingData = "0";
	Send(outgoingData, frame, frames.size(), pendingAck, ack);
}

// Transmit & receive message using the one bit sliding window.
void GoBackN(std::string message) {
	std::vector<std::string> incomingFrames;
	std::vector<bool> outgoingFrames = StringToBits(message);
	std::vector<bool> ackFrames = std::vector<bool>(outgoingFrames.size());
	int outgoingFrameTotal = outgoingFrames.size();
	int outgoingFrame = 0;
	int incomingFramesLastSize = -1;
	int incomingFrame = -1;
	int incomingAckFrame = -1;
	int lastAck = -1;
	int timeout = 10; // timeout after 10 frames.
	int lastAckSent = -1;
	int lastIncoming = -1;
	int pendingAck = -1;
	int expectedFrame = 0;
	int nAckFrame = -1;
	bool ack = true;
	std::string incomingMessage = "";
	std::string incomingData = "";
	std::string crc = "";
	std::string outgoingData = "0";
	
	printf("Attempt To Send.\n");
	while ((incomingFrame < incomingFramesTotal || expectedFrame <= incomingFramesTotal || incomingFramesTotal == -1) || (incomingAckFrame < outgoingFrameTotal - 1)) {
		//printf("Incoming Frame: %d/%d, incomingAckFrame: %d, outgoingFrameTotal: %d\n", incomingFrame, incomingFramesTotal, incomingAckFrame, outgoingFrameTotal);
		if (outgoingFrame - timeout >= lastAck) {
			outgoingFrame = lastAck + 1;
		}
		
		if (outgoingFrame < outgoingFrameTotal - 1 || pendingAck != -1) {
			SendFrame(outgoingFrames, outgoingFrame, pendingAck, ack);
			ack = true;
			pendingAck = -1;
		}
		
		if (outgoingFrame < outgoingFrameTotal)
			outgoingFrame++;
		
		incomingMessage = Listen();
		
		if (incomingMessage.empty()) {
			continue;
		}
		else {
			DecodeMessage(incomingMessage, incomingFrame, incomingFramesTotal, incomingData, incomingAckFrame, ack, crc);
			if (!verifier(incomingMessage, crc)) {
				incomingFrame = incomingLastFrame;
				continue;
			}
			
			if (!ack && incomingAckFrame != -1) {
				// NAK received. Resend frame.
				SendFrame(outgoingFrames, incomingAckFrame, -1, true);
			}
			else if (ack && incomingAckFrame != -1) {
				// ACK Received. We can move forward.
				if (incomingAckFrame > lastAck)
					lastAck = incomingAckFrame;
			}
			
			// Resize storage if needed.
			if (incomingFramesLastSize < incomingFramesTotal) {
				incomingFrames.resize(incomingFramesTotal);
				incomingFramesLastSize = incomingFramesTotal;
			}
			
			// Store data.
			if (incomingFrame < incomingFramesTotal)
				incomingFrames[incomingFrame] = incomingData;
			
			// If the incoming frame is one more than the last, store it. Otherwise, ignore it.
			if (incomingFrame == incomingLastFrame + 1) {
				incomingLastFrame++;
			}
			
			if (incomingFrame == expectedFrame) {
				expectedFrame++;
				// Send an ack.
				pendingAck = incomingLastFrame;
				ack = true;
				//Send("", -1, -1, incomingLastFrame, true);
			}
			else if (incomingFrame < expectedFrame) {
				// Send an ack.
				//Send("", -1, -1, incomingFrame, true);
				pendingAck = incomingFrame;
				ack = true;
			}
			else {
				// Send a nak for the expected frame.
				//Send("", -1, -1, expectedFrame, false);
				pendingAck = expectedFrame;
				ack = false;
				printf("Sending NAK %d. Received unexpected frame %d\n", expectedFrame, incomingFrame);
			}
		}
	}
	
	//printf("Last Incoming Frame: %d/%d Outgoing Frame: %d/%d\n", incomingLastFrame, incomingFramesTotal, outgoingFrame, outgoingFrameTotal);
	
	// Convert message to string
	std::vector<bool> bits(32);
	int counter = 0;
	std::string output;
	std::string line = "";
	
	for (int i = 0; i < incomingFrames.size(); i++) {
		if (counter == 31) {
			bits[counter++] = (incomingFrames[i] == "1");
			line += (incomingFrames[i] == "1") ? "1" : "0";
			printf("Char: %c %s %d\n", (char)ToInt(bits), line.c_str(), ToInt(bits));
			line = "";
			output += (char)ToInt(bits);
			counter = 0;
		}
		else {
			bits[counter++] = (incomingFrames[i] == "1");
			line += (incomingFrames[i] == "1") ? "1" : "0";
		}
	}
	
	printf("Message Received: %s \n", output.c_str());
}

int main(int argc, char * argv[])
{
	std::string message = "Hello world.";
	for (int i = 0; i < argc; i++) {
		//printf("arg %i = %s", i, argv[i]);
		if (strcmp(argv[i], "B") == 0) 
			isA = false;
		else if (strcmp(argv[i], "-m") == 0) {
			message = argv[++i];
		}
	}
	
	printf("Sending Message: %s\n", message.c_str());
	
	InitSockets();
	GoBackN(message);
	cleanup();
	return(0);
}
