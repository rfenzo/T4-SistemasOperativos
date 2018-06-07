#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>


int initializeClient(char* ip, int port){
	int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(ip);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

	// printf("Connected to server!\n");
  /*---- Read the buffer from the server into the buffer ----*/
  // recv(clientSocket, buffer, 1024, 0);
  // /*---- Print the received buffer ----*/
  // printf("Data received: %s",buffer);
  // strcpy(buffer,"Bye World\n");
  // send(clientSocket,buffer,11,0);
	return clientSocket;
}

char* recieveMessage(int socket, char* buffer){
  // printf("Waiting buffer... ♔ \n");
  recv(socket, buffer, 256, 0);
  return buffer;
}

void sendMessage(int socket, char* buffer){
  send(socket, buffer, 256,0);
}

char* readBuffer(char* buffer,unsigned int* id){
  *id = (int) (buffer[0] -'0');
  unsigned int payloadSize = (int)(buffer[1]-'0');
  char* payload = malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  return payload;
}

int main(int argc, char const *argv[]) {
	//WARNING falta hacer que los argumentos puedan ser pasados en desorden
	//WARNING falta hacer que no se caiga al recibir malos paquetes

  if (argc != 3) {
    printf("Argumentos inadecuados\n$ ./client -i <ip_address> -p <tcp-port>\n");
    return 1;
  }

  int socket = initializeClient((char*)argv[1], atoi(argv[2]));

	// char* buffer = malloc(sizeof(char)*256);
	char buffer[256];
	unsigned int id;
	sendMessage(socket, "100");
	recieveMessage(socket, buffer);
	char* payload = readBuffer(buffer, &id);
	if (id == 2) {
		printf("Solicitud de conexion aceptada!\n");
	}
	// printf(msg, "%s\n");
	//
  // while (1) {
  //   printf("\nYour Message: ");
  //   scanf("%s", buffer);
  //   printf("\n");
  //   sendMessage(socket, buffer);
  //   char* msg = recieveMessage(socket, buffer);
  //   printf(msg, "%s\n");
  // }
	return 0;
}
