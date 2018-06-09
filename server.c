#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

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
  *id = (int) (buffer[0] -'0');
  *payloadSize = (int)(buffer[1]-'0');
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
    buffer[0] = 5+'0';
    buffer[1] = strlen(nicknames[i])+'0';
    buffer[2] = 0; // hace que se concatene desde buffer[2]
    strcat(buffer,nicknames[i]);
    sendMessage(fds[1-i].fd, buffer);
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
  buff[0] = 10+'0';
  buff[1] = 10+'0';
  Card* card;
  for (int i = 0; i < 5; i++) {
    card = hand[i];
    buff[2*i+2] = card->numero+'0';
    buff[2*i+3] = card->pinta+'0';
  }
  sendMessage(fd.fd, buff);
}

void sendBothHands(struct pollfd* fds, Card*** hands){
  for (int i = 0; i < 2; i++) {
    sendHand(fds[i],hands[i]);
  }
}

void giveInitialCards(struct pollfd* fds, Card** deck, Card*** hands){
  for (int j = 0; j < 2; j++) {
    Card* card;
    for (int i = 0; i < 5; i++) {
      card = getRandomCard(deck);
      if (card) {
        hands[j][i] = card;
      }else{
        //WARNING puede pasar esto?
        printf("ERROR: No quedan cartas en el mazo\n");
        break;
      }
    }
  }
}

bool compareCards(Card* card1, Card* card2){
  return (card1->numero == card2->numero && card1->pinta == card2->pinta);
}

void changeCards(Card** hand, char* payload, int payloadSize, Card** deck){
  Card** oldCards = malloc(sizeof(Card)*(payloadSize/2));
  for (int i = 0; i < payloadSize; i++) {
    if (i % 2 == 0) {
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
  Card* deck[52];

  int timeout_msecs = 500;
  int ret, i, readedBytes, id, payloadSize;
  char buffer[258];
  int playercount = 0;

  bool startRound = false;
  bool betsTime = false;
  bool whosfirst = false;
  bool waitPlayer0 = false;
  bool waitPlayer1 = false;
  int starter = 0; //permite alternar quien comienza las rondas
  int turn;

  while(true){
    if (startRound) {
      updatePots(fds, pots, 6);
      initialBet(fds, pots);
      giveInitialCards(fds, deck, hands);
      sendBothHands(fds, hands);
      //get cards to change
      buffer[0] = 12 + '0';
      buffer[1] = '0';
      buffer[2] = '0';
      sendMessage(fds[0].fd, buffer);
      sendMessage(fds[1].fd, buffer);

      startRound = false;
      betsTime = true;
      whosfirst = true;
      waitPlayer0 = true;
      waitPlayer1 = true;
      turn = starter;
    }else if (betsTime && !waitPlayer0 && !waitPlayer1) {
      if (whosfirst) {
        buffer[0] = 11 + '0';
        buffer[1] = '1';
        buffer[2] = '1';
        sendMessage(fds[starter].fd, buffer);
        buffer[2] = '2';
        sendMessage(fds[1-starter].fd, buffer);
        starter = 1-starter;
        whosfirst = false;
      }

      if (turn == 0) {
        buffer[0] = 14 +'0';
        buffer[1] = 5 + '0';
        for (i = 2; i < 7; i++) {
          buffer[i] = i-1 + '0';
        }
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
            sleep(1);
						readedBytes = read(fds[i].fd, buffer, 258);
						if (readedBytes == 0) {
							printf("Cliente se ha desconectado\n");
							fds[i].fd = -1;
							// WARNING que pasa acá?.
						}else{
              char* payload = readBuffer(buffer, &id, &payloadSize);
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
                  playercount++;
                  if (playercount == 2) {
                    opponentFound(nicknames, fds);
                    generateDeckOfCards(deck);
                    //game start
                    sendMessage(fds[0].fd, "700");
                    sendMessage(fds[1].fd, "700");
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
                  //Return Bet
                  if (i == 0) {
                    waitPlayer0 = false;
                  }else if (i == 1) {
                    waitPlayer1 = false;
                  }
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
