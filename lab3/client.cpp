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
#include <bits/stdc++.h>


using namespace std;
#define MAX_NAME        1024
#define MAX_DATA        1024
#define LOGIN           0
#define LO_ACK          1
#define LO_NAK          2
#define EXIT            3
#define JOIN            4
#define JN_ACK          5
#define JN_NAK          6    
#define LEAVE_SESS      7
#define NEW_SESS        8
#define NS_ACK          9
#define MESSAGE         10
#define QUERY           11
#define QU_ACK          12

struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
    lab3message() {}
    lab3message(unsigned int type_, const char* s, unsigned int s_size, const char* d, unsigned int d_size) {
        type = type_;
        bcopy(s, source, s_size);
        bcopy(d, data, d_size);
        size = d_size;
    }
};

void helper_func(void) {
    cout << "/login <client ID> <password> <server-IP> <server-port>" << endl;
    cout << "/logout Exit the server" << endl;
    cout << "/joinsession <session ID>" << endl;
    cout << "/leavesession" << endl;
    cout << "/createsession <session ID> " << endl;
    cout << "/list" << endl;
    cout << "/quit" << endl;
    cout << "<text>" << endl;
}

int sock;
void login_func(void) {
    string client_id;
    string password;
    string server_IP;
    int server_port;
    cin >> client_id >> password >> server_IP >> server_port;
    int len, port=server_port;
    const char *host=server_IP.c_str();
    struct sockaddr_in addr;
    struct hostent *host_entry;
    host_entry = gethostbyname(host);

    if (!host_entry){
      fprintf(stderr,"Couldn't resolve host");
      exit(0);
    }

    memset(&addr,0,sizeof(addr));
    addr.sin_addr=*(struct in_addr *) host_entry->h_addr_list[0];
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);

    printf("Connecting to %s(%s):%d\n", host, inet_ntoa(addr.sin_addr),port);
    if((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
    	perror("socket");
    if(connect(sock,(struct sockaddr *)&addr, sizeof(addr))<0)
    	perror("connect");

    lab3message message(LOGIN, client_id.c_str(), client_id.size(), password.c_str(), password.size());
    send(sock, &message, sizeof(message),0);
    lab3message recv_message;
    len = recv(sock, &recv_message, sizeof(lab3message), 0);
    if (len < 0) {
	printf("Connection error, please try again!\n");
    } else {
    	if (recv_message.type == LO_ACK) {
		printf("Login successful!\n");
    	}
	else if (recv_message.type == LO_NAK){
		printf("Login fail, please try again\n");
    	}
	else {
		printf("Server is broken\n");
	}
    }
}


    
void input(void) {
    string command;
    cin >> command;
    if (command == "/help") helper_func();
    else if (command == "/login") login_func();
}
    
int main(int argc, char **argv)
{	
	while (1) {
		input();
	}
	return 0;
}
