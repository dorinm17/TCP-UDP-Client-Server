Manea Dorin-Mihai, 323 CA
Homework #2 - TCP and UDP client-server application for message management
May, 2023

    The project is implemented in C++. TCP clients communicate with the server
in order to subscribe or unsubscribe from topics provided by UDP clients. The
goal was to realize the TCP subscribers and the server that acts as an
intermediary between the two.
    Mentions: buffering and the Nagle algorithm are deactivated and I have
checked for mostly any input error


Subscriber:

    I create the socket, bind it and connect to the server and I send the ID of
the client. Afterwards, I poll through the file descriptors of stdin and the
server.
    From stdin, the client can receive three commands:
- exit: closes the communication with the server
- subscribe <topic> <sf>: if `sf` (store-and-forward) is set to 1, then the
subscriber will have to be updated with all the messages it did not receive
while being disconnected
- unsubscribe <topic>
    The received parameters are then sent to the server with the `cmd` struct.
    If data comes from the server in the form of the `tcp_msg` struct,
I display it. Otherwise, the connection was interrupted by the server and I
close the client side, too.


Server:

    I create the sockets for the UDP clients and for listening for the TCP
clients, bind them and start listening. Afterwards, I poll through the file
descriptors of stdin, the UDP clients, the one for listening and the TCP
clients once the connections are made.
    In order to keep track of what topics a TCP client is subscribed to,
whether `sf` is set or not and the pending messages which are to be sent to
them, I created the `Subscriber` class. Topics and the status of `sf` for each
one can be simply stored as an unordered map, while the pending messages can be
stored in a queue (FIFO). All th subscribers are kept in a vector of Subscriber
objects.
    The only command that can come from stdin is `exit`, which immediately
closes the server and the connections with the clients.
    If a subscriber wants to connect, I accept the request and check if the
client was not already subscribed. If so, the communication with that client is
ended. Otherwise, the connection is established and if the new client is
actually a former client, they will receive all pending messages. Polling
parameters are of course updated in any case.
    If a message comes from the UDP clients as a `udp_msg` struct, it is
processed accordingly based on its contents and forwarded to all subscribers.
    Commands coming from a TCP client are treated according to what is stated
above about the subscriber implementation.


Resources:
1) https://pcom.pages.upb.ro/labs/
