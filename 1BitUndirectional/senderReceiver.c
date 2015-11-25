#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <vector>
#include <math.h>
#include <iostream>     /* UNIX domain header */

#define BUFFER 64
static int sckIn;
static int sckOut;
const char *sckInAddr = "./sender_soc";
const char *sckOutAddr = "./receiver_soc";
static struct sockaddr_un in;
static struct sockaddr_un out;
static int incomingFrameTotal = -1;
static int incomingLastFrame = -1;
static bool isA = true;


void cleanup()
{
	close(sckIn);
	close(sckOut);
	unlink(in.sun_path);
}

void DecodeMessage(std::string message, int &frameNumber, int &frameTotal, std::string &data, int &ackFrame) {
	frameNumber = -1;
	frameTotal = -1;
	ackFrame = -1;
	std::vector<std::string> frames;
		
	// Get frame number.
	std::size_t fnPos = message.find("/");
	std::string frameNumber = message.substr(0,fnPos);
	frameNumber = atoi(frameNumber.c_str());
		
	// Get total frames
	std::size_t ftPos = message.find("|");
	std::string frameTotal = message.substr(fnPos + 1, ftPos - fnPos - 1);
	frameTotal = atoi(frameTotal.c_str());
	if (lastSize == -1) {
		frames.resize(frameTot);
		lastSize = frameTot;
	}
	
	std::size_t fAckPos = message.find("@ACK");
	data = message.substr(ftPos + 1, fAckPos - ftPos - 1);
	
	std::string ackNumber = message.substr(fAckPos + 4, message.size() - fAckPos - 4);
	ackFrame = atoi(ackNumber.c_str());
}

std::vector<bool> MergeVectors(std::vector<bool> left, std::vector<bool> right) {
	std::vector<bool> AB;
	AB.reserve(left.size() + right.size());
	AB.insert(AB.end(), left.begin(), left.end());
	AB.insert(AB.end(), right.begin(), right.end());
	return AB;
}

std::vector<bool> ToBits(int val) {
	int numBits = 32;
	std::vector<bool> bits(numBits);

	for (int i = numBits - 1; i >= 0; i--) {
		int power = pow(2, i);
		bits[i] = val & power;
	}

	return bits;
}

void InitListener() {
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
		fprintf(stderr, "bind failed\n");
		cleanup();
		exit(1);
	}
}

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

	InitListener();
	// ---------------------------
}

void Send(std::string buf, int frame, int total, int ackFrame) {
	int n;
	char buffer[BUFFER];
	sprintf(buffer, "%d/%d|%s@ACK%d", frame, total, buf.c_str(), ackFrame);
	if (access(out.sun_path, F_OK) > -1) {
		n = sendto(sckOut, buffer, strlen(buffer), 0, (struct sockaddr *)&out, sizeof(out));
		if ( n < 0 )
		{  
			fprintf(stderr, "sendto failed\n");
			exit(1);
		}

		//printf("Sender: (%s) %d characters sent!\n", buffer, n);   
		//close(sckOut); 
	}
}

std::string Listen() {
	char buf[BUFFER];
	int n;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 200;
	if (setsockopt(sckIn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		printf("Error setting timeout.\n");
	}
	
	socklen_t len;
	n = recvfrom(sckIn, buf, BUFFER, 0, (struct sockaddr *)&in, &len);
	if (n < 0)
	{
		printf("Receive Failed.\n");
		return "";
	}
	else {
		buf[n] = '\0';                               /* (E) */
		std::string msg(buf);
		//printf("\nDatagram received = %s\n", msg.substr(0, msg.size()).c_str());     /* (F) */
		return msg;
	}
	//close(sckIn);
	//InitListener();
}

std::vector<bool> StringToBits(std::string msg) {
	std::vector<bool> results;
	printf("Message size: %d\n", msg.size());
	for (int i = 0; i < msg.size(); i++) {
		std::vector<bool> data = ToBits((int)msg[i]);
		results = MergeVectors(results, data);
	}
	printf("Result size: %d\n", results.size());
	
	return results;
}

int ToInt(std::vector<bool> bits) 
{
	int val = 0;
	for (int i = 0; i < bits.size(); i++) {
		if (bits[i]) {
			val |= 1 << i;
		}
	}
	//std::cout << "Decoded: " << val << std::endl;

	return val;
}

void OneBitSliding(std::string message) {
	std::vector<std::string> incomingFrames;
	std::vector<bool> outgoingFrames = StringToBits(message);
	int outgoingFrames = outgoingFrames.size();
	int outgoingFrame = 0;
	int incomingFrameLastSize = -1;
	int incomingFramesTotal = -1;
	int incomingFrame = -1;
	int incomingLastFrame = -1;
	int incomingAckFrame = -1;
	int pendingAck = -1;
	std::string incomingMessage = "";
	std::string incomingData = "";
	
	while (incomingLastFrame < incomingFramesTotal - 1 || outgoingFrame < outgoingFrames - 1) {
		bool outgoingData = false;
		
		// Send message. Wait for response.
		std::string outgoingData = "-1";
		if (outgoingFrame < outgoingFrames - 1) {
			outgoingData = true;
			if (outgoingFrames[outgoingFrame]) outgoingData = "1";
			else outgoingData = "0";
		}
		
		if (outgoingData || pendingAck != -1) {
			Send(outgoingData, outgoingFrame, outgoingFrames, pendingAck);
			pendingAck = -1;
		}
		
		incomingMessage = Listen();
		
		// If incoming message is empty, no message received. We must resend last message.
		if (incomingMessage.empty()) {
			// Only resend if we weren't already finished sending. Otherwise, we are just acknowledging incoming messages.
			if (outgoingFrame < outgoingFrames - 1) {
				outgoingFrame--;
			}
			printf("Receive Failed.\n");
			continue;
		}
		else {
			// Decode message.
			DecodeMessage(incomingMessage, &incomingFrame, &incomingFramesTotal, &incomingData, &incomingAckFrame);
			
			// If frame acknowledge is frame sent, increase outgoing frame
			if (incomingAckFrame != -1 && incomingAckFrame == outgoingFrame) {
				outgoingFrame++;
			}
			
			if (incomingFramesLastSize < incomingFramesTotal) {
				incomingFrames.resize(incomingFramesTotal);
				incomingFramesLastSize = incomingFramesTotal;
			}
			
			if (incomingFrame != -1) {
				incomingFrames[incomingFrame] = incomingData;
				incomingLastFrame = incomingFrame;
			}
		}
	}
	
	// Convert message to string
	std::vector<bool> bits(32);
	int counter = 0;
	std::string output;
	std::string line = "";
	
	for (int i = 0; i < incomingFrames.size(); i++) {
		if (counter == 31) {
			//printf("Decoded: %s", (char)ToInt(bits));
			bits[counter++] = (incomingFrames[i] == "1");
			line += (incomingFrames[i] == "1") ? "1" : "0";
			printf("Char: %s %d\n", line.c_str(), ToInt(bits));
			line = "";
			output += (char)ToInt(bits);
			counter = 0;
		}
		else {
			bits[counter++] = (incomingFrames[i] == "1");
			line += (incomingFrames[i] == "1") ? "1" : "0";
		}
	}
	//printf("Raw Message Received: \n%s \n", frames.c_str());
	printf("Message Received: %s \n", output.c_str());
}

int main(int argc, char * argv[])
{
	std::string message = "Hello world.";
	for (int i = 0; i < argc; i++) {
		if (argv[i] == "B") 
			isA = false;
		else if (argv[i] == "-m") {
			message = argv[i++];
		}
	}
	
	InitSockets();
	OneBitSliding(message);
	cleanup();
	return(0);
}
