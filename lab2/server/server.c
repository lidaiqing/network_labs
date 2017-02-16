#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define MAXLEN 4096
int
checkSum(unsigned int size, char* buf) {
	unsigned int cnt = 0;
	char* it = buf;
	while (*it != '\n') {
		cnt++;
		it++;
	}
	return size == cnt;
}
int main(int argc, char **argv)
{
	int sd, client_len, port, n;
	char buf[MAXLEN];
	struct sockaddr_in server, client;
	const char* check_message = "ftp";
	const char* yes_message = "yes";
	const char* no_message = "no";
  const char* delim = ":";
	if (argc == 2) {
		port = atoi(argv[1]);
	} else {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Can't create a socket\n");
		exit(1);
	}

	bzero((char*)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	while (1) {
		client_len = sizeof(client);
		if ((n = recvfrom(sd, buf, MAXLEN, 0, (struct sockaddr *)&client, &client_len)) < 0) {
		    fprintf(stderr, "Can't receive datagram\n");
		    exit(1);
		}
		printf("receive %d bytes\n", n);
		unsigned int total_frag = atoi(strtok(buf, delim));
		unsigned int frag_no = atoi(strtok(NULL, delim));
		unsigned int size = atoi(strtok(NULL, delim));
		char* filename = strtok(NULL, delim);
		char* filedata = strtok(NULL, delim);
		printf("%u %u %u %s %s\n", total_frag, frag_no, size, filename, filedata);
		if (checkSum(size, filedata)) {
			char ack_message[50];
			sprintf(ack_message, "ACK %u", size);
			if (sendto(sd, ack_message, sizeof(ack_message), 0, (struct sockaddr *)&client, client_len) != sizeof(yes_message)) {
			fprintf(stderr, "Can't send datagram\n");
			exit(1);
			}
		} else {
			printf("no check\n");
		}
		/*if (strcmp(buf, check_message) == 0) {
			if (sendto(sd, yes_message, sizeof(yes_message), 0, (struct sockaddr *)&client, client_len) != sizeof(yes_message)) {
			fprintf(stderr, "Can't send datagram\n");
			exit(1);
			}
		} else {
			if (sendto(sd, no_message, sizeof(no_message), 0, (struct sockaddr *)&client, client_len) != sizeof(no_message)) {
			fprintf(stderr, "Can't send datagram\n");
			exit(1);
			}
		}*/
	}
	close(sd);
	return (0);
}
