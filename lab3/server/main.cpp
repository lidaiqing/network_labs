/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: litian17
 *
 * Created on March 13, 2017, 7:35 PM
 */

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
#include "Server.h"
#define MAX_NAME 1024
#define MAX_DATA 1024
#define MAX_BACKLOG 1024

using namespace std;

int main(int argc, char** argv) {
    Server server("client_info.txt");
    int sockfd = server.init_server(argc, argv, 0);
    int exit_status = server.run_server();
    return 0;
}

