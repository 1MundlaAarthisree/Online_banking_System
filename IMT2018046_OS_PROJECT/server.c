#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <fcntl.h>
extern void* handler(void*);

int main() { 
	int sd, client; 
	struct sockaddr_in server; 
	int opt = 1; 
	int addrlen = sizeof(server); 
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	server.sin_family = AF_INET; 
	server.sin_addr.s_addr = INADDR_ANY; 
	server.sin_port = htons(8080); 
	
	if (bind(sd, (struct sockaddr*) &server, sizeof(server))<0) { 
		perror("Could not bind socket to port."); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(sd, 3) < 0) { 
		perror("Error"); 
		exit(EXIT_FAILURE); 
	} 
    while(1) {
        if ((client = accept(sd, (struct sockaddr*) &server, (socklen_t*) &addrlen)) < 0) { 
            perror("Error"); 
            exit(EXIT_FAILURE); 
        } 
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handler, (void*) &client) < 0) {
            perror("Error during thread creation");
            return 1;
        }
        puts("Listening...");
    }
	return 0; 
} 

