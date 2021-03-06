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
#include <math.h>

void showbits(char x){
    int i;
    for(i=(sizeof(x)*8)-1; i>=0; i--)
            (x&(1u<<i))?putchar('1'):putchar('0');

    printf("\n");
}

int charsToInt(char* input) {
  //formato de envio: 2 chars en BigEndian. Ej para 1000 sería:
  // 00000011
  // 11101000
  int result = 0;
  for (int j = 0; j < 2; j++) {
    for(int i = 7; i >= 0; i--) {
      if (input[j]&(1u << i)) {
        result += pow(2,i+((1-j)*8));
      }
    }
  }

  return result;
}

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
	return clientSocket;
}

void sendMessage(int socket, char* message){
  send(socket, message, 258,0);
}

char* readBuffer(char* buffer, int* id, int* payloadSize){
  // printf("Recibo bits "); showbits(buffer[0]); printf(" Y los interpreto como %i\n", (int) (buffer[0]));
  *id = (int) (buffer[0]);
  *payloadSize = (int)(buffer[1]);
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
  }else if (numero == 1) {
    return 'A';
  }else if (numero == 10) {
    return 'D';
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
  if (argc != 5) {
		printf("Argumentos inadecuado\n$ ./server -i <ip_address> -p <tcp-port>\n");
		return 1;
	}else if ((strcmp(argv[1],"-i") != 0 && strcmp(argv[1],"-p") != 0) ||
            (strcmp(argv[3],"-i") != 0 && strcmp(argv[3],"-p") != 0) ||
            (strcmp(argv[1],argv[3]) == 0)) {
      printf("Argumentos inadecuado\n$ ./server -i <ip_address> -p <tcp-port>\n");
      return 2;
  }
  char* IP;
  int PORT;
  if (strcmp(argv[1],"-i") == 0) {
    IP = (char*)argv[2];
    PORT = atoi(argv[4]);
  }else{
    IP = (char*)argv[4];
    PORT = atoi(argv[2]);
  }

  int socket = initializeClient(IP, PORT);
	if (socket < 0) {
		return 2;
	}

	struct pollfd fds[1];
	fds[0].fd = socket;
	fds[0].events = POLLIN;

	int timeout_msecs = 1000;
	int ret, readedBytes, id, pot, i, payloadSize;
	char buffer[258];
	char* contrincante = malloc(256);
	Card* hand[5] = {malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card))};
  Card* oponent_hand[5] = {malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card)),malloc(sizeof(Card)),
                  malloc(sizeof(Card))};


	printf("Solicitando participar en el juego\n");
  buffer[0] = 1;
  buffer[1] = 0;
	sendMessage(socket, buffer);

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
					buffer[0] = 4;
					buffer[1] = strlen(nickname);
					buffer[2] = 0;
					strcat(buffer,nickname);
					sendMessage(socket, buffer);
          printf("  -> Esperando un contrincante...\n");
				}else if (id == 5) {
					//Opponent Found
          strcpy(contrincante,payload);
					printf("  -> Tu contrincante es %s\n", contrincante);
				}else if (id == 6) {
					//Initial Pot
					pot = charsToInt(payload);
          printf("\nDinero inicial de %i\n", pot);
				}else if (id == 7) {
					//Game Start
          printf("\n---- El juego ha iniciado! ----\n");
				}else if (id == 8) {
					//Start Round
					pot = charsToInt(payload);
          printf("Ha comenzado la ronda! Tu pot actual es de %i\n", pot);
				}else if (id == 9) {
					//Initial Bet
					pot -= payload[0];
          printf("\nPagando apuesta inicial\n  Dinero: %i -> %i\n", pot+10,pot);
				}else if (id == 10) {
					//5-Cards
					printf("\nAquí estan tus cartas:\n");
					for (i = 0; i < 5; i++) {
						hand[i]->numero = payload[2*i];
						hand[i]->pinta = payload[2*i+1];
						hand[i]->valid = true;
            printf("  %i: ",i+1);
            printCard(hand[i]);
            printf("\n");
					}
				}else if (id == 11) {
					//Who's First
          int numero = payload[0];
          if (numero== 1) {
            printf("\nTu comienzas en esta nueva ronda! \n");
          }else if (numero == 2) {
            printf("\nEsta ronda comienza %s, debes esperar su apuesta.\n", contrincante);
          }
				}else if (id == 12) {
          //Get Cards to Change
					i = 10;
					char* amount = malloc(1);
					while (i<0 || i>5){
						printf("\n¿Cuantas Cartas quieres cambiar? : ");
						scanf("%s", amount);
						i = atoi(amount);
						if (i<0 && i>5){
							printf("Debes ingresar un numero entre 0 y 5\n");
						}
					}
          char carta[1];
          buffer[0]= 13;
					buffer[1]= i*2;
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
            formatBet(payload[i], betname);
            printf("  Opcion %i: %s\n",payload[i], betname);
          }
          printf("Opcion: ");
          scanf("%s", select);
          buffer[0] = 15;
          buffer[1] = 1;
          buffer[2] = atoi(select);
          sendMessage(socket, buffer);
          printf("Opcion %i enviada", buffer[2]);
				}else if (id == 16) {
					//Error Bet
          printf(" pero rechazada por el servidor, vuelve a ingresar una apuesta... \n");
          char select[1];
          printf("Opcion: ");
          scanf("%s", select);
          buffer[0] = 15;
          buffer[1] = 1;
          buffer[2] = atoi(select);
          sendMessage(socket, buffer);
          printf("Opcion %i enviada", buffer[2]);
				}else if (id == 17) {
					//Ok Bet
          printf(" y aceptada por el servidor, esperando al otro jugador\n");
				}else if (id == 18) {
					//End Round
          printf("\nLa ronda ha terminado!\n");
				}else if (id == 19) {
					//Show Opponent Cards
          printf("\nAquí estan las cartas de tu OPONENTE:\n");
					for (i = 0; i < 5; i++) {
						oponent_hand[i]->numero = payload[2*i];
						oponent_hand[i]->pinta = payload[2*i+1];
						oponent_hand[i]->valid = true;
            printf("  %i: ",i+1);
            printCard(oponent_hand[i]);
            printf("\n");
					}
				}else if (id == 20) {
					//Winner/Loser
          if (payload[0] == 1){printf("\n---- GANASTE ESTA RONDA! ----\n");}
          else if (payload[0] == 2){printf("\n---- PERDISTE ESTA RONDA! ----\n");}
				}else if (id == 21) {
					//Update Pot
          pot = charsToInt(payload);
          printf("Tu pot actual es de %i\n", pot);
				}else if (id == 22) {
					//Game End
          printf("El juego ha terminado\n");
          return 0;
				}else if (id == 23) {
					//Image
          printf("Recibiendo imagen!\n");
          FILE* imagen = fopen("imagenRecibida.gif","wb");
          fwrite(payload, payloadSize, 1, imagen);
          fclose(imagen);
				}else if (id == 24) {
					//Error Not Implemented
				}else if (id > 24 || id<1) {
					//Error Not Implemented
          buffer[0]=24;
          buffer[1]=0;
          sendMessage(socket, buffer);
        }
			}
		}
	}
	return 0;
}
