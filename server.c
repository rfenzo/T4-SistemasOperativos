#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>

struct card{
  int pinta;
  int numero;
  bool taken;
};
typedef struct card Card;

Card** handMaker(){
  Card** hand = malloc(sizeof(Card*)*5);
  for (int i = 0; i < 5; i++) {
    hand[i] = malloc(sizeof(Card));
  }
  return hand;
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

void sendMessage(int socket, char* message){
  send(socket, message, 256,0);
}

void opponentFound(char** nicknames, struct pollfd* fds){
  for (int i = 0; i < 2; i++) {
    printf("%s\n",nicknames[i]);
    char buffer[254];
    buffer[0] = '5';
    buffer[1] = strlen(nicknames[i])+'0';
    buffer[2] = 0; // hace que se concatene desde buffer[2]
    strcat(buffer,nicknames[i]);
    sendMessage(fds[i].fd, buffer);
  }
}

int main (int argc, char *argv[])
//Elemento 1 de argv será 1 si es server o 0 si es client
{
	if (argc != 3) {
		printf("Número de argumentos inadecuado\n$ ./server -i <ip_address> -p <tcp-port>\n");
		return 1;
	}

	int welcomeSocket;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;

	welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr((char*)argv[1]);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  if(listen(welcomeSocket,2)!=0)
    printf("Error\n");

  struct pollfd fds[3];
	fds[0].fd = welcomeSocket;
  fds[0].events = POLLIN;
  fds[1].fd = -1;
  fds[1].events = POLLIN;
  fds[2].fd = -1;
  fds[2].events = POLLIN;

  char* nicknames[2] = {malloc(sizeof(char*)),malloc(sizeof(char*))};
  Card** hands[2] = {handMaker(),handMaker()};

  int timeout_msecs = 500;
  int ret, i, readedBytes, id;
  // Player* players[2] = { player_init(), player_init() };
  Card* deck[52];
  char buffer[256];
  int playercount = 0;

  while(1){
    ret = poll(fds, 3, timeout_msecs);
    if (ret > 0) {
      for (i = 0; i < 3; i++) {
				if (fds[i].revents) {
          // Player* currentPlayer = getPlayer(fds[i].fd, players);
					if (i == 0) {
						//new connection
            //WARNING se queda en loop infinito si un tercer jugador ingresa
						for (int j = 1; j < 3; j++) {
							if (fds[j].fd < 0) {
								addr_size = sizeof serverStorage;
								fds[j].fd = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
								break;
							}
						}
					}else{
            sleep(1);
						readedBytes = read(fds[i].fd, buffer, 256);
						if (readedBytes == 0) {
							printf("Cliente se ha desconectado\n");
							fds[i].fd = -1;
							// WARNING que pasa acá?.
						}else{
              char* payload = readBuffer(buffer, &id);
              if (id == 1) {
                //Start Connection
                printf("Cliente solicitó conexión\n");
                sendMessage(fds[i].fd, "200");
                //solicitar nickname;
                sendMessage(fds[i].fd, "300");
              }else if (id == 24) {
                //Error Not Implemented
              }else if (fds[i].fd > 0) {
                if (id == 4) {
                  //Return Nickname
                  printf("Recibido el nickname: %s\n", payload);
                  strcpy(nicknames[i], payload);
                  printf("%s, %s\n", nicknames[0], nicknames[1]);
                  playercount++;
                  if (playercount == 2) {
                    opponentFound(nicknames, fds);
                    // generateDeckOfCards(deck);
                    // send5Cards(players, deck);
                  }
                }else if (id == 13) {
                  //Return Cards to Change
                  // changeCards(currentPlayer, payload, deck);
                }else if (id == 15) {
                  //Return Bet
                  //WARNING Falta hacer ésto
                }
              }else{
                //WARNING que hacer?
                printf("Cliente no conectado enviando data: id:%i | payload: %s\n", id,payload);
              }
						}
					}
				}
      }
    }
  }
	return 0;
}
