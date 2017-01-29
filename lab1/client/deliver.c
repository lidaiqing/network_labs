#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLEN 4096

long cal_delay(struct timeval t1, struct timeval t2);

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
    inet_pton(AF_INET, host, &(server.sin_addr));
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
    struct timeval start, end;
	printf("Please input ftp <file name> \n");
	scanf("%79s %239s", protocol, file_name);
	if (access(file_name, F_OK) != -1) {
	    sbuf = "ftp";
        // start measuring
        gettimeofday(&start, NULL);
		if (sendto(sd, sbuf, sizeof(sbuf), 0, (struct sockaddr*)&server, server_len) == -1) {
			fprintf(stderr, "sendto error\n");
			exit(1);
		}
		 
		if (recvfrom(sd, rbuf, MAXLEN, 0, (struct sockaddr *)&server, &server_len) < 0) {
		    fprintf(stderr, "recvfrom error\n");
		    exit(1);
		}
        gettimeofday(&end, NULL);
		if (strcmp(rbuf, "yes") == 0) {
			printf("A file transfer can start\n");
		} else {
			printf("Error from server\n");
			exit(1);
		}
        printf("Round-trip delay = %ld ms.\n", cal_delay(start, end));
	} else {
		printf("File not exist\n");
		exit(1);
	}
	close(sd);
	return 0;
}

long cal_delay (struct timeval t1, struct timeval t2)
{
    long d;

    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
    return d;
}
