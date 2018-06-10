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

int hand_value(int class, int rank){
	return (13 * class) + rank;
}

int card_comparator(const void* a, const void * b){
  const Card * c1 = *(const Card **)a;
  const Card * c2 = *(const Card **)b;
  return (c1->numero - c2->numero);
}

void sort_cards(Card** cards){
	qsort(cards, 5, sizeof(Card*), card_comparator);
}

int getPoints(Card** hand, char* nickname){
  int i, oneP, twoP, threeP, fourP, fiveP, oneN, twoN, threeN, fourN, fiveN;
  sort_cards(hand);

  oneP   = hand[0]->pinta; oneN = hand[0]->numero;
  twoP   = hand[1]->pinta; twoN = hand[1]->numero;
  threeP = hand[2]->pinta; threeN = hand[2]->numero;
  fourP  = hand[3]->pinta; fourN = hand[3]->numero;
  fiveP  = hand[4]->pinta; fiveN = hand[4]->numero;

  if ((oneP == twoP && twoP == threeP && threeP == fourP && fourP == fiveP) &&
  (oneN == 10 && twoN == 11 && threeN == 12 && fourN == 13 && fiveN == 1)){
    printf("Jugador %s, Royal Flush\n", nickname);
    return hand_value(9, fiveN);
  }

  /* Straight Flush */
  /* has a class rank of 9 */
  if (oneP == twoP && twoP == threeP && threeP == fourP && fourP == fiveP &&
    fiveN == fourN + 1 && fourN == threeN + 1 && threeN == twoN + 1 && twoN == oneN + 1){
      printf("Jugador %s, Straight Flush\n", nickname);
      return hand_value(9, fiveN);
  }

  /* Four-of-a-Kind */
  /* has a class rank of 8 */
  if ((oneN == twoN && twoN == threeN && threeN == fourN) ||
    (twoN == threeN && threeN == fourN && fourN == fiveN)){
      printf("Jugador %s, Four of a kind\n", nickname);
      return hand_value(8, threeN);
  }

  /* Full House */
  /* has a class of 7 */
  if ((oneN == twoN && twoN == threeN && fourN == fiveN) ||
      (oneN == twoN && threeN == fourN && fourN == fiveN)) {
      printf("Jugador %s, Full House\n", nickname);
      return hand_value(7, fiveN);
  }

  /* Flush */
  /* has a class of 6 */
  if (oneP == twoP && twoP == threeP && threeP == fourP && fourP == fiveP){
    printf("Jugador %s, Flush\n", nickname);
    return hand_value(6, fiveN);
  }


  /* Straight */
  /* has a class of 5 */
  if (fiveN == fourN+1 && fourN+1 == threeN+2 && threeN+2 == twoN+3 && twoN+3 == oneN+4){
    printf("Jugador %s, Straight\n", nickname);
    return hand_value(5, fiveN);
  }

  /* Three-of-a-Kind */
  /* has a class rank of 4 */
  for (i = 0; i < 5 - 2; ++i){
    if ((hand[i]->numero == hand[i+1]->numero) && (hand[i+1]->numero == hand[i+2]->numero)){
      printf("Jugador %s, 3 of a kind\n", nickname);
      return hand_value(4, threeN);
    }
  }

  /* Two-Pair */
  /* has a class rank of 3 */
  if ((oneN == twoN && threeN == fourN) ||
    (twoN == threeN && fourN == fiveN) ||
    (oneN == twoN && fourN == fiveN)) {
      printf("Jugador %s, Two Pair\n", nickname);
      return hand_value(4, fourN);
  }

  /* One-Pair */
  /* has a class rank of 2 */
  for (i = 0; i < 5-1; ++i) {
    if (hand[i]->numero == hand[i+1]->numero){
      printf("Jugador %s, One Pair\n", nickname);
      return hand_value(2, hand[i]->numero);
    }
  }
  /* High Card */
  /* has a class rank of 1 */
  printf("Jugador %s, High - Card\n", nickname);
  return hand_value(1, fiveN);
}

int identifyWinner(Card*** hands, char** nicknames){
  return getPoints(hands[0],nicknames[0]) < getPoints(hands[1],nicknames[1]);
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
  //Actualizo Pots y bets
  pots[winner] += bets[0];
  pots[winner] += bets[1];
  bets[0] = 0;
  bets[1] = 0;

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

  //Enviar si gano o perdio
  for (int p=0;p<2;p++){
    buff[0]=20;
    buff[1]=1;
    if (p == winner){
      buff[2]=1;
      sendMessage(fds[p].fd, buff);
    }else{
      buff[2]=2;
      sendMessage(fds[p].fd, buff);
    }
  }

  //Envio update pots
  for (int p=0; p<2;p++){
    sendInteger(fds[p], pots[p], 21);
  }
}

void initialBet(struct pollfd fd, int* pot, int* bet){
  *pot -= 10;
  *bet += 10;
  char buff[200];
  buff[0] = 9;
  buff[1] = 1;
  buff[2] = 10;
  sendMessage(fd.fd, buff);
}

void changeBet(int* bets, int* pots, int bet_id, int player){
  if (bet_id == 3 ){
    pots[player] -= (100 - bets[player]);
    bets[player] = 100;
  }else if (bet_id == 4){
    pots[player] -= (200 - bets[player]);
    bets[player] = 200;
  }else if (bet_id == 5){
    pots[player] -= (500 - bets[player]);
    bets[player] = 500;
  }
}

int betIdToInt(int id){
  if (id==2) return 0;
  else if(id==3)return 100;
  else if (id==4) return 200;
  else if (id==5) return 500;
  else return -1;
}

void betOptions(int* buffer, int* money_available, int* pots, int player, int move){
  for (int e=0; e<5; e++){
    buffer[e] = 0;
  }
  int diferencia_apuestas = pots[1-player] - pots[1-player];
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

int sendImage(struct pollfd fd,int tipo){
  char filename[15];
  if (tipo == 0) {
    strcpy(filename, "winner.gif");
  }else if (tipo == 1) {
    strcpy(filename, "loser.gif");
  }
  printf("Enviando imagen %s\n", filename);

  FILE* file = fopen(filename, "rb");
  if (!file){
    printf("Unable to open file %s", filename);
    return 1;
  }

  fseek(file, 0, SEEK_END);
  unsigned long fileLen = ftell(file);
  fseek(file, 0, SEEK_SET);

  //Allocate memory
  char* buffer = malloc(fileLen);
  if (!buffer){
    printf("Memory error!");
    fclose(file);
    return 2;
  }
  fread(buffer,fileLen,1,file);
  fclose(file);

  char* buffer2 = malloc(fileLen+2);
  buffer2[0] = 23;
  buffer2[1] = fileLen;
  for (int i = 0; i < fileLen; i++) {
    buffer2[i+2] = buffer[i];
  }
  sendMessage(fd.fd, buffer2);
  return 0;
}

int getMinBet(int otherPlayerBet){
  if (otherPlayerBet == 10) {
    return 2;
  }else if (otherPlayerBet == 100) {
    return 3;
  }else if (otherPlayerBet == 200) {
    return 4;
  }else if (otherPlayerBet == 500) {
    return 5;
  }
  return -1;
}

void sendAvailableBets(struct pollfd* fds, int* bets, int turn, bool firstTurn){
  char buffer[50];
  int minBet = getMinBet(bets[1-turn]);
  if (minBet < 0) {
    printf("Error! Revisar getMinBet linea ~484\n");
  }
  buffer[0] = 14;
  int j = 2;
  if (!firstTurn) {
    buffer[2] = 1;
    j = 3;
  }
  buffer[1] = 6-minBet+(j-2);
  for (int i = minBet; i < 6; i++) {
    buffer[j] = i;
    j++;
  }
  sendMessage(fds[turn].fd, buffer);
}

int main (int argc, char *argv[]){
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

	int welcomeSocket;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;

	welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr(IP);
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
  bool whosfirst = false;
  bool wait[2] = {false, false};
  int starter = 0; //permite alternar quien comienza las rondas
  int turn;
  int move;

  while(true){
    if (startRound) {
      for (i = 0; i < 2; i++) {
        if (pots[i]<10) {
          sendImage(fds[i], 1);
          sendImage(fds[1-i], 0);
          buffer[0] = 22;
          buffer[1] = 0;
          buffer[2] = 0;
          sendMessage(fds[i].fd, buffer);
          sendMessage(fds[1-i].fd, buffer);
          return 0;
        }
      }
      for (i = 0; i < 2; i++) {
        //start round, envia pot actuales
        sendInteger(fds[i], pots[i], 8);

        //initial bet, envia monto de apuesta inicial y descuenta de su pot
        initialBet(fds[i], &pots[i], &bets[i]);

        //setea en el servidor las cartas de los jugadores
        giveInitialCards(fds[i], deck, hands[i]);

        //5-Cards, envia las cartas de los jugadores
        sendHand(fds[i], hands[i]);

        //get cards to change, pregunta a los jugadores si quieres cambiar cartas
        buffer[0] = 12;
        buffer[1] = 0;
        buffer[2] = 0;
        sendMessage(fds[i].fd, buffer);
      }
      startRound = false;
      betsTime = true;
      wait[0] = true;
      wait[1] = true;
      whosfirst = true;
      starter = 1 - starter;
      turn = starter;
      move = 1;
    }else if (betsTime && !wait[0] && !wait[1]) {
      if (whosfirst) {
        //Who's first: Envio id 11 de si parto o espero
        buffer[0]=11;
        buffer[1]=1;
        buffer[2]=1;
        sendMessage(fds[turn].fd, buffer);
        buffer[2]=2;
        sendMessage(fds[1-turn].fd, buffer);
        whosfirst = false;
      }
      wait[turn] = true;
      if (move == 1) {
        sendAvailableBets(fds, bets, turn, true);
      }else if (move == 2) {
        sendAvailableBets(fds, bets, turn, false);
      }else if (move == 3) {
        if (bets[turn]==bets[1-turn]){
          finish(fds, identifyWinner(hands, nicknames), bets, pots, hands);
          startRound=true;
        }else{
          sendAvailableBets(fds, bets, turn, false);
        }
      }else if (move == 4) {
        finish(fds, identifyWinner(hands, nicknames), bets, pots, hands);
        startRound=true;
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
                printf("ID24: Error Not Implemented\n");
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
                    //initial pots
                    sendInteger(fds[0], pots[0], 6);
                    sendInteger(fds[1], pots[1], 6);
                    startRound = true;
                  }
                }else if (id == 13) {
                  //Return Cards to Change
                  changeCards(hands[i], payload, payloadSize, deck);
                  sendHand(fds[i], hands[i]);
                  wait[i] = false;

                }else if (id == 15) {
                  //Return Bet
                  int betId = payload[0];
                  int apuesta = betIdToInt(betId);
                  if (apuesta < 0 && move != 1) {
                    printf("Jugador %i hizo FOLD, terminando la partida\n",i);
                    finish(fds, 1-i, bets, pots, hands);
                    startRound=true;
                  }else{
                    if (apuesta > pots[i]+bets[i] || (apuesta < 0 && move == 1)) {
                      //Send Error bet
                      printf("Jugador %i mandó bet inadecuado, enviando ID 16\n",i);
                      buffer[0] = 16;
                      buffer[1] = 0;
                      sendMessage(fds[i].fd, buffer);
                    }else{
                      printf("Jugador %i hizo bet de %i\n",i, apuesta);
                      changeBet(bets, pots, betId, i);
                      buffer[0] = 17;
                      buffer[1] = 0;
                      sendMessage(fds[i].fd, buffer);
                      move += 1;
                      turn = 1-i;
                      wait[i] = false;
                    }
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

