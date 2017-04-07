/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Clients.h
 * Author: litian17
 *
 * Created on March 24, 2017, 10:01 AM
 */

#ifndef CLIENTS_H
#define CLIENTS_H


#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#define MAX_NAME 1024
#define MAX_DATA 1024
#define MAX_BACKLOG 1024

using namespace std;


enum MessageTypes {LOGIN, LO_ACK, LO_NAK, EXIT, JOIN, JN_ACK, JN_NAK, 
            LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK, REG, REG_ACK, REG_NAK};

// by ECE361 protocol
struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};


class Client {
public:
    Client(string username, string _password) : id(username), password(_password) {;}
    string id;
    string password;
    bool online = false;
    bool banned = false;
    unordered_set<int> sess_ids;
    int sockfd;
    struct sockaddr_in client_addr;
};


class Conference {
public:
    Conference() {;}
    Conference(int id) : sess_id(id) {;}
    void add_client(string username, Client* client_p) { clients.insert(make_pair(username, client_p)); }
    void rm_client(string username) { if (clients.find(username) != clients.end()) clients.erase(username); } 
    bool is_empty() { return clients.empty(); }
    vector<string> get_client_names();
    
    // sends the message to all clients involved
    void multicast_message(string message, Client* client_p);
    
private:
    int sess_id;
    unordered_map<string, Client*> clients;
};


// HELPER FUNCTIONS 
lab3message form_message(int type, string source, string data);

// forms and sends message in ECE361 protocol format through specified socket
int send_message(int type, string source, string data, int sockfd);


#endif /* CLIENTS_H */

