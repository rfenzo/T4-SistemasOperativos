#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>

struct card{
  int pinta;
  int numero;
	bool valid;
};
typedef struct card Card;

int initializeClient(char* ip, int port){
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  int clientSocket = socket(PF_INET, SOCK_STREAM, 0);

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(ip);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  addr_size = sizeof serverAddr;
	int x = connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	if (x<0) {
		printf("ERROR: %s\n", strerror(errno));
		return -1;
	}
  /*---- Read the message from the server into the buffer ----*/
  // recv(clientSocket, buffer, 256, 0);
  // /*---- Print the received message ----*/
  // printf("Data received: %s",buffer);
  // strcpy(buffer,"Bye World\n");
  // send(clientSocket,buffer,11,0);
	return clientSocket;
}

void sendMessage(int socket, char* message){
  send(socket, message, 256,0);
}

char* readBuffer(char* buffer, int* id){
  *id = (int) (buffer[0] -'0');
  unsigned int payloadSize = (int)(buffer[1]-'0');
  char* payload = malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  return payload;
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printf("Número de argumentos inadecuado\n$ ./client -i <ip_address> -p <tcp-port>\n");
    return 1;
  }
  int socket = initializeClient((char*)argv[1], atoi(argv[2]));
	if (socket < 0) {
		return 2;
	}

	struct pollfd fds[1];
	fds[0].fd = socket;
	fds[0].events = POLLIN;

	int timeout_msecs = 1000;
	int ret, readedBytes, id, pot, i;
	char buffer[256];
	char contrincante[254];
	Card* hand[5];

	printf("Solicitando participar en el juego\n");
	sendMessage(socket, "100");

	while(1){
		ret = poll(fds, 1, timeout_msecs);
		if (ret > 0) {
			readedBytes = read(fds[0].fd, buffer, 256);
			if (readedBytes == 0) {
				// WARNING que pasa acá?.
			}else{
				// waitingResponse = false;
				char* payload = readBuffer(buffer, &id);
				if (id == 2) {
					//Connection Established
					printf("Solicitud aceptada!\n");
				}else if (id == 3) {
					//Ask Nickname
					  printf("Ingresa tu nombre de usuario: ");
						char nickname[254];
					  scanf("%s", nickname);
						buffer[0] = '4';
						buffer[1] = strlen(nickname)+'0';
						buffer[2] = 0; // hace que se concatene desde buffer[2]
						strcat(buffer,nickname);
						sendMessage(socket, buffer);
				}else if (id == 5) {
					//Opponent Found
					*contrincante = *payload;
					printf("Contrincante: %s\n", contrincante);
				}else if (id == 6) {
					//Initial Pot
					pot = (int) payload;
				}else if (id == 7) {
					//Game Start
				}else if (id == 8) {
					//Start Round
					pot = (int) payload;
				}else if (id == 9) {
					//Initial Bet
					pot -= 10;
				}else if (id == 10) {
					//5-Cards
					printf("getting cards\n");
					for (i = 0; i < 5; i++) {
						hand[i]->numero = (int)payload[2*i];
						hand[i]->pinta = (int)payload[2*i+1];
						hand[i]->valid = true;
						printf("%i: numero %i, pinta %i\n",i, hand[i]->numero, hand[i]->pinta);
					}
				}else if (id == 11) {
					//Who's First
				}else if (id == 12) {
					//Get Cards to Change
				}else if (id == 14) {
					//Get Bet
				}else if (id == 15) {
					//Return Bet
				}else if (id == 16) {
					//Error Bet
				}else if (id == 17) {
					//Ok Bet
				}else if (id == 18) {
					//End Round
				}else if (id == 19) {
					//Show Opponent Cards
				}else if (id == 20) {
					//Winner/Loser
				}else if (id == 21) {
					//Update Pot
				}else if (id == 22) {
					//Game End
				}else if (id == 23) {
					//Image
				}else if (id == 24) {
					//Error Not Implemented
				}
			}
		}
	}

	// char* message = malloc(sizeof(char)*256);
  // while (1) {
  //   printf("\nYour Message: ");
  //   scanf("%s", message);
  //   printf("\n");
  //   sendMessage(socket, message);
  //   // char* msg = recieveMessage(socket, message);
  //   // printf(msg, "%s\n");
	// 	// sleep(2);
  // }
	return 0;
}
