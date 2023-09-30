#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <poll.h>
#include <unordered_map>
#include <string>
#include <queue>
#include <math.h>

using namespace std;

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define BUFMAX 1552
#define TOPIC_SIZE 50
#define CONTENT_SIZE 1501
#define MAX_CONNECTIONS 10
#define SIGN_SIZE 1
#define ID_SIZE 10
#define CMD_SIZE 12
#define IP_SIZE 16
#define TYPE_SIZE 11
#define INPUT_SIZE 25

#define EXIT "exit"
#define SUB "subscribe"
#define UNSUB "unsubscribe"
#define SUBSCRIBED "Subscribed to topic."
#define UNSUBSCRIBED "Unsubscribed from topic."
#define INT "INT"
#define SHORT_REAL "SHORT_REAL"
#define FLOAT "FLOAT"
#define STRING "STRING"

#define INT_VAL 0
#define SHORT_REAL_VAL 1
#define FLOAT_VAL 2
#define STRING_VAL 3

struct __attribute__ ((__packed__)) cmd {
    char command[CMD_SIZE];
    char topic[TOPIC_SIZE + 1];
   	uint8_t sf;
};

struct __attribute__ ((__packed__)) tcp_msg {
    char ip_udp[IP_SIZE];
    uint16_t port_udp;
    char topic[TOPIC_SIZE + 1];
    char type[TYPE_SIZE];
    char content[CONTENT_SIZE];
};

struct __attribute__ ((__packed__)) udp_msg {
    char topic[TOPIC_SIZE];
    uint8_t type;
    char content[CONTENT_SIZE];
};

class Subscriber {
	private:
		string id;
		int sockfd;
		bool connected;
		unordered_map<string, uint8_t> topics;
    	queue<struct tcp_msg> pending;

	public:
		Subscriber(string id, int sockfd) {
			this->id = id;
			this->sockfd = sockfd;
			connected = true;
		}

		string getId() {
			return id;
		}

		int getSockfd() {
			return sockfd;
		}

		void setSockfd(const int sockfd) {
        	this->sockfd = sockfd;
    	}

		bool getConnected() {
        	return connected;
    	}

		void setConnected(const bool connected) {
			this->connected = connected;
		}

		unordered_map<string, uint8_t> getTopics() {
			return topics;
		}

		void setTopics(unordered_map<string, uint8_t> topics) {
			this->topics = topics;
		}

		queue<struct tcp_msg> getPending() {
			return pending;
		}

		void setPending(queue<struct tcp_msg> pending) {
			this->pending = pending;
		}
};

#endif
