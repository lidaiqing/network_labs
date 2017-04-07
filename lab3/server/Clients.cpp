/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Clients.cpp
 * Author: litian17
 * 
 * Created on March 24, 2017, 10:01 AM
 */

#include "Clients.h"


void Conference::multicast_message(string message, Client* client_p)
{
    cout << "MESSAGE: " << client_p->id << " broadcasting to session " << sess_id << " with message: '" << message << "'" << endl ;
    for (unordered_map<string, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client* cur = it->second;
        if (cur != client_p)
        {
            cout << "   sending to " << cur->id << endl ;
            send_message(MESSAGE, client_p->id, message, cur->sockfd);
        }
    }
}


vector<string> Conference::get_client_names()
{
    vector<string> names;
    for (unordered_map<string, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        string name = it->first;
        names.push_back(name);
    }
    return names;
}


// HELPERS
lab3message form_message(int type, string source, string data)
{
    if (data.size() > MAX_DATA)
    {
        cout << "send_message:: The given data does not fit" << endl ;
        lab3message temp;
        return temp;
    }
    lab3message message;
    message.type = type;
    unsigned char* usource = (unsigned char*)source.c_str();
    unsigned char* udata = (unsigned char*)data.c_str();
    memset(message.source, 0, sizeof(message.source));
    memset(message.data, 0, sizeof(message.data));
    memcpy(message.source, usource, source.size());
    memcpy(message.data, udata, data.size());
    return message;
}


int send_message(int type, string source, string data, int sockfd)
{
    lab3message message = form_message(type, source, data);
    return send(sockfd , (void*)&message , sizeof(lab3message), 0);
}