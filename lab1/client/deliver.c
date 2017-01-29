#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAXLEN 4096

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s [address] [port]", argv[0]);
		exit(1);
	}
	char* host = argv[1];
	int port = atoi(argv[2]);
	int sd, server_len;
	struct sockaddr_in server, client;
	struct hostent* hp;
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Can't create a socket\n");
		exit(1);
	}
	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyaddr(host, sizeof(host), AF_INET)) == NULL) {
		fprintf(stderr, "Can't get IP\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	printf("%s\n", hp->h_name);
	bzero((char*)&client, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_port = htons(0);
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&client, sizeof(client)) == -1) {
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}
	server_len = sizeof(server);

	char protocol[80], file_name[240];
	char *sbuf;
	char rbuf[MAXLEN];
	printf("Please input ftp <file name> \n");
	scanf("%79s %239s", protocol, file_name);
	if (access(file_name, F_OK) != -1) {
		sbuf = "ftp";
		printf("sending %s\n", sbuf);
		int sendByte;
		if ((sendByte = sendto(sd, sbuf, sizeof(sbuf), 0, (struct sockaddr*)&server, server_len)) == -1) {
			fprintf(stderr, "sendto error\n");
			exit(1);
		}
		printf("send %d\n", sendByte);	
		 
		if (recvfrom(sd, rbuf, MAXLEN, 0, (struct sockaddr *)&server, &server_len) < 0) {
		fprintf(stderr, "recvfrom error\n");
		exit(1);
		}
		if (strcmp(rbuf, "yes") == 0) {
			printf("A file transfer can start\n");
		} else {
			printf("Error from server\n");
			exit(1);
		}
	} else {
		printf("File not exist\n");
		exit(1);
	}
	close(sd);
	return 0;
}
