#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>

extern char* get_menu(char*);
extern void user(int);
extern void admin(int);


void bank(int socket){
	char* username = malloc(50 * sizeof(char));
	char* password = malloc(50 * sizeof(char));
	char* message = malloc(100 * sizeof(char));
	char* choice = get_menu("Start");
	char* mode = (!strcmp(choice, "SIGN_UP") ? "Sign Up" : "Sign In");
	char* option = get_menu(mode);
    
	printf("Username: ");
	scanf("%s",username);
	printf("Password: ");
	scanf("%s",password);

	if (!strcmp(choice, "SIGN_UP") || !strcmp(choice, "SIGN_IN")) {
		send(socket, option, sizeof(option), 0); 
	    send(socket, username, sizeof(username), 0); 
	    send(socket, password, sizeof(password), 0); 
		read(socket, message, 100 * sizeof(char)); 
		printf("%s\n", message); 
		if(!strcmp(message, "Authentication failed\n") || !strcmp(message,"User with this username already exists\n")) exit(1);
		while(1) {
			char type = option[strlen(option) - 1];
			if (type == 'U' || type == 'J') user(socket);
			else if (type == 'A') admin(socket);  
		}
	} 
}

int main(int argc, char const *argv[]) { 
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(8080); 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	bank(sock);
	return 0; 
} 

