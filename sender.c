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


void cleanup()
{
	close(sckIn);
	close(sckOut);
	unlink(in.sun_path);
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

void Send(std::string buf, int frame, int total) {
	int n;
	char buffer[BUFFER];
	sprintf(buffer, "%d/%d|%s", frame, total, buf.c_str());
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

void OneBitSliding(std::string message) {
	std::vector<bool> data = StringToBits(message);
	std::string rawMessage = "";
	int total = data.size();
	printf("Result size: %d\n", data.size());
	for (int i = 0; i < total; i++) {
		std::string d;
		if (data[i]) d = "1";
		else d = "0";
		Send(d, i, total);
		std::string ack = Listen();
		if (ack.empty()) {
			printf("Go back 1 \n");
			i--;
		}
		else {
			//printf("ACK? %d atoi() = %d \n", ack.compare(0, 3, "ACK"), atoi(ack.substr(3, ack.size() - 3).c_str()));
			if (ack.compare(0, 3, "ACK") != 0 && atoi(ack.substr(3, ack.size() - 3).c_str()) != i) {
				i--;
			}
			else {
				rawMessage += d;
			}
		}
	}
	printf("Raw Message:\n%s\n", rawMessage.c_str());
}

int main()
{
	InitSockets();
	OneBitSliding("Hello World");
	cleanup();
	return(0);
}
