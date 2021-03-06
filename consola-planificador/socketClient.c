/*
 * SocketClient.c
 *
 *  Created on: 24 abr. 2018
 *      Author: Pablo Delucchi
 */

#include "socketClient.h"
#define IDENTIDAD "consola-planificador"


int connect_to_server(char * ip, char * port) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, port, &hints, &server_info);  // Carga en server_info los datos de la conexion

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	int res = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);  // No lo necesitamos mas

	if (res < 0) {
		_exit_with_error(server_socket, "No me pude conectar al servidor", NULL);
	}

	log_info(logger, "Conectado!");

	return server_socket;
}

void _exit_with_error(int socket, char* error_msg, void * buffer) {
  	if (buffer != NULL) free(buffer);
	log_error(logger, error_msg);
	close(socket);
	exit_gracefully(1);
}

void exit_gracefully(int return_nr) {
	log_destroy(logger);
	exit(return_nr);
}

void configure_logger() {
	logger = log_create("consola-planificador.log", "consola-planificador", true, LOG_LEVEL_INFO);
}

void wait_hello(int socket) {

	char * hola = "identify";

    char * buffer = (char*) calloc(sizeof(char), strlen(hola) + 1);

    int result_recv = recv(socket, buffer, strlen(hola), MSG_WAITALL); //MSG_WAITALL

	printf("Recibi %s\n",buffer);

    if(result_recv <= 0) {
      _exit_with_error(socket, "No se pudo recibir hola", buffer);
    }

    if (strcmp(buffer, hola) != 0) {
      _exit_with_error(socket, "No es el mensaje que se esperaba", buffer);
    }

    log_info(logger, "Mensaje de hola recibido: '%s'", buffer);

	send(socket, IDENTIDAD, strlen(IDENTIDAD), 0);

    free(buffer);
}

void send_message(int socket){
	char mensaje[1000];

	char * server_reply = NULL;

	while (1) {

		printf("escribi algo!\n");

  		scanf("%s", mensaje);

  		printf("valor de mensaje %s!\n", mensaje);

  		send(socket, mensaje, strlen(mensaje), 0);

  		log_info(logger,"envie %s\n", mensaje);

  		server_reply = (char *)calloc(sizeof(char), 1000);

		//Receive a reply from the server
		if(recv(socket , server_reply , 200 , 0) < 0)
		{
			puts("recv failed");

			break;
		}

  		log_info(logger,"recibi %s\n", server_reply);

  		free(server_reply);
  	}
}

// esto no va aca
int create_client(char * ip, char * port){
	 int socket_server = 0;

	 socket_server = connect_to_server(ip, port);

	 wait_hello(socket_server);

	 return socket_server;
}
