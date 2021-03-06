#include "socket.h"

void inicializar_instancia (int socket) {
	unsigned char identificador = INICIALIZAR_INSTANCIA;
	int messageLength = sizeof(int) * 3;
	void * buffer = malloc (messageLength + 6);
	memcpy(buffer, &identificador, 1);
	memcpy(buffer+1, &messageLength, 4);
	memcpy(buffer+5, &cantidad_entradas, 4);
	memcpy(buffer+9, &size_key, 4);
	memcpy(buffer+13, &retardo, 4);
	send(socket, buffer, 5 + messageLength, 0);
	log_info(logger, "Inicializacion enviada correctamente");
	free(buffer);
}

bool find_instancia(void * element) {
	t_instancia * elemento = (t_instancia *) element;
	if(elemento->id == instancia_to_find) {
		return true;
	}

	return false;
}

void restaura_clave(int identificador_instancia, int socket_local) {
	char * message = NULL;
	int size_clave = 0, claves =0, size_package = 0;
	unsigned char identificador = 57;

	void _evaluar_restaurar(char * clave, t_clave * elemento) {
		if(elemento->instancia == identificador_instancia) {
			size_clave = strlen(clave);
			message = (char *)realloc(message,size_package+strlen(clave)+4);
			memcpy(message+size_package, &size_clave, 4);
			memcpy(message+size_package+4, clave, strlen(clave));
			size_package += strlen(clave)+4;
			claves++;
		}
	}

	dictionary_iterator(diccionario_claves, (void *)_evaluar_restaurar);

	if(!claves) identificador = 58;
	send(socket_local, &identificador, 1, 0);
	if(identificador == 57) {
		send(socket_local, &claves, 4, 0);
		send(socket_local, message, size_package, 0);
	}

	return;
}

void _instancia(int socket_local) {
	int rc = 0, close_conn = 0, identificador_instancia = 0;
	int size_clave = 0, size_valor = 0, messageLength = 0;
	bool restore = false;
	unsigned char buffer = 0, identificador = 0, id_response = 0;
	char * mensajes = NULL;
	t_instancia * local_struct;
   int s;
   int optval;
   socklen_t optlen = sizeof(optval);

	rc = recv(socket_local, &identificador_instancia, sizeof(identificador_instancia), 0);
	if (rc == 0) {
		log_error(logger, "  recv() failed");
		close_conn = TRUE;
	}

	log_info(logger, "Esta instancia es: %d", identificador_instancia);

	if(total_instancias != 0) {
		instancia_to_find = identificador_instancia;
		local_struct = list_find(list_instances, (void *)find_instancia);
		if(!local_struct) {
			local_struct = instancia_create(identificador_instancia, cantidad_entradas);
			list_add(list_instances, local_struct);
			total_instancias++;
		    instancias_activas++;
		} else {
			if(!local_struct->status) {
				local_struct->status = true;
			    instancias_activas++;
			}
			restore = true;
		}
	} else {
		local_struct = instancia_create(identificador_instancia, cantidad_entradas);
		list_add(list_instances, local_struct);
		total_instancias++;
	    instancias_activas++;
	}

	inicializar_instancia(socket_local);

	if(restore)
		restaura_clave(identificador_instancia-1, socket_local);
	else {
		identificador = 58;
		send(socket_local, &identificador, 1, 0);
	}

	sem_init(&(local_struct->instance_sem), 1, 0);

	log_info(logger, "Cantidad de instancias -> %d", list_size(list_instances));

	if(algoritmo_elegido == KE) {
		letras_instancia = (25 / instancias_activas) + (25 % instancias_activas);
		log_info(logger, "Cantidad de letras por instancia -> %d", letras_instancia);
	}


	local_struct->socket_instancia = socket_local;

	while(1) {

		sem_wait(&(local_struct->instance_sem));

		log_info(logger, "Pase el semaforo de la instancia %d", local_struct->id);

		if(local_struct->operacion == ESI_GET) {
			identificador = local_struct->operacion;
			rc = send(local_struct->socket_instancia, &identificador, 1, 0);

			log_info(logger, "El send me dice %d", rc);
	        if (rc <= 0) {
	        	log_error(logger, "  recv() failed");
	        	close_conn = TRUE;
	        	break;
	        }

	        log_info(logger, "Le mande a la instancia un %d", identificador);

		}else if(local_struct->operacion == INSTANCIA_COMPACTAR) {
			identificador = local_struct->operacion;
			rc = send(local_struct->socket_instancia, &identificador, 1, 0);

			log_info(logger, "El send me dice %d", rc);
	        if (rc <= 0) {
	        	log_error(logger, "  recv() failed");
	        	close_conn = TRUE;
	        	break;
	        }

	        rc=recv(local_struct->socket_instancia, &identificador, 1, 0);
	        if (rc <= 0) {
				log_error(logger, "  recv() failed");
				close_conn = TRUE;
				break;
			}

	        log_info(logger, "Mando a compactar las instancias, %d", identificador);
		} else {
			size_clave = strlen(local_struct->clave);
			messageLength = 5 + size_clave;
			if(local_struct->operacion == ESI_SET) {
				size_valor = strlen(local_struct->valor);
				messageLength += 4 + size_valor;
			}

			mensajes = (char *) malloc (messageLength);
			memcpy(mensajes, &(local_struct->operacion), 1);
			memcpy(mensajes+1, &size_clave, 4);

			log_info(logger,"Clave a mandar %s", local_struct->clave);
			memcpy(mensajes+5, local_struct->clave, size_clave);
			if(local_struct->operacion == ESI_SET) {
				memcpy(mensajes+size_clave+5, &size_valor, 4);
				log_info(logger,"Valor a mandar %s", local_struct->valor);
				memcpy(mensajes+size_clave+9, local_struct->valor, size_valor);
			}

			log_info(logger, "Enviandole a la instancia %d bytes", messageLength);
			rc = send(local_struct->socket_instancia, mensajes, messageLength, 0);

			log_info(logger, "El send me dice %d", rc);
			if (rc <= 0) {
				log_error(logger, "  recv() failed");
				close_conn = TRUE;
				break;
			}

			free(mensajes);
		}
        rc = recv(local_struct->socket_instancia, &id_response, 1, 0);
        if (rc <= 0) {
        	log_error(logger, "  recv() failed");
        	close_conn = TRUE;
        	break;
        }

        log_info(logger, "La instancia me dice %d", id_response);

        if(local_struct->operacion == INSTANCIA_STATUS) {
			if(id_response == INSTANCIA_VALOR) {
				recv(local_struct->socket_instancia, &size_clave, 4, 0);
				mensajes = (char *) malloc (size_clave + 1);
				recv(local_struct->socket_instancia, mensajes, size_clave, 0);
				mensajes[size_clave] = '\0';
				log_info(logger, "Recibi el valor %s", mensajes);
				local_struct->valor = mensajes;
				sem_post(&mutex_status);
			}else if(id_response == INSTANCIA_ERROR) {
				local_struct->valor = NULL;
				sem_post(&mutex_status);
			}
        }


        log_info(logger, "La instancia termino de procesar");
        sem_post(&mutex_instancia);

        if(id_response == INSTANCIA_COMPACTAR) {
            log_info(logger, "Las instancias necesitan compactar");

            void compactarInstancias(void * element) {
            	t_instancia * instancia = (t_instancia *) element;
            	if(instancia->status && instancia->id != identificador_instancia) {
            		instancia->operacion = INSTANCIA_COMPACTAR;
            		sem_post(&(instancia->instance_sem));
            	}
            }

            list_iterate(list_instances, (void *)compactarInstancias);
            rc = recv(local_struct->socket_instancia, &id_response, 1, 0);

        } else if(id_response == INSTANCIA_OCUPADAS) {
        	int entradasOcupadas = 0;
        	local_struct->entradasLibres = cantidad_entradas;
        	rc = recv(local_struct->socket_instancia, &(entradasOcupadas), 4, 0);
        	local_struct->entradasLibres -= entradasOcupadas;

        	//Marito, esto es una villa
        	if(algoritmo_elegido_anterior == LSU && algoritmo_elegido == EL && local_struct->entradasLibres < cantidad_entradas)
        		algoritmo_elegido = LSU;

        	log_info(logger, "La instancia %d tiene %d entradas ocupadas", identificador_instancia, local_struct->entradasLibres);
        }
	}


	if (close_conn) {
		//shutdown(socket_local, SHUT_RDWR);
		log_error(logger, "La instancia %d esta caida", identificador_instancia);
		instancias_activas--;
		local_struct->status = false;
		if(algoritmo_elegido == KE) {
			letras_instancia = (25 / instancias_activas) + (25 % instancias_activas);
			log_info(logger, "Cantidad de letras por instancia -> %d", letras_instancia);
		}
		sem_post(&mutex_instancia);
	}
}
