#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/un.h>
#include <iostream>     /* UNIX domain header */
#include <sstream>

namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

#define BUFFER 64
static int sckIn;
static int sckOut;
const char *sckInAddr = "./receiver_soc";
const char *sckOutAddr = "./sender_soc";
static struct sockaddr_un in;
static struct sockaddr_un out;

void cleanup()
{
	close(sckIn);
	close(sckOut);
	unlink(in.sun_path);
}

bool FakeError(int percentageChance) {
	int r = rand() % 100;
	if (r <= percentageChance) return true;
	return false;
}

void InitListener() {
	in.sun_family = AF_UNIX;
	strcpy(in.sun_path, sckOutAddr);
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
	strcpy(out.sun_path, sckInAddr);
	sckOut = socket(AF_UNIX, SOCK_DGRAM, 0);

	InitListener();
	// ---------------------------
}

void Send(std::string buf) {
	int n;
	if (access(out.sun_path, F_OK) > -1) {
		n = sendto(sckOut, buf.c_str(), strlen(buf.c_str()), 0, (struct sockaddr *)&out, sizeof(out));
		if ( n < 0 )
		{  
			fprintf(stderr, "sendto failed\n");
		}

		//printf("Sender: %d characters sent!\n", n);   
		//close(sckOut); 
	}
}

std::string Listen() {
	char buf[BUFFER];
	int n;
	socklen_t len;
	n = recvfrom(sckIn, buf, BUFFER, 0, (struct sockaddr *)&in, &len);
	if (n < 0)
	{
		fprintf(stderr, "recvfrom failed\n");
		return "";
	}
	
	buf[n] = '\0';                               /* (E) */
	std::string msg(buf);
	//printf("\nDatagram received = %s\n", msg.substr(0, msg.size()).c_str());     /* (F) */
	
	return buf;
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

void OneBitSliding() {
	std::string value;
	int frameNum = -1;
	int frameTot = -1;
	int lastSize = -1;
	std::vector<std::string> frames;
	
	while (true) {
		value = Listen();
		
		if (value.empty()) {
			printf("Receive Failed.\n");
			continue;
		}
		
		// Fake an error 20% of the time.
		///*
		if (FakeError(10)) {
			printf("Faking error.\n");
			continue;
		}
		/**/
		
		// Get frame number.
		std::size_t fnPos = value.find("/");
		std::string frameNumber = value.substr(0,fnPos);
		frameNum = atoi(frameNumber.c_str());
		
		// Get total frames
		std::size_t ftPos = value.find("|");
		std::string frameTotal = value.substr(fnPos + 1, ftPos - fnPos - 1);
		frameTot = atoi(frameTotal.c_str());
		if (lastSize == -1) {
			frames.resize(frameTot);
			lastSize = frameTot;
		}
		
		// Append to message.
		frames[frameNum] = value.substr(ftPos + 1, value.size() - ftPos - 1);
		
		// Show status.
		//printf("Frame #: %s Frame Total: %s \n", frameNumber.c_str(), frameTotal.c_str());
		
		// Send reply "ACK{FrameNumber}"
		std::string msg = "ACK" + frameNumber;
		Send(msg);
		if (frameTot != -1 && frameNum == frameTot -1) break;
	}
	
	// Convert message to string
	std::vector<bool> bits(32);
	int counter = 0;
	std::string output;
	std::string line = "";
	//std::string message = "";
	for (int i = 0; i < frames.size(); i++) {
		if (counter == 31) {
			//printf("Decoded: %s", (char)ToInt(bits));
			bits[counter++] = (frames[i] == "1");
			line += (frames[i] == "1") ? "1" : "0";
			printf("Char: %s %d\n", line.c_str(), ToInt(bits));
			line = "";
			output += (char)ToInt(bits);
			counter = 0;
		}
		else {
			bits[counter++] = (frames[i] == "1");
			line += (frames[i] == "1") ? "1" : "0";
		}
	}
	//printf("Raw Message Received: \n%s \n", frames.c_str());
	printf("Message Received: %s \n", output.c_str());
}

int main()
{
	InitSockets();
	OneBitSliding();
	cleanup();
	return(0);
}
