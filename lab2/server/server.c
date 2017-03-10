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
		//printf("%c\n", *it);
		cnt++;
		it++;
	}
	printf("buf size %u\n", cnt);
	return size == (cnt + 1);
}
int main(int argc, char **argv)
{
	int sd, client_len, port, n;
	char buf[MAXLEN];
	struct sockaddr_in server, client;
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
  FILE *fp;
	int ack_counter = 0;
	while (1) {
		client_len = sizeof(client);
		if ((n = recvfrom(sd, buf, MAXLEN, 0, (struct sockaddr *)&client, &client_len)) < 0) {
		    fprintf(stderr, "Can't receive datagram\n");
		    exit(1);
		}
		printf("receive %d bytes\n", n);
        char* bufcpy = malloc(sizeof(buf));
        memcpy(bufcpy, buf, sizeof(buf));
		unsigned int total_frag = atoi(strtok(bufcpy, delim));
		unsigned int frag_no = atoi(strtok(NULL, delim));
		unsigned int size = atoi(strtok(NULL, delim));
		char* filename = strtok(NULL, delim);
		char* filedata = buf + (n - size);
		//if (frag_no == 1) printf("%u %u %u %s %s\n", total_frag, frag_no, size, filename, filedata);
		if (frag_no == 1) {
			fp = fopen(filename, "w");
			printf("Creating file %s\n", filename);
		}
		if (frag_no == 2 && ack_counter < 5) {
			printf("Ack counter for frag 2 is %d\n", ack_counter);
			ack_counter++;
			continue;
		}
		printf("Writing frag %u into file, size %u\n", frag_no, size);
		fwrite(filedata, 1, size, fp);
		if (frag_no == total_frag) {
			fseek(fp, 0L, SEEK_END);
			int sz = ftell(fp);
			printf("Total file size written %d\n", sz);
			printf("Closing file\n");
			fclose(fp);
		}
		char* ack_message;
		asprintf(&ack_message, "ACK %u", size);
		if (sendto(sd, ack_message, sizeof(ack_message), 0, (struct sockaddr *)&client, client_len) != sizeof(ack_message)) {
		fprintf(stderr, "Can't send datagram\n");
		exit(1);
		}
 }
	close(sd);
	return (0);
}
