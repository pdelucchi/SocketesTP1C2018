#include "socket.h"

void _planificador(int socket_local) {
	int rc = 0, close_conn = 0;
	int size_clave = 0, cantidad_claves = 0;
	t_instancia * instancia_destino = NULL;
	unsigned char identificador = 0;
	char * clave = NULL;
	t_clave * clave_diccionario = NULL;

	thread_planificador = planificador_create();

	while(1) {
        rc = recv(socket_local, &identificador, 1, 0);
        if (rc == 0) {
        	log_error(logger, "  recv() failed");
        	close_conn = TRUE;
        	break;
        }

        log_info(logger, "El Planificador me esta diciendo: %d" , identificador);

        switch(identificador) {
			case PLANIFICADOR_CLAVE:
				sem_wait(&mutex_planificador);
				log_info(logger, "Contestando pedido de clave");
				int messageLength = 1;
				int size_clave = strlen(thread_planificador->clave);
				if(thread_planificador->status != COORDINADOR_SET) {
					messageLength += 5 + size_clave;
				}

				log_info(logger, "Le voy a mandar al planificador un %d", thread_planificador->status);
				char * mensajes = (char *) malloc (messageLength);
				memcpy(mensajes, &(thread_planificador->status), 1);
				if(thread_planificador->status != COORDINADOR_SET) {
					memcpy(mensajes+1, &size_clave, 4);
					memcpy(mensajes+5, thread_planificador->clave, size_clave);
					messageLength -= 1;
				}

				log_info(logger, "Enviandole al planificador %d bytes", messageLength);
				send(socket_local, mensajes, messageLength, 0);
				free(mensajes);

				break;
			case PLANIFICADOR_BLOQUEAR:
				recv(socket_local, &cantidad_claves, 4, 0);
				for(int i=0; i < cantidad_claves; i++) {
					recv(socket_local, &size_clave, 4, 0);
					clave = (char *)malloc (size_clave + 1);
					recv(socket_local, clave, size_clave, 0);
					clave[size_clave] = '\0';

					log_info(logger, "Voy a bloquear la clave '%s'", clave);
					printf("%s\n",clave);
	        		if(dictionary_has_key(diccionario_claves, clave)) {
	        			clave_diccionario = (t_clave * ) dictionary_get(diccionario_claves, clave);
	        			if(!clave_diccionario->tomada) {
	            			clave_diccionario->tomada = true;
	            			instancia_destino = distribuir(clave, NULL);
	            			clave_diccionario->instancia = instancia_destino->id;
	        			}
	        		} else {
	        			instancia_destino = distribuir(clave, NULL);
	        			dictionary_put(diccionario_claves, clave, clave_create(0, instancia_destino->id, true));
	        		}
	        		sem_wait(&mutex_instancia);
	        		log_info(logger, "Se almaceno la clave en la instancia %d", instancia_destino->id);
	        		//free(clave);
				}
				break;
			case PLANIFICADOR_DESBLOQUEAR:
				recv(socket_local, &size_clave, 4, 0);
				clave = (char *)malloc (size_clave + 1);
				recv(socket_local, clave, size_clave, 0);
				clave[size_clave] = '\0';

				log_info(logger, "Voy a desbloquear la clave '%s'", clave);
				if(dictionary_has_key(diccionario_claves, clave)) {
					clave_diccionario = (t_clave * ) dictionary_get(diccionario_claves, clave);
					store_clave(clave, clave_diccionario->instancia);
					clave_diccionario->tomada = false;
					log_error(logger, "clave desbloqueada");
				} else {
					log_error(logger, "Intentando desbloquear una clave inexistente");
				}
				//free(clave);
				break;
			case PLANIFICADOR_STATUS:
				recv(socket_local, &size_clave, 4, 0);
				clave = (char *)malloc (size_clave + 1);
				recv(socket_local, clave, size_clave, 0);
				clave[size_clave] = '\0';

				log_info(logger, "Voy a consultar estado de la clave '%s'", clave);

				/*if(dictionary_has_key(diccionario_claves, clave)) {
					clave_diccionario = (t_clave * ) dictionary_get(diccionario_claves, clave);

					log_info(logger, "Enviandole al planificador %d bytes", messageLength);
					send(socket_local, mensajes, messageLength, 0);
					free(mensajes);


					//preguntar a instancia el valor
					//ver en que instancia esta
					//simular distribucion

				} else {

				}*/


				break;
        }
	}

	if (close_conn) {
		shutdown(socket_local, SHUT_RDWR);
	}

}
