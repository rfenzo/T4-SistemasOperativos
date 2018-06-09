#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <locale.h>
#include <wchar.h>

#define CORAZON 0x2665

//MARTIN

struct card{
  int pinta;
  int numero;
	bool valid;
};
typedef struct card Card;

wchar_t printPinta(int pinta){
  if (pinta == 1) {
    return (wchar_t) 0x2665;
  }else if (pinta == 2) {
    return (wchar_t) 0x2666;
  }else if (pinta == 3) {
    return (wchar_t) 0x2663;
  }else if (pinta == 4) {
    return (wchar_t) 0x2660;
  }
  return 0;
}

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
	return clientSocket;
}

void sendMessage(int socket, char* message){
  send(socket, message, 258,0);
}

char* readBuffer(char* buffer, int* id){
  *id = (int) (buffer[0] -'0');
  // printf("id: %i\n", *id);
  unsigned int payloadSize = (int)(buffer[1]-'0');
  // printf("payloadsize: %i\n", payloadSize);
  char* payload = malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  // printf("id:%i, size:%i, payload: %s\n",*id,payloadSize,payload );
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
	char buffer[258];
	char contrincante[256];
	Card* hand[5] = {malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card))};


	printf("Solicitando participar en el juego\n");
	sendMessage(socket, "100");

	while(1){
		ret = poll(fds, 1, timeout_msecs);
		if (ret > 0) {
			readedBytes = read(fds[0].fd, buffer, 258);
			if (readedBytes == 0) {
				// WARNING que pasa acá?.
			}else{
				char* payload = readBuffer(buffer, &id);
        printf("ID: %i\n", id);
				if (id == 2) {
					//Connection Established
					printf("Solicitud aceptada!\n");
				}else if (id == 3) {
					//Ask Nickname
				  printf("Ingresa tu nombre de usuario: ");
					char nickname[256];
				  scanf("%s", nickname);
					buffer[0] = '4';
					buffer[1] = strlen(nickname)+'0';
					buffer[2] = 0; // hace que se concatene desde buffer[2]
					strcat(buffer,nickname);
					sendMessage(socket, buffer);
				}else if (id == 5) {
					//Opponent Found
				strcpy(contrincante,payload);
					printf("Contrincante: %s\n", contrincante);
				}else if (id == 6) {
					//Initial Pot
					pot = atoi(payload);
          printf("Dinero inicial de %i\n", pot);
				}else if (id == 7) {
					//Game Start
          printf("El juego ha iniciado!\n");
				}else if (id == 8) {
					//Start Round
					pot = atoi(payload);
				}else if (id == 9) {
					//Initial Bet
					pot -= 10;
          printf("Pagando apuesta inicial\n  -> Dinero: %i -> %i\n", pot+10,pot);
				}else if (id == 10) {
					//5-Cards
					printf("\nAquí estan tus cartas:\n");
          // setlocale(LC_CTYPE, "");
					for (i = 0; i < 5; i++) {
						hand[i]->numero = payload[2*i]-'0';
						hand[i]->pinta = payload[2*i+1]-'0';
						hand[i]->valid = true;
            printf("  -> %i de pinta %i\n", hand[i]->numero,hand[i]->pinta);
					}
				}else if (id == 11) {
					//Who's First
				}else if (id == 12) {
					//Get Cards to Change
          printf("Entro id 12\n");
					i = 10;
					char* amount = malloc(1);
					while (i<0 || i>5){
						printf("Cuantas Cartas quieres cambiar:\n");
						scanf("%s", amount);
						i = atoi(amount);
						if (i<0 && i>5){
							printf("Debes ingresar un numero entre 0 y 5\n");
						}
					}
          printf("i: %i\n",i );
					if (i>0){
            char carta[1];
            buffer[0]= 13 +'0';
						buffer[1]= i*2 +'0';
  						for (int c=0; c<i; c++){
  						printf("Ingresa Nº de carta %i a cambiar:\n", c+1);
  						scanf("%s", carta);
              buffer[2+c*2]= hand[atoi(carta)-1]->numero;
              buffer[2+c*2+1]= hand[atoi(carta)-1]->pinta;
              printf("Carta %i: %i de %i\n",atoi(carta)+1, hand[atoi(carta)-1]->numero,hand[atoi(carta)-1]->pinta);
  						}
              printf("Cartas ingresadas");
            }else if (i == 0){
              buffer[0] = 13 +'0';
              buffer[1] = '0';
            }
            printf("Salio del if\n");
            sendMessage(socket, buffer);
            printf("Size sended payload: %i\n", buffer[1]-'0');
				}else if (id == 14) {
					//Get Bet
          char select[1];
          printf("Te toca apostar! Elige tu apuesta:");
          printf("FOLD: 1\n");
          printf("0: 2\n");
          printf("100: 3\n");
          printf("200: 4\n");
          printf("500: 5\n");
          scanf("%s", select);

          buffer[0] = 15+'0';
          buffer[1] = '1';
          buffer[2] = atoi(select);
          sendMessage(socket, buffer);

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
          printf("El juego ha terminado\n");
          return 0;
				}else if (id == 23) {
					//Image
				}else if (id == 24) {
					//Error Not Implemented
				}
			}
		}
	}
	return 0;
}
