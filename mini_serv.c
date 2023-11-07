#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>


void	serverSetup(int portNumber);
void	selectSetup();
void	newClientConnection();
void	newClientMessage();
void	fatalError(char *error);
void	sendMessage(int senderSocket);

int serverSocket, newClientSocket, maxfd, id = 0;

int		clientsIDs[65000];
char	buffer[200100];

struct		sockaddr_in serverAddress, newClientAddress;
socklen_t	len;

fd_set	aset, wset, rset;

int main(int argc, char **argv) {

	if (argc != 2) {
		fatalError("Wrong number of arguments\n");
	}
	
	serverSetup(atoi(argv[1]));
	selectSetup();

	while (1) {
		rset = wset = aset;

		if (select(maxfd + 1, &rset, &wset, 0, NULL) < 0)
			continue;
		if (FD_ISSET(serverSocket, &rset))
			newClientConnection();
		else
			newClientMessage();
	}
}

void	fatalError(char *error) {
	if (serverSocket > 2)
		close(serverSocket);
	write(2, error, strlen(error));
	exit(1);
}

void	serverSetup(int portNumber) {
	// socket create and verification 
	serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
	if (serverSocket == -1)
		fatalError("Fatal error\n");
	bzero(&serverAddress, sizeof(serverAddress)); 

	// assign IP, PORT 
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	serverAddress.sin_port = htons(portNumber); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(serverSocket, (const struct sockaddr *)&serverAddress, sizeof(serverAddress))) != 0)
		fatalError("Fatal error\n");

	if (listen(serverSocket, 10) != 0)
		fatalError("Fatal error\n");
}

void	selectSetup() {
	len = sizeof(newClientAddress);
	FD_ZERO(&aset);
	FD_SET(serverSocket, &aset);
	maxfd = serverSocket;
}

void	newClientConnection() {
	newClientSocket = accept(serverSocket, (struct sockaddr *)&newClientAddress, &len);
	if (newClientSocket < 0)
		fatalError("Fatal error\n");
	sprintf(buffer, "server: client %d just arrived\n", id);
	// printf("%s\n", buffer); debug
	sendMessage(newClientSocket);
	clientsIDs[newClientSocket] = id++;
	FD_SET(newClientSocket, &aset);
	maxfd = newClientSocket > maxfd ? newClientSocket : maxfd;
}

void	newClientMessage() {
	for (int fd = 3; fd <= maxfd; ++fd) {
		if (FD_ISSET(fd, &rset)) {
			int r = 1;
			char msg[200000];
			bzero(&msg, sizeof(msg));
			while (r == 1 && msg[strlen(msg) - 1] != '\n')
				r = recv(fd, msg + strlen(msg), 1, 0);
			if (r <= 0) {
				sprintf(buffer, "server: client %d just left\n", clientsIDs[fd]);
				// printf("%s\n", buffer); debug
				FD_CLR(fd, &aset);
				close(fd);
			} else {
				sprintf(buffer, "client %d: %s", clientsIDs[fd], msg);
				// printf("%s\n", buffer); debug
			}
			sendMessage(fd);
		}
	}
}

void	sendMessage(int clientSocket) {
	for (int fd = 3; fd <= maxfd; ++fd) {
		if (fd != clientSocket && FD_ISSET(fd, &wset))
			if (send(fd, buffer, strlen(buffer), 0) < 0)
				fatalError("Fatal error\n");
	}
	bzero(&buffer, sizeof(buffer));
}