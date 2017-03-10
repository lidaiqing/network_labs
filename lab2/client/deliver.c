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
#include <signal.h>

#define MAXLEN 4096
#define DATASIZE 1000
int timeout_flag;
struct packet {
		unsigned int total_frag;
		unsigned int frag_no;
		unsigned int size;
		char* filename;
		char filedata[DATASIZE];
};

long cal_delay (struct timeval t1, struct timeval t2)
{
    long d;
    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
    return d;
}

void timeout_handler(int signum)
{
	printf ("Timer time out, resend the package\n");
	timeout_flag = 1;
}
struct packet*
build_packet(unsigned int _total_frag, unsigned int _frag_no, unsigned int _size, char *_filename, char* _filedata) {
	struct packet* p = (struct packet*)malloc(sizeof(struct packet));
	p->total_frag = _total_frag;
	p->frag_no = _frag_no;
	p->size = _size;
	p->filename = _filename;
	int i;
	for (i = 0; i < _size; i++)
		p->filedata[i] = _filedata[i];
	return p;
}
int
prepare_packet(struct packet* p, char sendbuf[]) {
	int sz = 0;
    sz += sprintf(sendbuf, "%u:%u:%u:%s:", p->total_frag, p->frag_no, p->size, p->filename);
    int i;
    memcpy(sendbuf + sz, p->filedata, p->size);
    sz += p->size;
	return sz;
}
int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s [address] [port]\n", argv[0]);
		exit(1);
	}
	char* host = argv[1];
	int port = atoi(argv[2]);
	int sd, server_len;
	struct sockaddr_in server, client;
	struct hostent* hp;
	struct itimerval timer;
	struct sigaction sa;
	timer.it_value.tv_sec = 10;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;

	sa.sa_handler = &timeout_handler;
	sigaction(SIGALRM, &sa, 0);

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
	char sbuf[DATASIZE * 4];
	char rbuf[MAXLEN];
  struct timeval start, end;
	printf("Please input ftp <file name> \n");
	scanf("%79s %239s", protocol, file_name);
	if (access(file_name, F_OK) != -1) {
	    FILE *fp;
			fp = fopen(file_name, "r");
			fseek(fp, 0L, SEEK_END);
			int sz = ftell(fp);
			printf("Total file size %d\n", sz);
			rewind(fp);
			int npackets = sz / DATASIZE;
			npackets = (sz % DATASIZE == 0 ? npackets : npackets + 1);
			unsigned int cur_npacket = 1;
			char* databuf = (char*)malloc(sizeof(char) * DATASIZE);
			size_t bytes_read;
			while (bytes_read = fread(databuf, 1, DATASIZE, fp)) {
				printf("bytes read %lu\n", bytes_read);
				struct packet *p = build_packet(npackets, cur_npacket, bytes_read, file_name, databuf);
				int buf_sz = prepare_packet(p, sbuf);
				printf("sending buffer size %d\n", buf_sz);
				int timeout_counter = 6;
				while (timeout_counter--) {
					if (sendto(sd, sbuf, buf_sz, 0, (struct sockaddr*)&server, server_len) == -1) {
						fprintf(stderr, "sendto error\n");
						exit(1);
					}
					setitimer(ITIMER_REAL, &timer, 0);
					timeout_flag = 0;
					if (recvfrom(sd, rbuf, MAXLEN, 0, (struct sockaddr *)&server, &server_len) < 0) {
						  if (timeout_flag == 1) continue;
							fprintf(stderr, "recvfrom error\n");
							exit(1);
						}
					if (timeout_flag == 0) {
						setitimer (ITIMER_REAL, 0, 0);
						char* ackChecker;
						asprintf(&ackChecker, "ACK %u", bytes_read);
						if (strcmp(ackChecker, rbuf) != 0) {
							fprintf(stderr, "package lost\n");
							exit(1);
						}
						cur_npacket++;
						break;
					}
				}
		}
			fclose(fp);
	} else {
		fprintf(stderr, "File not exist\n");
		exit(1);
	}
	close(sd);
	return 0;
}
