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
#include <pthread.h>

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
	source[s_size] = '\0';
        bcopy(d, data, d_size);
	data[d_size] = '\0';
        size = d_size;
    }
};
string client_id;
pthread_t message_thread;
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
    string password;
    string server_IP;
    int server_port;
    cin >> client_id >> password >> server_IP >> server_port;
    if (sock != -1) {
	cout << "You have logged in already!" << endl;
	return;
    }
	struct timeval tv;
	tv.tv_sec = 2;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
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
    	//setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));
    if(connect(sock,(struct sockaddr *)&addr, sizeof(addr))<0)
    	perror("connect");

    lab3message message(LOGIN, client_id.c_str(), client_id.size(), password.c_str(), password.size());
    send(sock, &message, sizeof(message),0);
}

void logout_func(void) {
    if (sock == -1) {
	printf("You have already logged out!");
	return;
    }
    lab3message message(EXIT, client_id.c_str(), client_id.size(), 0, 0);
    send(sock, &message, sizeof(message),0);
    close(sock);
    sock = -1;
    printf("Logout successful!\n");
}

void joinsession_func(void) {
    string session_id;
    cin >> session_id;
    lab3message message(JOIN, client_id.c_str(), client_id.size(), session_id.c_str(), session_id.size());
    send(sock, &message, sizeof(message),0);
}

void leavesession_func(void) {
  lab3message message(LEAVE_SESS, client_id.c_str(), client_id.size(), 0, 0);
  send(sock, &message, sizeof(message),0);
  printf("Leave current session successful!\n");
}

void createsession_func(void) {
  string session_id;
  cin >> session_id;
  lab3message message(NEW_SESS, client_id.c_str(), client_id.size(), session_id.c_str(), session_id.size());
  send(sock, &message, sizeof(message),0);
}

void getlist_func(void) {
  lab3message message(QUERY, client_id.c_str(), client_id.size(), 0, 0);
  send(sock, &message, sizeof(message),0);
}

void send_message_func(string input) {
  lab3message message(MESSAGE, client_id.c_str(), client_id.size(), input.c_str(), input.size());
  send(sock, &message, sizeof(message),0);
  printf("Send message: %s\n", input.c_str());
}

void* recv_message_func(void*) {
   while(1) {
    if (sock == -1) continue;
    lab3message recv_message;
    int len = recv(sock, &recv_message, sizeof(lab3message), 0);
      if (recv_message.type == MESSAGE) {
        printf("Recv message: %s\n", recv_message.data);
      }
	else if (recv_message.type == LO_ACK) {
		printf("Login successful!\n");
    	}
	    else if (recv_message.type == LO_NAK){
		printf("Login fail, please try again!\n");
		sock = -1;
    	}
	    else if (recv_message.type == JN_ACK) {
		printf("Join session successful!\n");
    	}
	    else if (recv_message.type == JN_NAK){
		printf("Join session fail, reason: %s, please try again!\n", recv_message.data);
    	}
	   else if (recv_message.type == NS_ACK) {
  		printf("Create session successful!\n");
    	}
	else if (recv_message.type == QU_ACK) {
      		printf("%s\n", recv_message.data);
    	}
	    else {
		printf("Operation fail, please try again!\n");
	    }
     
   }
}

void quit_func(void) {
    pthread_cancel(message_thread);
    exit(0);
}
void input(void) {
    string command;
    cin >> command;
    if (command == "/help") helper_func();
    else if (command == "/login") login_func();
    else if (command == "/logout") logout_func();
    else if (command == "/joinsession") joinsession_func();
    else if (command == "/leavesession") leavesession_func();
    else if (command == "/createsession") createsession_func();
    else if (command == "/list") getlist_func();
    else if (command == "/quit") quit_func();
    else send_message_func(command);
}

int main(int argc, char **argv)
{

  int rc = pthread_create(&message_thread, NULL, recv_message_func, NULL);
  if (rc) {
    perror("Message thread");
    exit(0);
  }
  sock = -1;
	while (1) {
		input();
	}
	return 0;
}
