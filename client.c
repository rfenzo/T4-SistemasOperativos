#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
/* Función que inicializa el cliente en el port
con ip */

int initializeClient(char* ip, int port){
	int clientSocket;
  //char buffer[1024];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  /*---- Creación del Socket. Se pasan 3 argumentos ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP en este caso) */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);

  /*---- Configuración de la estructura del servidor ----*/
	/* Address family = Internet */
  serverAddr.sin_family = AF_INET;
	/* Set port number */
  serverAddr.sin_port = htons(port);
  /* Setear IP address como localhost */
  serverAddr.sin_addr.s_addr = inet_addr(ip);
  /* Setear todos los bits del padding en 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  /*---- Conectar el socket al server ----*/
  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	printf("Connected to server!\n");
  /*---- Read the message from the server into the buffer ----*/
  // recv(clientSocket, buffer, 1024, 0);
  // /*---- Print the received message ----*/
  // printf("Data received: %s",buffer);
  // strcpy(buffer,"Bye World\n");
  // send(clientSocket,buffer,11,0);
	return clientSocket;
}

char* recieveMessage(int socket, char* message){
  printf("Waiting message... ♔ \n");
  recv(socket, message, 1024, 0);
  return message;
}

void sendMessage(int socket, char* message){
  send(socket, message, 1024,0);
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printf("Número de argumentos inadecuado\n$ ./client -i <ip_address> -p <tcp-port>\n");
    return 1;
  }
  printf("Client\n");
  int socket;
  socket = initializeClient((char*)argv[1], atoi(argv[2]));

  while (1) {
    char* message = malloc(sizeof(char)*1024);
    printf("\nYour Message: ");
    scanf("%s", message);
    printf("\n");
    sendMessage(socket, message);
    char* msg = recieveMessage(socket, message);
    printf(msg, "%s\n");
  }
	return 0;
}
