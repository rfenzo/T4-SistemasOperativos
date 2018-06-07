#include <stdlib.h> //atoi
#include <stdio.h> //printf
#include <arpa/inet.h> //socket
#include <stdbool.h> //true, false
#include <unistd.h> //read
#include <string.h> //strlen
#include <time.h> //time

struct card{
  int pinta;
  int numero;
  bool taken;
};
typedef struct card Card;

struct player{
  int socket;
  char* nickname;
  Card* hand[5];
};
typedef struct player Player;

Player* getPlayer(int sd, Player** connected){
  for (int i = 0; i < 2; i++) {
    if (connected[i]->socket == sd) {
      return connected[i];
    }
  }
  return NULL;
}

void swap (Card* a, Card* b){
    Card temp = *a;
    *a = *b;
    *b = temp;
}

Card** generateDeckOfCards(Card** deck){
  // Card** deck = malloc(sizeof(Card)*52);
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 4; j++) {
      deck[i+j]->numero = i+1;
      deck[i+j]->pinta = j+1;
      deck[i+j]->taken = false;
    }
  }
  //shuffle
  srand(time(NULL));
  for (int i = 52-1; i > 0; i--){
      int j = rand() % (i+1);
      swap(deck[i], deck[j]);
  }
  return deck;
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

bool compareCards(Card* card1, Card* card2){
  return (card1->numero == card2->numero && card1->pinta == card2->pinta);
}

void changeCards(Player* currentPlayer, char* payload, Card** deck){
  Card** oldCards = malloc(sizeof(Card)*(sizeof(payload)/2));
  for (int i = 0; i < sizeof(payload); i++) {
    if (i % 2 == 0) {
      oldCards[i/2]->numero = (int)payload[i];
    }else{
      oldCards[i/2]->pinta = (int)payload[i];
    }
  }
  //cambiar las cartas del player con unas nuevas
  for (int j = 0; j < sizeof(payload)/2; j++) {
    for (int i = 0; i < 5; i++) {
      if (compareCards(currentPlayer->hand[i], oldCards[j])) {
        Card* newCard = getRandomCard(deck);
        if (newCard) {
          currentPlayer->hand[i] = newCard;
        }else{
          //WARNING que hacer?
          printf("No quedan cartas en el mazo\n");
        }
        break;
      }
    }
  }
  //quizas se deba retornas las nuevas cartas
}

// char* readBuffer(char* buffer,unsigned int* id, unsigned int* payloadSize){
char* readBuffer(char* buffer,unsigned int* id){
  *id = (int) (buffer[0] -'0');
  unsigned int payloadSize = (int)(buffer[1]-'0');
  char* payload = malloc(payloadSize);
  for (int i = 0; i < payloadSize; i++) {
    payload[i] = buffer[2+i];
  }
  return payload;
}

void sendMessage(int socket, char* message){
  send(socket, message, sizeof(message),0);
}

int main(int argc, char const *argv[]) {
  //WARNING falta hacer que los argumentos puedan ser pasados en desorden
  //WARNING falta hacer que no se caiga al recibir malos paquetes

  if (argc != 3) {
		printf("Argumentos inadecuados\n$ ./server -i <ip_address> -p <tcp-port>\n");
		return 1;
	}

  const char* IP = argv[1];
  int PORT = atoi(argv[2]);
  int MAX_CLIENTS = 30;


  int clientSockets[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++){
      clientSockets[i] = 0;
  }

  int masterSocket = socket(AF_INET , SOCK_STREAM , 0);
  if( masterSocket == 0){
      printf("Server socket falló en su creación");
      return 2;
  }
  int opt = 1;
  int reuseaddr = setsockopt(masterSocket,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (char *)&opt,
                            sizeof(opt));
  if( reuseaddr  < 0 ){
    printf("Error al setear reuseaddr\n");
    return 5;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(IP);
  address.sin_port = htons( PORT );

  int stat = bind(masterSocket, (struct sockaddr*)&address, sizeof(address));
  if (stat<0){
      printf("Bind del server socket falló");
      return 3;
  }
  printf("Servidor escuchando el puerto %d\n", PORT);

  if (listen(masterSocket, 3) < 0){
      printf("Listen del server socket falló");
      return 4;
  }

  int addressLength = sizeof(address);
  unsigned int max_sd, sd, i, actividad, new_socket, readedBytes, id;
  // unsigned int  payloadSize;
  char buffer[256];
  fd_set readfds;

  //almacena el socket de los usuarios a los que se aceptó su start connection
  Player* connected[2] = { malloc(sizeof(Player)), malloc(sizeof(Player)) };
  bool acceptedConnection;
  Card* deck[52];

  while (true) {
    //setear sockets activos
    FD_ZERO(&readfds);
    FD_SET(masterSocket,&readfds);
    max_sd = masterSocket;
    for (i = 0; i < MAX_CLIENTS; i++) {
      sd = clientSockets[i];
      if (sd > 0)
        FD_SET(sd,&readfds);
      if (sd > max_sd)
        max_sd = sd;
    }

    //checkear actividad en los sockets
    actividad = select(max_sd+1, &readfds, NULL, NULL, NULL);
    if (actividad < 0)
      printf("Error en el select, al buscar actividad\n");

    //revisar masterSocket
    if (FD_ISSET(masterSocket, &readfds)) {
      //nueva conexion
      new_socket = accept(masterSocket,
                          (struct sockaddr*)&address,
                          (socklen_t*)&addressLength);

      //agregarlo a una posicion vacia de clientSockets
      for (i = 0; i < MAX_CLIENTS; i++) {
        if (clientSockets[i] == 0) {
          clientSockets[i] = new_socket;
          // printf("Agregando cliente a lista de sockets en casilla %i, con valor %i\n",i, new_socket);
          break;
        }
      }
    }

    //revisar clientes
    for (i = 0; i < MAX_CLIENTS; i++) {
      sd = clientSockets[i];

      //NULL si no está conectado
      Player* currentPlayer = getPlayer(sd, connected);

      if (FD_ISSET(sd, &readfds)) {
        //escuchar al cliente
        readedBytes = read( sd , buffer, 256);
        if (readedBytes == 0){
            //Un cliente se desconectó
            getpeername(sd ,
                        (struct sockaddr*)&address ,
                        (socklen_t*)&addressLength);
            printf("Cliente de ip %s se desconectó\n" ,
                    inet_ntoa(address.sin_addr));

            close(sd);
            clientSockets[i] = 0;
        }else{
          //aca se debe hacer todo el manejo de peticiones por parte del cliente
          // char* payload = readBuffer(buffer, &id, &payloadSize);
          char* payload = readBuffer(buffer, &id);

          //convertir a handleRequest
          if (id == 1) {
            //Start Connection
            printf("Cliente solicitó conexión ");
            acceptedConnection = false;
            for (i = 0; i < 2; i++) {
              if (connected[i]->socket == 0) {
                connected[i]->socket = sd;
                acceptedConnection = true;
                break;
              }
            }
            if (acceptedConnection) {
              //id 2, payloadSize 0, payload 0
              sendMessage(sd, "200");
              printf("y se aceptó\n");
            }else{
              //WARNING que hacer?
              //lo correcto sería advertirle de que no fue posible conectar
              printf("pero se rechazó pues ya hay dos jugadores\n");
            }
          }else if (id == 24) {
            //Error Not Implemented
          }else if (currentPlayer) {
            if (id == 4) {
              //Return Nickname
              currentPlayer->nickname = payload;
            }else if (id == 13) {
              //Return Cards to Change
              changeCards(currentPlayer, payload, deck);
            }else if (id == 15) {
              //Return Bet
              //WARNING Falta hacer ésto
            }
          }else{
            //WARNING que hacer?
            printf("Cliente no conectado enviando data: id:%i | payload: %s\n", id,payload);
          }


          // printf("Se recibió %s\n",buffer );
          // send(sd,buffer,strlen(buffer),0);
        }
      }
    }
  }
  return 0;
}
