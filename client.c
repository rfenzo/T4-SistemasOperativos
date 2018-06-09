#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>

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

char* readBuffer(char* buffer, int* id, int* payloadSize){
  *id = (int) (buffer[0] -'0');
  *payloadSize = (int)(buffer[1]-'0');
  char* payload = malloc(*payloadSize);
  for (int i = 0; i < *payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  return payload;
}

void formatBet(int betId, char* buffer){
    if (betId == 1) {
      strcpy(buffer, "FOLD");
    }else if (betId == 2) {
      strcpy(buffer, "$0");
    }else if (betId == 3) {
      strcpy(buffer, "$100");
    }else if (betId == 4) {
      strcpy(buffer, "$200");
    }else if (betId == 5) {
      strcpy(buffer, "$500");
    }
}

char numberToCardChar(int numero){
  if (numero == 11) {
    return 'J';
  }else if (numero == 12) {
    return 'Q';
  }else if (numero == 13) {
    return 'K';
  }else{
    return numero + '0';
  }
}

void printCard(Card* card){
  if (card->pinta == 1) {
    printf("%2c %s",numberToCardChar(card->numero), "\u2665" );
  }else if (card->pinta == 2) {
    printf("%2c %s",numberToCardChar(card->numero), "\u2666" );
  }else if (card->pinta == 3) {
    printf("%2c %s",numberToCardChar(card->numero), "\u2663" );
  }else if (card->pinta == 4) {
    printf("%2c %s",numberToCardChar(card->numero), "\u2660" );
  }
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
	int ret, readedBytes, id, pot, i, payloadSize;
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
				char* payload = readBuffer(buffer, &id, &payloadSize);
				if (id == 2) {
					//Connection Established
					printf("  -> Solicitud aceptada!\n");
				}else if (id == 3) {
					//Ask Nickname
				  printf("\nIngresa tu nombre de usuario: ");
					char nickname[256];
				  scanf("%s", nickname);
					buffer[0] = '4';
					buffer[1] = strlen(nickname)+'0';
					buffer[2] = 0; // hace que se concatene desde buffer[2]
					strcat(buffer,nickname);
					sendMessage(socket, buffer);
          printf("  -> Esperando un contrincante...\n");
				}else if (id == 5) {
					//Opponent Found
				strcpy(contrincante,payload);
					printf("Contrincante: %s\n", contrincante);
				}else if (id == 6) {
					//Initial Pot
					pot = atoi(payload);
          printf("\nDinero inicial de %i\n", pot);
				}else if (id == 7) {
					//Game Start
          printf("\n---- El juego ha iniciado! ----\n");
				}else if (id == 8) {
					//Start Round
					pot = atoi(payload);
				}else if (id == 9) {
					//Initial Bet
					pot -= 10;
          printf("\nPagando apuesta inicial\n  Dinero: %i -> %i\n", pot+10,pot);
				}else if (id == 10) {
					//5-Cards
					printf("\nAquí estan tus cartas:\n");
          // setlocale(LC_CTYPE, "");
					for (i = 0; i < 5; i++) {
						hand[i]->numero = payload[2*i]-'0';
						hand[i]->pinta = payload[2*i+1]-'0';
						hand[i]->valid = true;
            printf("  %i: ",i+1);
            printCard(hand[i]);
            printf("\n");
					}
				}else if (id == 11) {
					//Who's First
          if (atoi(payload) == 1) {
            printf("Tu comienzas esta ronda! \n");
          }else if (atoi(payload) == 2) {
            printf("Esta ronda comienza %s, debes esperar su apuesta\n", contrincante);
          }
				}else if (id == 12) {
          //Get Cards to Change
					i = 10;
					char* amount = malloc(1);
					while (i<0 || i>5){
						printf("¿Cuantas Cartas quieres cambiar?\n");
						scanf("%s", amount);
						i = atoi(amount);
						if (i<0 && i>5){
							printf("Debes ingresar un numero entre 0 y 5\n");
						}
					}
          char carta[1];
          buffer[0]= 13 +'0';
					buffer[1]= i*2 +'0';
					for (int c=0; c<i; c++){
						printf("Carta %i/%i: Ingresa Nº de carta a cambiar: ", c+1, i);
						scanf("%s", carta);
            buffer[2+c*2]= hand[atoi(carta)-1]->numero;
            buffer[2+c*2+1]= hand[atoi(carta)-1]->pinta;
            printf("Se selecciońo ");
            printCard(hand[atoi(carta)-1]);
            printf("\n");
					}
          sendMessage(socket, buffer);
				}else if (id == 14) {
          //Get Bet
          char select[1];
          printf("Elige tu apuesta:\n");
          char betname[4];
          for (i = 0; i < payloadSize; i++) {
            formatBet(payload[i]-'0',betname);
            printf("  %i: %s\n",i+1, betname);
          }
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
