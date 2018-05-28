/*
 * socket_client.h
 *
 *  Created on: May 28, 2018
 *      Author: fmocchetti
 */

#ifndef SOCKET_CLIENT_H_
#define SOCKET_CLIENT_H_

#include <stdio.h>
#include <openssl/md5.h> // Para calcular el MD5
#include <string.h>
#include <stdlib.h> // Para malloc
#include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
#include <netdb.h> // Para getaddrinfo
#include <unistd.h> // Para close
#include <readline/readline.h> // Para usar readline
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

// Definimos algunas variables globales
t_log * logger;

enum mensajes {
	IDENTIFY = 10
};

enum { IDENTIFY_ESI = 1, IDENTIFY_INSTANCIA = 2, IDENTIFY_PLANIFICADOR = 3};

int create_client(char*, char *);
void configure_logger();
int connect_to_server(char *, char *);
void wait_hello(int);
void send_message(int);
void _exit_with_error(int, char*, void*);
void exit_gracefully(int);

#endif /* SOCKET_CLIENT_H_ */
