#include <stdlib.h> //atoi
#include <stdio.h> //printf
#include <arpa/inet.h> //socket
#include <stdbool.h> //true, false
#include <unistd.h> //read
#include <string.h> //strlen

int main(int argc, char const *argv[]) {
  if (argc != 3) {
		printf("Argumentos inadecuado\n$ ./server -i <ip_address> -p <tcp-port>\n");
		return 1;
	}

  const char* IP = argv[1];
  int PORT = atoi(argv[2]);
  int MAX_CLIENTS = 2;

  fd_set readfds;

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
  // address.sin_addr.s_addr = INADDR_ANY;
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
  int max_sd, sd, i, actividad, new_socket, readedBytes;
  char buffer[1024];

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
    if (actividad<0)
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
      printf("%i: %i\n",  i, sd);
      if (FD_ISSET(sd, &readfds)) {
        //escuchar al cliente
        readedBytes = read( sd , buffer, 1024);
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
          printf("Se recibió %s\n",buffer );
          send(sd,buffer,strlen(buffer),0);
        }
      }
    }
  }
  return 0;
}
