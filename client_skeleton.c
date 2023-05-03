#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

static int sockfd;
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condQueue = PTHREAD_COND_INITIALIZER;

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    printf("Or input messages sending to everyone in the chatroom.\n");
}

void* recv_server_msg_handler() {
    /********************************/
	/* receive message from the server and desplay on the screen*/
	/**********************************/
	char buffer[MAX];
	int nbytes;
	while(1){
		bzero(buffer, sizeof(buffer));
		
		if ((nbytes = recv(sockfd, buffer, sizeof(buffer), 0))==-1){
			perror("recv");
		}
		printf("%s", buffer);
	}
	return NULL;

}

int main(){
    int n;
	int nbytes;
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	char name[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	} else {
		printf("Socket successfully created...\n");
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(PORT);

	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
		printf("Connection with the server failed...\n");
		exit(0);
	}
	else
		printf("Connected to the server...\n");

	generate_menu();
	// recieve welcome message to enter the nickname
    bzero(buffer, sizeof(buffer));
    if ((nbytes = recv(sockfd, buffer, sizeof(buffer), 0))==-1){
        perror("recv");
    }
    printf("%s", buffer);

	/*************************************/
	/* Input the nickname and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register/login message*/
	/*******************************************/

	bzero(buffer, sizeof(buffer));
	strcpy(buffer, "REGISTER ");
	n = 9;
	char c;
	int validName = 0;
	while(!validName){
		while (c != '\n') {
			c = getchar();
			if (n == MAX - 1)
				break;
			buffer[n++] = c;
			name[n - 10] = c;
		}
		if (n - 10 >= C_NAME_LEN - 1) {
			printf("Your name is too long, please try again\n");
			bzero(buffer, sizeof(buffer));
			bzero(name, sizeof(name));
			strcpy(buffer, "REGISTER ");
			n = 9;
			c='\0';
		} else {
			validName = 1;
		}
	}
	buffer[n] = '\0';
	send(sockfd, buffer, sizeof(buffer), 0);

    // receive welcome message "welcome xx to joint the chatroom. A new account has been created." (registration case) or "welcome back! The message box contains:..." (login case)
    bzero(buffer, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0)==-1){
        perror("recv");
    }
    printf("%s", buffer);

    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/
	pthread_t recv_server_msg_thread;
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);

    if (pthread_create(&recv_server_msg_thread, NULL, &recv_server_msg_handler, NULL) != 0) {
        perror("Failed to create the thread");
    }
    
	// chat with the server
	for (;;) {
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n')
			;
		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			printf("Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			printf("Exit message has been sent to the server\nIt's OK to close the window now OR enter CTRL+C\n ");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_EXIT failed");
				exit(1);
			}
			pthread_mutex_destroy(&mutexQueue);
    		pthread_cond_destroy(&condQueue);

		}
		else if (strncmp(buffer, "WHO", 3) == 0) {
			printf("Getting user list, pls hold on...\n");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				exit(1);
			}
			printf("If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message "Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending direct message failed...");
				exit(1);
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending broadcast message failed...");
				exit(1);
			}
	
		}
	}

	getch();
    endwin();

	return 0;
}

