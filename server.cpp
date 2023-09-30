#include "helpers.h"

void run_server(int sockfd, int udp_sockfd, struct sockaddr_in udp_addr)
{
    // polling file descriptors
    struct pollfd pfds[MAX_CONNECTIONS];
    pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
    pfds[1].fd = udp_sockfd;
	pfds[1].events = POLLIN;
    pfds[2].fd = sockfd;
	pfds[2].events = POLLIN;
    // number of file descriptors
	int nfds = 3;

    // auxiliary variables
    int rc, newsockfd;
    char buf[BUFMAX];
    struct sockaddr_in tcp_addr;
    struct tcp_msg tcp_m;
    struct udp_msg *udp_m;
    socklen_t size = sizeof(struct sockaddr);
    uint32_t int32;
    uint16_t int16;
    uint8_t int8;
    float short_real, nr_float;
    short sign;
    struct cmd *m;

    // more auxiliary variables
    vector<Subscriber> clients;
    bool already;
    unordered_map<string, uint8_t> topics;
    queue<struct tcp_msg> pending;

    // polling
    while (1) {
        rc = poll(pfds, nfds, -1);
        DIE(rc < 0, "poll error");

        for (int i = 0; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                memset(buf, 0, sizeof(buf));
                // commands from stdin
                if (pfds[i].fd == STDIN_FILENO) {
                    fgets(buf, BUFMAX, stdin);
                    DIE(strcmp(buf, "exit\n") != 0, 
                        "unknown commmand");
                    // the server and all connections are closed
                    for (int j = 1; j < nfds; j++)
                        close(pfds[j].fd);

                    return;
                } else if (pfds[i].fd == sockfd) {
                    // new subscriber establishes a connection
                    newsockfd = accept(sockfd, (struct sockaddr *) &tcp_addr,
                        &size);
                    DIE(newsockfd < 0, "accept error");
                    rc = recv(newsockfd, buf, ID_SIZE, 0);
                    DIE(rc < 0, "recv error");

                    string id(buf);
                    already = false;

                    for (auto &client : clients)
                        if (client.getId() == id) {
                            already = true;
                            // client was already connected
                            if (client.getConnected()) {
                                cout << "Client " << id 
                                    << " already connected.\n";
                                client.setConnected(false);
                                close(newsockfd);
                                close(client.getSockfd());

                                for (int j = i; j < nfds - 1; j++)
                                    pfds[j] = pfds[j + 1];
                                nfds--;
                                i--;
                            } else {
                                client.setSockfd(newsockfd);
                                client.setConnected(true);
                                cout << "New client " << id 
                                    << " connected from "
                                    << inet_ntoa(tcp_addr.sin_addr) << ":"
                                    << ntohs(tcp_addr.sin_port) << ".\n";

                                pfds[nfds].fd = newsockfd;
                                pfds[nfds].events = POLLIN;
                                nfds++;
                                // display pending messages while being off
                                pending = client.getPending();
                                while (!pending.empty()) {
                                    tcp_m = pending.front();
                                    rc = send(client.getSockfd(), &tcp_m,
                                        sizeof(struct tcp_msg), 0);
                                    DIE(rc < 0, "send error");
                                    pending.pop();
                                }
                                client.setPending(pending);
                            }

                            break;
                        }
                    // brand new subscriber
                    if (!already) {
                        Subscriber newClient(id, newsockfd);
                        clients.push_back(newClient);
                        cout << "New client " << id << " connected from "
                            << inet_ntoa(tcp_addr.sin_addr) << ":" 
                            << ntohs(tcp_addr.sin_port) << ".\n";

                        pfds[nfds].fd = newsockfd;
                        pfds[nfds].events = POLLIN;
                        nfds++;
                    }
                } else if (pfds[i].fd == udp_sockfd) {
                    // incoming data from UDP clients
                    rc = recvfrom(udp_sockfd, buf, BUFMAX - 1, 0,
                        (struct sockaddr*) &udp_addr, &size);
                    DIE(rc < 0, "recvfrom error");
                    udp_m = (struct udp_msg *)buf;

                    memset(&tcp_m, 0, sizeof(tcp_m));
                    strcpy(tcp_m.ip_udp, inet_ntoa(udp_addr.sin_addr));
                    tcp_m.port_udp = htons(udp_addr.sin_port);
                    strcpy(tcp_m.topic, udp_m->topic);

                    // processing input accordingly
                    switch (udp_m->type) {
                    case INT_VAL:
                        memcpy(&int32, udp_m->content + SIGN_SIZE,
                            sizeof(uint32_t));
                        int32 = ntohl(int32);
                        sign = udp_m->content[0];
                        if (sign)
                            int32 = -int32;

                        strcpy(tcp_m.type, INT);
                        sprintf(tcp_m.content, "%d", int32);
                        break;

                    case SHORT_REAL_VAL:
                        memcpy(&int16, udp_m->content, sizeof(uint16_t));
                        short_real = abs(1.0 * ntohs(int16) / 100);

                        strcpy(tcp_m.type, SHORT_REAL);
                        sprintf(tcp_m.content, "%.2f", short_real);
                        break;

                    case FLOAT_VAL:
                        memcpy(&int32, udp_m->content + SIGN_SIZE,
                            sizeof(uint32_t));
                        memcpy(&int8, udp_m->content + SIGN_SIZE 
                            + sizeof(uint32_t), sizeof(uint8_t));
                        nr_float = 1.0 * ntohl(int32) / pow(10, int8);
                        sign = udp_m->content[0];
                        if (sign)
                            nr_float = -nr_float;

                        strcpy(tcp_m.type, FLOAT);
                        sprintf(tcp_m.content, "%f", nr_float);
                        break;

                    case STRING_VAL:
                        strcpy(tcp_m.type, STRING);
                        memcpy(tcp_m.content, udp_m->content,
                            sizeof(tcp_m.content));
                        break;
                    }

                    // update the clients
                    string topic(tcp_m.topic);
                    for (auto &client : clients) {
                        topics = client.getTopics();
                        
                        if (topics.count(topic)) {
                            if (client.getConnected()) {
                                rc = send(client.getSockfd(), &tcp_m,
                                    sizeof(tcp_m), 0);
                                DIE (rc < 0, "send error");
                            } else if (topics[topic]) {
                                client.getPending().push(tcp_m);
                            }
                        }
                    }
                } else {
                    // incoming commands from TCP clients
                    rc = recv(pfds[i].fd, buf, sizeof(struct cmd), 0);
                    DIE(rc < 0, "recv error");

                    for (auto &client : clients)
                        if (client.getSockfd() == pfds[i].fd) {
                            if (!rc) {
                                //// no data -> disconnect
                                close(pfds[i].fd);
                                client.setConnected(false);
                                cout << "Client " << client.getId()
                                    << " disconnected.\n";

                                for (int j = i; j < nfds - 1; j++)
                                    pfds[j] = pfds[j + 1];
                                nfds--;
                                i--;
                            } else {
                                m = (struct cmd *)buf;
                                topics = client.getTopics();
                                string topic(m->topic);

                                if (strcmp(m->command, SUB) == 0) {
                                    topics[topic] = m->sf;
                                } else {
                                    // unsubscribe
                                    DIE(!topics.count(topic),
                                        "not subscribed error");
                                    topics.erase(topic);
                                }

                                client.setTopics(topics);
                            }

                            break;
                        }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    DIE(argc < 2, "server start error");
    // disable Nagle
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // create socket for listening
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket create error");
    int one = 1;
    // disable buffering
    int rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&one,
        sizeof(one));
    DIE(rc < 0, "socket options error");

    // set socket parameters
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    uint16_t port;
    rc = sscanf(argv[1], "%hu", &port);
    DIE(rc < 0, "port error");
    serv_addr.sin_port = htons(port);

    // binding and listening
    rc = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    DIE(rc < 0, "bind error");
    rc = listen(sockfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen error");

    // create UDP socket
    int udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_sockfd < 0, "socket create error");

    // set socket parameters
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port);

    // bind the UDP connection
    rc = bind(udp_sockfd, (struct sockaddr *) &udp_addr,
        sizeof(struct sockaddr));
    DIE(rc < 0, "bind error");

    run_server(sockfd, udp_sockfd, udp_addr);

    return 0;
}
