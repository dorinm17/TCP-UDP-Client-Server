#include "helpers.h"

void run_tcp_client(int sockfd)
{
    // polling file descriptors
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
    pfds[1].fd = sockfd;
	pfds[1].events = POLLIN;
    // number of file descriptors
	int nfds = 2;

    // auxiliary variables
    int rc;
    char buf[BUFMAX], msg[INPUT_SIZE];
    char *p;
    vector<char *> words;
    struct tcp_msg *tcp_m;
    struct cmd m;

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
                    memset(msg, 0, sizeof(msg));
                    memset(&m, 0, sizeof(m));

                    // for checking the arguments of the command
                    p = strtok(buf, " \n");
                    while (p) {
                        words.push_back(p);
                        p = strtok(NULL, " \n");
                    }

                    DIE(strcmp(words[0], SUB) != 0 
                        && strcmp(words[0], UNSUB) != 0
                        && strcmp(words[0], EXIT) != 0, "unkown command");

                    if (strcmp(words[0], EXIT) == 0) {
                        DIE(words.size() != 1, "exit does not take paramters");
                        return;
                    } else if (strcmp(words[0], SUB) == 0) {
                        DIE(words.size() != 3, "subscribe parameters error");
                        DIE(strcmp(words[2], "0") != 0 
                            && strcmp(words[2], "1") != 0, "sf error");
                        m.sf = words[2][0] - '0';
                        strcpy(msg, SUBSCRIBED);
                    } else {
                        DIE(words.size() != 2, "unsubscribe error");
                        m.sf = 0;
                        strcpy(msg, UNSUBSCRIBED);
                    }

                    // send command to the server
                    strcpy(m.command, words[0]);
                    strcpy(m.topic, words[1]);
                    rc = send(sockfd, &m, sizeof(m), 0);
                    DIE(rc < 0, "send error");
                    puts(msg);

                    words.clear();
                } else if (pfds[i].fd == sockfd) {
                    // messages from the UDP clients through the server
                    rc = recv(sockfd, buf, sizeof(struct tcp_msg), 0);
                    DIE(rc < 0, "recv error");

                    // no data -> disconnect
                    if (!rc)
                        return;

                    tcp_m = (struct tcp_msg *)buf;
                    printf("%s:%hu - %s - %s - %s\n", tcp_m->ip_udp,
                        tcp_m->port_udp, tcp_m->topic, tcp_m->type,
                        tcp_m->content);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    DIE(argc < 4, "TCP client start error");
    // disable Nagle
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // create socket
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
    rc = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(!rc, "ip error");
    uint16_t port;
    rc = sscanf(argv[3], "%hu", &port);
    DIE(rc < 0, "port error");
    serv_addr.sin_port = htons(port);

    // connect to the server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect error");
    rc = send(sockfd, argv[1], sizeof(argv[1]), 0);
    DIE(rc < 0, "send error");

    run_tcp_client(sockfd);
    close(sockfd);

    return 0;
}
