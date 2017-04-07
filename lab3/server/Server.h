/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Server.h
 * Author: litian17
 *
 * Created on March 24, 2017, 9:58 AM
 */

#ifndef SERVER_H
#define SERVER_H

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
#include <unordered_map>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#include "Clients.h"
#define MAX_NAME 1024
#define MAX_DATA 1024
#define MAX_BACKLOG 1024

            
            
// server-client bindings are kept through the socket that is established at connection
class Server {
public:
    Server() {;}
    Server(string db_filename);  // load client_list from .txt file, each row has username and password (separated by spaces)
    string db_filename;
    string name;   // server ID
    int sockfd;
    struct sockaddr_in server_addr;
    
    unordered_map<int, Conference> conferences;
    unordered_map<string, Client> client_list;
    int init_server(int argc, char** argv, int cmd_offset);
    int run_server();
    
    // sockfd is the socket used for the current TCP connection
    void handle_connection(int sockfd, struct sockaddr_in client_addr);
    
    // if client is NULL then source has not yet been authenticated
    int handle_message(const lab3message& message, int sockfd, Client*& client);
    
    // helper functions to handle_message
    int handle_reg(const lab3message& message, int sockfd, Client*& client_p);
        
    // login modifies client pointer to the client object within 'client_list', if found
    int handle_login(const lab3message& message, int sockfd, Client*& client_p);
    int handle_exit(Client*& client_p);
    int handle_conf_create(const lab3message& message, Client* client_p);
    int handle_conf_join(const lab3message& message, Client* client_p);
    int handle_conf_leave_all(Client* client_p);
    int handle_conf_leave(const lab3message& message, Client* client_p);
    int handle_query(Client* client_p);
    int handle_conf_message(const lab3message& message, Client*& client);
};

#endif /* SERVER_H */

