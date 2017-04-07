/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Server.cpp
 * Author: litian17
 * 
 * Created on March 24, 2017, 9:58 AM
 */

#include "Server.h"

Server::Server(string _db_filename)
{
    db_filename = _db_filename;
    ifstream client_info (db_filename);
    string line;
    if (client_info.is_open())
    {
        cout << "loading client usernames and passwords from " << db_filename << endl ;
        while (getline (client_info,line))
        {
            string username, password;
            stringstream ss(line);
            ss >> username >> password;
            client_list.insert(make_pair(username, Client(username, password)));
        }
        client_info.close();
    }
}


// cmd_offset specifies how much command line args are offset (1 if running in netbeans debug)
int Server::init_server(int argc, char** argv, int cmd_offset= 0)
{
    // port number is provided by user
    if (argc < 2 + cmd_offset || argv[1 + cmd_offset] == NULL)
    {
        cout << "usage: server [port number]" << endl ;
        return -1;
    }
    int port = atoi(argv[1 + cmd_offset]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cout << "unable to create socket" << endl ;
        return -1;
    }
    
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    const struct sockaddr* server_addr_p = (sockaddr*)&server_addr;
    
    if (bind(sockfd, server_addr_p , sizeof(struct sockaddr)) < 0)
    {
        cout << "unable to assign addr to socket" << endl ;
        return -1;
    }
    if (listen(sockfd, MAX_BACKLOG) < 0)
    {
        cout << "unable to listen at socket" << endl ;
        return -1;
    }
    
    // ignore SIGPIPE errors (default kills process when writing to broken pipe)
    signal(SIGPIPE, SIG_IGN);
    cout << "hosting server at port " << port << endl << endl;
    return sockfd;
}


int Server::run_server()
{
    struct sockaddr_in client_addr;
    unsigned int clientlen = sizeof(client_addr);
    while (1) 
    {
        // extract a connection request from sockfd's queue
        int childfd = accept(sockfd, (struct sockaddr *) &client_addr, &clientlen);
        if (childfd < 0)
        {
            cout << "could not create a new socket for connection" << endl ;
            return -1;
        }
        struct hostent* hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr,
                                               sizeof(client_addr.sin_addr.s_addr), AF_INET);
        
        // run new thread just for this new connection
        thread handler([=] { handle_connection(childfd, client_addr); });
        handler.detach();
    }
}


void Server::handle_connection(int sockfd, struct sockaddr_in client_addr)
{
    // all messages should be in the format of the given struct
    lab3message msg_buf;
    Client* client_p = NULL;
    
    while(1)
    {
        memset((void*)&msg_buf, 0, sizeof(lab3message));
        int read_size = recv(sockfd, (void*)&msg_buf , sizeof(lab3message), 0);
        
        // if the client exits, read_size = 0
        // if unexpected error from client, read_size = -1
        if (read_size <= 0)
        {
            if (client_p != NULL)
                handle_exit(client_p);
            return;
        }
        
        // handle message, status = -1 indicates connection should be terminated
        int status = handle_message(msg_buf, sockfd, client_p);
        if (status < 0)
        {
            // no need calling handle_exit if client was never logged in
            if (status < 0 && client_p != NULL)
                handle_exit(client_p);
            return;
        }
    }
}


int Server::handle_message(const lab3message& message, int sockfd, Client*& client_p)
{
    int type = message.type;
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    string data(s2.str());
    int status = 0;
    string reply;
    
    // check if authenticated
    if (client_p == NULL)
    {
        if (type == LOGIN)
        {
            status = handle_login(message, sockfd, client_p);
        }
        else if (type == REG)
        {
            status = handle_reg(message, sockfd, client_p);
        }
        else
        {
            cout << "denied requests from unauthenticated source" << endl ;
            return -1;
        }
    }
    else if (type == EXIT)
    {
        status = handle_exit(client_p);
    }
    else if (type == JOIN)
    {
        status = handle_conf_join(message, client_p);
    }
    else if (type == LEAVE_SESS)
    {
        status = handle_conf_leave(message, client_p);
    }
    else if (type == NEW_SESS)
    {
        status = handle_conf_create(message, client_p);
    }
    else if (type == MESSAGE)
    {
        handle_conf_message(message, client_p);
    }
    else if (type == QUERY)
    {
        status = handle_query(client_p);
    }
    cout << endl;
    return status;
}


int Server::handle_conf_message(const lab3message& message, Client*& client)
{
    // data should contain message type and message itself, separated by a space
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    
    string str_targ_sess, str_message;
    s2 >> str_targ_sess;
    getline(s2, str_message);
    str_message = "from " + client->id + " : " + str_message;
    
    // message must begin with @
    if (str_targ_sess[0] != '@')
    {
        string reply = "please use @all or @yoursessionid to message other clients";
        send_message(MESSAGE, name, reply, client->sockfd);
        return 0;
    }

    if (str_targ_sess == "@all")
    {
        for (unordered_set<int>::iterator it = client->sess_ids.begin(); it != client->sess_ids.end(); ++it)
        {
            int cur_sess = *it;
            conferences[cur_sess].multicast_message(str_message, client);
        }
    }
    else
    {
        // check if str_targ_sess specifies a session
        int targ_sess;
        stringstream int_test(str_targ_sess.substr(1, string::npos));
        int_test >> targ_sess;
        
        if (!int_test.fail())
        {
            // check if targ_sess is one of the sessions that client is part of
            if (client->sess_ids.find(targ_sess) == client->sess_ids.end())
            {
                cout << "MESSAGE: client attempted to message to session " << targ_sess << ", which it is not part of" << endl;
                string reply = "you are not part of session " + to_string(targ_sess);
                send_message(MESSAGE, name, reply, client->sockfd);
            }
            else
            {
                conferences[targ_sess].multicast_message(str_message, client);
            }
        }
    }
    return 0;
}


int Server::handle_reg(const lab3message& message, int sockfd, Client*& client_p)
{
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    string password(s2.str());
    
    if (client_list.find(username) != client_list.end())
    {
        cout << "REG: " << username << " is already a registered user" << endl;
        string reply = "already registered";
        send_message(REG_NAK, name, reply, sockfd);
        return -1;
    }
    
    // register user
    client_list.insert(make_pair(username, Client(username, password)));
    send_message(REG_ACK, name, "", sockfd);
    
    // write to persistent memory
    ofstream outfile;
    outfile.open(db_filename, std::ios_base::app);
    outfile << username << " " << password << endl;
    
    // break connection
    return -1;
}


int Server::handle_login(const lab3message& message, int sockfd, Client*& client_p)
{
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    string data(s2.str());
    string reply;
    
    // check if client ID exists, and if logged on already
    cout << "LOGIN: " << username << " is attempting to login: " << endl ;
    unordered_map<string, Client>::iterator p = client_list.find(username);
    if (p == client_list.end())
    {
        // send message
        reply = "not a valid username ";
        cout << reply << endl ;
        send_message(LO_NAK, name, reply, sockfd);
        
        // break connection
        return -1;
    }

    // check if password is correct
    cout << "   correct password is: " << p->second.password << endl ;
    cout << "   given password is: " << data << endl ;
    if (p->second.password == data)
    {
        if (p->second.online == true)
        {
            // send message 
            reply = "this client is already logged in";
            cout << "   " << reply << endl ;
            send_message(LO_NAK, name, reply, sockfd);
            return -1;
        }
        cout << "   successfully logged in" << endl ;
        p->second.online = true;
        p->second.sockfd = sockfd;

        // set pointer to the client
        client_p = &(p->second);
        
        // acknowledge login (no need for reply data)
        return send_message(LO_ACK, name, "hello", sockfd);
    }
    else
    {
        reply = "incorrect password provided";
        cout << "   " <<  reply << endl ;
        send_message(LO_NAK, name, reply, sockfd);
        return -1;
    }
    return 0;
}


int Server::handle_exit(Client*& client_p)
{
    cout << "EXIT: " << client_p->id << " is logging out ... " << endl ;
        
    // leave the conference session that client is a part of
    handle_conf_leave_all(client_p);

    client_p->sess_ids.clear();
    client_p->online = false;
    client_p = NULL;
    return -1;
}


int Server::handle_conf_create(const lab3message& message, Client* client_p)
{
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    string data(s2.str());
    string reply;
    int targ_sess = stoi(data);
    cout << "NEW_SESS: " << client_p->id << " ";
    
    if (conferences.find(targ_sess) != conferences.end())
    {
        cout << "attempted to create " << targ_sess << " but it already exists" << endl ;
        return 0;
    }
    
    client_p->sess_ids.insert(targ_sess);
    Conference c(targ_sess);
    conferences.insert(make_pair(targ_sess, c));
    conferences[targ_sess].add_client(username, client_p);
    reply = to_string(targ_sess);
    cout << "created and joined conference session " << reply << endl ;
    return send_message(NS_ACK, name, reply, client_p->sockfd);
}


int Server::handle_conf_join(const lab3message& message, Client* client_p)
{
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    string data(s2.str());
    string reply;
    cout << "JOIN: " << client_p->id << " ";
    
    // check if conference session exists
    int targ_sess = stoi(data);
    if (client_p->sess_ids.find(targ_sess) != client_p->sess_ids.end())
    {
        reply = to_string(targ_sess) + ":already part of this conference session";
        cout << " is already part of conference session " << targ_sess << endl;
        return send_message(JN_NAK, name, reply, client_p->sockfd);
    }
    if (conferences.find(targ_sess) == conferences.end())
    {
        reply =  to_string(targ_sess) + ":the specified conference session does not exist";
        cout << " requested to join conference session " << targ_sess << ", which does not exist" << endl ;
        return send_message(JN_NAK, name, reply, client_p->sockfd);
    }

    client_p->sess_ids.insert(targ_sess);
    conferences[targ_sess].add_client(client_p->id , client_p);
    reply = to_string(targ_sess);
    cout << "joined session " << reply << endl ;
    return send_message(JN_ACK, name, reply, client_p->sockfd);
}


int Server::handle_conf_leave_all(Client* client_p)
{
    if (client_p->sess_ids.empty())
    {
        return 0;
    }
    
    // remove client from all conference sessions that it was part of
    for (unordered_set<int>::iterator it = client_p->sess_ids.begin(); it != client_p->sess_ids.end(); ++it)
    {
        int cur_sess = *it;
        conferences[cur_sess].rm_client(client_p->id);
        
        // if the conference has no more clients then erase
        if (conferences[cur_sess].is_empty())
        {
            conferences.erase(cur_sess);
        }
    }
   
    client_p->sess_ids.clear();
    return 0;
}


int Server::handle_conf_leave(const lab3message& message, Client* client_p)
{
    stringstream s1, s2;
    s1 << message.source;
    s2 << message.data;
    string username(s1.str());
    int targ_sess;
    s2 >> targ_sess;
    
    if (s2.fail())
    {
        return 0;
    }
    
    if (client_p->sess_ids.find(targ_sess) != client_p->sess_ids.end())
    {
        client_p->sess_ids.erase(targ_sess);
        conferences[targ_sess].rm_client(client_p->id);
        if (conferences[targ_sess].is_empty())
        {
            conferences.erase(targ_sess);
        }
        cout << "LEAVE_SESS: " + username + " has left " << targ_sess << endl;
    }
    else
    {
        cout << "LEAVE_SESS: " + username + " attempted to leave " << targ_sess << " but was never part of it" << endl;
    }
    
    return 0;
}


int Server::handle_query(Client* client_p)
{
    // list of all users, followed by all available sessions
    string reply;
    cout << endl  << "QUERY:" << endl ;
    cout << "------------------------" << endl ;
    cout << "LONERS: " << endl;
    reply += "loners : ";
    for (unordered_map<string, Client>::iterator it = client_list.begin(); it != client_list.end(); ++it)
    {
        if (it->second.sess_ids.empty() && it->second.online)
        {
            cout << it->second.id + " ";
            reply += it->second.id + " ";
        }
    }
    reply += "\n";
    cout << endl;
    
    cout << "ALL SESSIONS AND USERS:" << endl ;
    for (unordered_map<int, Conference>::iterator it = conferences.begin(); it != conferences.end(); ++it)
    {
        cout << " part of session " << it->first << " : ";
        reply += to_string(it->first) + " : ";
        vector<string> names = it->second.get_client_names();
        for (int i = 0; i < names.size(); ++i)
        {
            cout << names[i] + " ";
            reply += names[i] + " ";
        }
        reply += "\n";
        cout << endl;
    }
    cout << "------------------------" << endl ;
    cout << endl ;
    return send_message(QU_ACK, name, reply, client_p->sockfd);
}



