#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

void showbits(char x){
  int i;
  for(i=(sizeof(x)*8)-1; i>=0; i--)
    (x&(1u<<i))?putchar('1'):putchar('0');
  printf("\n");
}

char* intToChars(int numero) {
  //formato de envio: 2 chars en BigEndian. Ej para 1000 sería:
  // 00000011
  // 11101000

  char* result = calloc(2,1);
  for (int j = 0; j < 2; j++) {
    if (numero != 0) {
      for (int i = 0; i < 8; i++) {
        if (numero % 2 == 0) {
          result[2-1-j] = result[2-1-j] | 0 << i;
        }else{
          result[2-1-j] = result[2-1-j] | 1 << i;
        }
        numero /= 2;
      }
    }else{
      break;
    }
  }
  return result;
}

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


char* readBuffer(char* buffer, int* id, int* payloadSize){
  *id = (int) (buffer[0]);
  *payloadSize = (int)(buffer[1]);
  char* payload = malloc(*payloadSize);
  for (int i = 0; i < *payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  return payload;
}

void sendMessage(int socket, char* message){
  send(socket, message, 258,0);
}

void opponentFound(char** nicknames, struct pollfd* fds){
  for (int i = 0; i < 2; i++){
    char buffer[256];
    buffer[0] = 5;
    buffer[1] = strlen(nicknames[i]);
    buffer[2] = 0; // hace que se concatene desde buffer[2]
    strcat(buffer,nicknames[i]);
    sendMessage(fds[1-i].fd, buffer);
    // printf("Mostrando id 5:" );
    // showbits(buffer[0]);
  }
}

void swap(Card* a, Card* b){
   int oldAnumero = a->numero;
   int oldApinta = a->pinta;
   a->pinta = b->pinta;
   a->numero = b->numero;
   b->pinta = oldApinta;
   b->numero = oldAnumero;
}

void generateDeckOfCards(Card** deck){
  int count = 0;
  for (int i = 1; i < 14; i++) {
    for (int j = 1; j < 5; j++) {
      deck[count] = malloc(sizeof(Card));
      deck[count]->numero = i;
      deck[count]->pinta = j;
      deck[count]->taken = false;
      count++;
    }
  }
  //shuffle
  srand(time(NULL));
  for (int i = 52-1; i > 0; i--) {
    int j = rand() % (i+1);
    swap(deck[i], deck[j]);
  }
}

Card* getRandomCard(Card** deck){
  for (int i = 0; i < 52; i++) {
    if (!deck[i]->taken) {
      deck[i]->taken = true;
      return deck[i];
    }
  }
  return NULL;
}

void sendHand(struct pollfd fd, Card** hand){
  char buff[258];
  buff[0] = 10;
  buff[1] = 10;
  Card* card;
  for (int i = 0; i < 5; i++) {
    card = hand[i];
    buff[2*i+2] = card->numero;
    buff[2*i+3] = card->pinta;
  }
  sendMessage(fd.fd, buff);
}

void giveInitialCards(struct pollfd fd, Card** deck, Card** hand){
  Card* card;
  for (int i = 0; i < 5; i++) {
    card = getRandomCard(deck);
    if (card) {
      hand[i] = card;
    }else{
      //WARNING puede pasar esto?
      printf("ERROR: No quedan cartas en el mazo\n");
      break;
    }
  }
}

bool compareCards(Card* card1, Card* card2){
  return (card1->numero == card2->numero && card1->pinta == card2->pinta);
}

int identifyWinner(hand1, hand2){
  return 0;
}

void changeCards(Card** hand, char* payload, int payloadSize, Card** deck){
  Card** oldCards = malloc(sizeof(Card)*(payloadSize/2));
  for (int i = 0; i < payloadSize; i++) {
    if (i % 2 == 0) {
      oldCards[i/2] = malloc(sizeof(Card));
      oldCards[i/2]->numero = (int)payload[i];
    }else{
      oldCards[i/2]->pinta = (int)payload[i];
    }
  }

  //cambiar las cartas del player con unas nuevas
  for (int j = 0; j < payloadSize/2; j++) {
    for (int i = 0; i < 5; i++) {
      if (compareCards(hand[i], oldCards[j])) {
        Card* newCard = getRandomCard(deck);
        if (newCard) {
          hand[i] = newCard;
        }else{
          //WARNING que hacer?
          printf("No quedan cartas en el mazo\n");
        }
        break;
      }
    }
  }
}

void sendInteger(struct pollfd fd, int value, int id){
  char buff[4];
  char* potChar;
  buff[0] = id;
  buff[1] = 2;
  potChar = intToChars(value);
  buff[2] = potChar[0];
  buff[3] = potChar[1];
  sendMessage(fd.fd, buff);
}

void winnerLoser(struct pollfd fdsWinner,struct pollfd fdsLoser){
  char buff[3];
  buff[0] = 20 ;
  buff[1] = 1;
  buff[2] = 1;
  sendMessage(fdsWinner.fd, buff);
  buff[2] = 2;
  sendMessage(fdsLoser.fd, buff);
}

void finish(struct pollfd* fds, int winner, int* bets, int* pots, Card*** hands){
  char buff[200];

  pots[winner] += bets[0];
  pots[winner] += bets[1];
  //Envio ronda Termino
  for (int p=0;p<2;p++){
    buff[0] = 18;
    buff[1] = 0;
    sendMessage(fds[p].fd, buff);
  }



  //Envio Cartas Oponente
  for (int p=0;p<2;p++){
    buff[0] = 19;
    buff[1] = 10;
    Card* card;
    for (int i = 0; i < 5; i++) {
      card = hands[p][i];
      buff[2*i+2] = card->numero;
      buff[2*i+3] = card->pinta;
    }
    sendMessage(fds[1-p].fd, buff);
  }

//Eviar si gano o perdio
for (int p=0;p<2;p++){
  buff[0]=20;
  buff[1]=1;
  if (p == winner){
    buff[2]=1;
  }else{buff[2]=2;}
  sendMessage(fds[p].fd, buff);
  }

}

int initialBet(struct pollfd fd, int pot, int bet){
  if (pot >= 10) {
    pot -= 10;
    bet += 10;
    char buff[200];
    buff[0] = 9;
    buff[1] = 1;
    buff[2] = 10;
    sendMessage(fd.fd, buff);
    return 0;
  }else{
    return -1;
  }
}

int changeBet(int* bets, int* pots, int bet_id, int player){
  if (bet_id == 2){ return 0;}
  else if (bet_id == 3 ){
    pots[player] -= (100 - bets[player]);
    bets[player] = 100;
  }else if (bet_id == 4){
    pots[player] -= (200 - bets[player]);
    bets[player] = 200;
  }else if (bet_id == 5){
    pots[player] -= (500 - bets[player]);
    bets[player] = 500;

  }
  return 1;
}



void betOptions(int* buffer, int* money_available, int* pots, int player, int move){
  for (int e=0; e<5; e++){
    buffer[e] = 0;
  }
  int diferencia_apuestas = pots[1-player] - pots[1-player];
  //int diferencia_pots = pots[1-player] - pots[1-player];
  int limite = money_available[1-player] + diferencia_apuestas;

  printf("Limite: %i  Mi Pozo: %i  move: %i\n", limite, pots[player], move);
  if (move > 1){buffer[0]=1;}
  if (pots[1-player]==pots[player]){buffer[1]=1;}
  if (limite>=100 && money_available[player]>=100){buffer[2]=1;}
  if (limite>=200 && money_available[player]>=200){buffer[3]=1;}
  if (limite>=500 && money_available[player]>=500){buffer[4]=1;}
  printf("Imprimiento opciones de apuesta:\n");
  for (int e=0; e<5; e++){
    printf("%i", buffer[e]);
  }
  printf("\n");


}

void printMoneyAvailable(int* ma){
  printf(" Dinero 1: %i\n", ma[0]);
  printf("Dinero 2: %i\n", ma[1]);
}

int main (int argc, char *argv[]){
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
	fds[2].fd = welcomeSocket;
  fds[2].events = POLLIN;
  fds[0].fd = -1;
  fds[0].events = POLLIN;
  fds[1].fd = -1;
  fds[1].events = POLLIN;

  char* nicknames[2] = {malloc(sizeof(char*)),malloc(sizeof(char*))};
  Card** hands[2] = { handMaker(), handMaker() };
  int pots[2] = {1000,1000};
  int bets[2] = {0,0};
  Card* deck[52];

  int timeout_msecs = 500;
  int ret, i, readedBytes, id, payloadSize;
  char buffer[258];
  int playercount = 0;

  bool startRound = false;
  bool betsTime = false;
  // bool whosfirst = false;
  bool waitPlayer0 = false;
  bool waitPlayer1 = false;
  int starter = 0; //permite alternar quien comienza las rondas
  int turn;
  int move;
  int perdedor;

  while(true){
    if (startRound) {
      for (i = 0; i < 2; i++) {
        //start round, envia pot actuales
        sendInteger(fds[i], pots[i], 8);

        //initial bet, envia monto de apuesta inicial y descuenta de su pot
        perdedor = initialBet(fds[i], pots[i], bets[i]);
        //WARNING SI PERDEDOR == -1, SIGNIFICA QUE EL JUGADOR i PERDIÓ

        //setea en el servidor las cartas de los jugadores
        giveInitialCards(fds[i], deck, hands[i]);

        //5-Cards, envia las cartas de los jugadores
        sendHand(fds[i], hands[i]);

        //get cards to change, pregunta a los jugadores si quieres cambiar cartas
        buffer[0] = 12 ;
        buffer[1] = 0;
        buffer[2] = 0;
        sendMessage(fds[i].fd, buffer);
      }
      startRound = false;
      betsTime = true;
      waitPlayer0 = true;
      waitPlayer1 = true;
      turn = starter;
      move = 1;
    }else if (betsTime && !waitPlayer0 && !waitPlayer1) {
      if (turn == 0 && move == 1) {
        //Envío Apuestas Disponibles - Estoy enviando todas
        //betOptions(&possible_bets, &money_available, &pots, i, move);
        printf("Enviando a primero\n");
        buffer[0] = 14;
        buffer[1] = 4 ;
        buffer[2] = 2 ;
        buffer[3] = 3 ;
        buffer[4] = 4 ;
        buffer[5] = 5 ;

        sendMessage(fds[0].fd, buffer);
        turn = 1 - turn;
        waitPlayer0 = true;
      }else if (turn == 1){
        //WARNING falta hacer el turno del segundo player
        turn = 1 - turn;
        waitPlayer1 = true;
      }
    }

    ret = poll(fds, 3, timeout_msecs);
    if (ret > 0) {
      for (i = 0; i < 3; i++) {
				if (fds[i].revents) {
					if (i == 2) {
						//new connection
            //WARNING se queda en loop infinito si un tercer jugador ingresa
						for (int j = 0; j < 2; j++) {
							if (fds[j].fd < 0) {
								addr_size = sizeof serverStorage;
								fds[j].fd = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
								break;
							}
						}
					}else{
						readedBytes = read(fds[i].fd, buffer, 258);
						if (readedBytes == 0) {
							printf("Cliente se ha desconectado\n");
							fds[i].fd = -1;
							// WARNING que pasa acá?.
						}else{

              char* payload = readBuffer(buffer, &id, &payloadSize);
              // char aux;
              // aux = id;
              // showbits(aux);
              if (id == 1) {
                //Start Connection
                printf("Cliente solicitó conexión\n");
                buffer[0] = 2;
                buffer[1] = 0;
                sendMessage(fds[i].fd, buffer);
                //solicitar nickname;
                buffer[0] = 3;
                buffer[1] = 0;
                sendMessage(fds[i].fd, buffer);
              }else if (id == 24) {
                //Error Not Implemented
              }else if (fds[i].fd > 0) {
                if (id == 4) {
                  //Return Nickname
                  printf("Recibido el nickname: %s\n", payload);
                  strcpy(nicknames[i], payload);
                  playercount++;
                  if (playercount == 2) {
                    opponentFound(nicknames, fds);
                    generateDeckOfCards(deck);
                    //game start
                    buffer[0]=7;
                    buffer[1]=0;
                    sendMessage(fds[0].fd, buffer);
                    sendMessage(fds[1].fd, buffer);
                    sendInteger(fds[0], pots[0], 6);
                    sendInteger(fds[1], pots[1], 6);
                    //initial pots
                    startRound = true;
                  }
                }else if (id == 13) {
                  //Return Cards to Change
                  changeCards(hands[i], payload, payloadSize, deck);
                  sendHand(fds[i], hands[i]);
                  if (i == 0) {
                    waitPlayer0 = false;
                  }else if (i == 1) {
                    waitPlayer1 = false;
                  }
                }else if (id == 15) {
                  //Return Bet, entra aca solo cuando move = 1 por lo que move aca siempre >=2
                  move += 1;  //if move = 3 se acaba o si hay FOLD
                  int bet_id=0;
                  if (payload[0]<2){bet_id=1;}
                  else if(payload[0]>1 && payload[0]<3){bet_id=2;}
                  else if(payload[0]>2 && payload[0]<4){bet_id=3;}
                  else if(payload[0]>3 && payload[0]<5){bet_id=4;}
                  else if(payload[0]>4){bet_id=5;}
                  printf("Recibi apuesta %i\n", bet_id);
                  // showbits(payload[0]);
                  printf("Ahora le toca a :%i\n", 1-i);


                  int valid = changeBet(bets, pots, bet_id, i);
                  printMoneyAvailable(bets);

                  if (move == 2){
                    printf("bet del anterior: %i\n", bets[i]);
                    if (bet_id == 1){finish(fds, 1-i, bets, pots, hands); startRound=true;}
                    if (bets[i]==10){
                      buffer[0] = 14;
                      buffer[1] = 5 ;
                      buffer[2] = 1 ;
                      buffer[3] = 2 ;
                      buffer[4] = 3 ;
                      buffer[5] = 4 ;
                      buffer[6] = 5 ;
                      sendMessage(fds[1-i].fd, buffer);
                    }
                    else if(bets[i]==100){
                      buffer[0] = 14;
                      buffer[1] = 4 ;
                      buffer[2] = 1 ;
                      buffer[3] = 3 ;
                      buffer[4] = 4 ;
                      buffer[5] = 5 ;
                      sendMessage(fds[1-i].fd, buffer);
                    }
                    else if(bets[i]==200){
                      buffer[0] = 14;
                      buffer[1] = 3 ;
                      buffer[2] = 1 ;
                      buffer[3] = 4 ;
                      buffer[4] = 5 ;
                      sendMessage(fds[1-i].fd, buffer);
                    }
                    else if(bets[i]==500){
                      buffer[0] = 14;
                      buffer[1] = 2 ;
                      buffer[2] = 1 ;
                      buffer[3] = 5 ;
                      sendMessage(fds[1-i].fd, buffer);
                    }
                  }

                  if (i == 0) {
                    waitPlayer0 = false;

                  }else if (i == 1) {
                    waitPlayer1 = false;
                  }
                  if (move == 3){
                    if (bet_id == 1){finish(fds, 1-i, bets, pots, hands); startRound=true;}
                    if (bets[i]==bets[1-i]){printf("Termino");}
                    else{
                      if (bets[i]==100){
                        buffer[0] = 14;
                        buffer[1] = 2 ;
                        buffer[2] = 1 ;
                        buffer[3] = 3 ;
                        sendMessage(fds[1-i].fd, buffer);
                      }else if (bets[i]==200){
                        buffer[0] = 14;
                        buffer[1] = 2 ;
                        buffer[2] = 1 ;
                        buffer[3] = 4 ;
                        sendMessage(fds[1-i].fd, buffer);
                      }else if (bets[i]==500){
                        buffer[0] = 14;
                        buffer[1] = 2 ;
                        buffer[2] = 1;
                        buffer[3] = 5 ;
                        sendMessage(fds[1-i].fd, buffer);
                      }

                    }
                  }else if (move == 4){
                    if (bet_id == 1){finish(fds, 1-i, bets, pots, hands); startRound=true;}
                    else{finish(fds, 1-i, bets, pots, hands); startRound=true;} //aqui hay que determinar quien gana

                  }

                }else if (id > 24 || id<1) {
        					//Error Not Implemented
                  buffer[0]=24;
                  buffer[1]=0;
                  sendMessage(fds[i].fd, buffer);
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
