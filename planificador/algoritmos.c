/*
 * algoritmos.c
 *
 *  Created on: 12 may. 2018
 *      Author: pdelucchi
 */

#include "algoritmos.h"

int result_connection = 0;
bool result_satisfy =false;
char claveGlobal[100];

void laWeaReplanificadoraFIFO(t_list * listaDestino, t_list *listaEntrada){
    t_list * tempList = list_create();
    tempList = list_take_and_remove(listaEntrada, 1);
    list_add_all(listaDestino, tempList);
    list_destroy(tempList);
}

//muestra el estado de los procesos encolados en las listas existentes
void estadoListas(){
    int cantidadListos = 0;
    int cantidadEjecucion = 0;
    int cantidadBloqueados = 0;
    int cantidadTerminados = 0;
    int cantidadMuertos = 0;
    cantidadEjecucion = list_size(ejecucion);
    cantidadListos = list_size(listos);
    cantidadBloqueados = list_size(bloqueados);
    cantidadTerminados = list_size(terminados);
    cantidadMuertos = list_size(muertos);
    printf("Procesos en la cola de listos %d\n",cantidadListos);
    printf("Procesos en la cola de ejecucion %d\n",cantidadEjecucion);
    printf("Procesos en la cola de bloqueados %d\n",cantidadBloqueados);
    printf("Procesos en la cola de terminados %d\n",cantidadTerminados);
    printf("Procesos en la cola de muertos %d\n",cantidadMuertos);

}

void sjfsd(){
	//sem_init(&mutex_ejecucion, 0, 1);
	//pthread_mutex_init(&mutex_ejecucion,NULL);
	//int lista_vacia = list_is_empty(ejecucion);
	int sem_value = 0;

	unsigned char permisoDeEjecucion = 1;
	unsigned char contestacionESI = 0;

	while(1){
		ESI *nodo_lista_ejecucion = NULL;
		printf("Estas en SJFSD\n");
		printf("Esperando que haya un nuevo proceso encolado en listos\n");
		//espero a que me digan que hay algo en la cola de listos
		sem_wait(&new_process);
		printf("Nuevo elemento en la cola de listos\n");
		estadoListas();

		//replanifico aca, dependiendo de la rafaga
		list_sort(listos, (void*)sort_by_estimacion);

		//Muevo de la lista de listos, el primer nodo a la lista de ejecucion
		laWeaReplanificadoraFIFO(ejecucion,listos);
		printf("Nodo de listos movido a Ejecucion\n ");
		nodo_lista_ejecucion =  (ESI*) list_get(ejecucion, 0);
		id_esi_global = nodo_lista_ejecucion->id_ESI;

		//Mientras la cantidadDeLineas de la ESI en ejecucion sea mayor a 0
		while(nodo_lista_ejecucion->cantidadDeLineas >0){
			sem_getvalue(&sem_pausar_planificacion,&sem_value);
			if(sem_value<1){
				sem_wait(&sem_pausar_algoritmo);
			}

			//Envio al socket de la esi que esta en ejecucion, que puede ejecutarse
			send(nodo_lista_ejecucion->socket_esi, &permisoDeEjecucion, 1, 0);

			//Espero que la esi me conteste
			result_connection = recv(nodo_lista_ejecucion->socket_esi, &contestacionESI, 1,0);
			if (result_connection <= 0) {
				//////////elimino de lista de claves tomadas la ESI y hago un Store avisando que otra clave puede pasarse a ready
				printf("------RECIBI MENOS DE 0\n");
				/*claves* clave_temporal = (claves*) malloc(sizeof(claves));
				clave_temporal = list_remove_by_condition(claves_tomadas,identificador_ESI);
				ESI_STORE(clave_temporal->claveAEjecutar);
				//list_remove_and_destroy_by_condition(claves_tomadas,(void*)identificador_ESI,(void*)clave_destroy);
				free(clave_temporal);*/
				//////////
				//hago close del socket
				_exit_with_error(nodo_lista_ejecucion->socket_esi, "La ESI en ejecucion murio", NULL);
				//break;
				nodo_lista_ejecucion->cantidadDeLineas = 0;

				printf("------RECIBI MENOS DE 123123132\n");
			}

			printf("contestacionESI %d\n",contestacionESI);
			//Si es 1, entonces espero que me envie la nueva cantidad de Lineas que tiene

			//magia con el coord (Recibe si es store/get y realiza acorde)
			coord_communication(nodo_lista_ejecucion->socket_esi,nodo_lista_ejecucion->id_ESI ,contestacionESI);

			if(contestacionESI == 2){
				//recibo de la esi la cantidad de lineas
				//recv(nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->cantidadDeLineas, sizeof(nodo_lista_ejecucion->cantidadDeLineas),0);
				nodo_lista_ejecucion->cantidadDeLineas --;
				//printf("Cantidad de lineas por ejecutar: %d\n", nodo_lista_ejecucion->cantidadDeLineas);
			} else {
				//nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->claveAEjecutar, sizeof(nodo_lista_ejecucion->claveAEjecutar),0);
				//
				//agregar a la estructura
				printf("ESTOY BLOQUEANDO\n");
				laWeaReplanificadoraFIFO(bloqueados,ejecucion);
				break;
			}

		}
		//////////elimino de lista de claves tomadas la ESI y hago un Store avisando que otra clave puede pasarse a ready
		printf("ENTRO ACAAAAAAAA\n");
		claves* clave_temporal = (claves*) malloc(sizeof(claves));
		clave_temporal = list_remove_by_condition(claves_tomadas,(void*)identificador_ESI);


		ESI_STORE(clave_temporal->claveAEjecutar);

		free(clave_temporal);
		free(nodo_lista_ejecucion);
		//free(clave1);//REVISAR SI ESTO SE HACE ACA
		//limpio la lista de ejecucion una vez que termino de ejecutar la ESI
		list_clean(ejecucion);
		printf("Limpio lista\n");
	}
}

void sjfcd(){
	log_info(logger,"Estas en SJFCD");

	unsigned char permisoDeEjecucion = 1;
	unsigned char contestacionESI = 0;
	int sem_value = 0;
	int lista_vacia = 0;
	int resultado_lista = 0;
	int result_connection = 0;
	int resultado_lista_satisfy = 0;
	int resultado_satisfy = 0;
	int result_send = 0;

	while(1) {
		ESI *nodo_lista_ejecucion = NULL;//(ESI*) malloc(sizeof(ESI));

		log_info(logger,"Esperando para planificar");

		//espero a que me digan que hay un nuevo proceso en listos
		sem_wait(&new_process);

		log_info(logger,"Nuevo elemento en la cola de listos o desbloqueo manual de una clave que genero una replanificacion");
		estadoListas();

		//replanifico dependiendo de la rafaga
		if(replanificar == 1){
			log_info(logger,"Replanificando");
			list_sort(listos, (void*)sort_by_estimacion);
			replanificar = 0;
		}

		//Muevo de la lista de listos, el primer nodo a la lista de ejecucion
		laWeaReplanificadoraFIFO(ejecucion,listos);
		log_info(logger,"Nodo de listos movido a Ejecucion");

		resultado_lista = list_is_empty(ejecucion);

		log_info(logger, "Size list ejecucion %d", list_size(ejecucion));

		if(!resultado_lista){
			log_info(logger,"Voy a agarrar la wea de ejecucion");
			nodo_lista_ejecucion =  (ESI*) list_get(ejecucion, 0);
			log_info(logger,"Ya agarre la wea de ejecucion");
			log_info(logger,"ID de la ESI a ejecutar %d", nodo_lista_ejecucion->id_ESI);
			log_info(logger, "Calculo de rafaga: %f", nodo_lista_ejecucion->rafaga);
			id_esi_global = nodo_lista_ejecucion->id_ESI;
		}
		//sem_getvalue(&new_process,&sem_value);
		//Ejecuto la esi seleccionada hasta recibir algun evento que necesite replanificar(nueva esi en listos, de bloqueado a listos, etc).
		while(replanificar == 0){

			//En caso de ejecutar el comando pausar se usa el siguiente semaforo y condicion
			sem_getvalue(&sem_pausar_planificacion,&sem_value);
			if(sem_value<1){
				sem_wait(&sem_pausar_algoritmo);
				break;
			}
			//Si la cantidadDeLineas de la ESI en ejecucion sea mayor a 0
			if(nodo_lista_ejecucion->cantidadDeLineas >0) {

				//Envio al socket de la esi que esta en ejecucion, que puede ejecutarse
				result_send = send(nodo_lista_ejecucion->socket_esi, &permisoDeEjecucion, 1, 0);
				printf("RESULTADO SEND %d\n",result_send);
				//POR QUE EL SEND ME DA 1??????????
				//aca tengo que verificar
				if (result_send <= 0) {
					nodo_lista_ejecucion->cantidadDeLineas = 0;
					laWeaReplanificadoraFIFO(muertos,ejecucion);
					_exit_with_error(nodo_lista_ejecucion->socket_esi, "La ESI en ejecucion murio", NULL);
					break;
				}

				//Espero que la esi me conteste
				result_connection = recv(nodo_lista_ejecucion->socket_esi, &contestacionESI, 1,0);
				printf("RESULTADO RECV %d\n",result_connection);

				//magia con el coord (Recibe si es store/get y realiza acorde)
				coord_communication(nodo_lista_ejecucion->socket_esi,nodo_lista_ejecucion->id_ESI ,contestacionESI);

				if (result_connection <= 0) {
					//strcpy(clave_bloqueada_global,claveAEjecutar);
					id_esi_global = nodo_lista_ejecucion->id_ESI;
					resultado_lista_satisfy = list_any_satisfy(claves_tomadas, (void*)identificador_clave_por_idESI);
					while(resultado_lista_satisfy == 1){
						//////////elimino de lista de claves tomadas la ESI y hago un Store avisando que otra clave puede pasarse a ready
						printf("------RECIBI MENOS DE 0\n");
						claves* clave_temporal = (claves*) malloc(sizeof(claves));
						clave_temporal = list_remove_by_condition(claves_tomadas,identificador_clave_por_idESI);
						ESI_STORE(clave_temporal->claveAEjecutar);
						free(clave_temporal);
						//list_remove_and_destroy_by_condition(claves_tomadas,(void*)identificador_clave_por_idESI,(void*)clave_destroy);
						resultado_lista_satisfy = list_any_satisfy(claves_tomadas, (void*)identificador_clave_por_idESI);
					}

					//hago close del socket

					laWeaReplanificadoraFIFO(muertos,ejecucion);
					_exit_with_error(nodo_lista_ejecucion->socket_esi, "La ESI en ejecucion murio", NULL);
					nodo_lista_ejecucion->cantidadDeLineas = 0;
					break;
				}
					/*
					resultado_satisfy = list_any_satisfy(ejecucion, (void*)identificador_ESI);
						if(resultado_satisfy==1){
							ESI *nodo_lista_ejecucion2 = (ESI*) malloc(sizeof(ESI));
							nodo_lista_ejecucion2 =  (ESI*) list_get(ejecucion, 0);
							nodo_lista_ejecucion2->cantidadDeLineas = 0;
							laWeaReplanificadoraFIFO(muertos,ejecucion);
							_exit_with_error(nodo_lista_ejecucion2->socket_esi, "La ESI en ejecucion murio", NULL);
							free(nodo_lista_ejecucion2);
						}
						else{
							printf("NO ESTA EN EJECUCION\n");
						}
						resultado_satisfy = list_any_satisfy(listos, (void*)identificador_ESI);
						if(resultado_satisfy==1){
							ESI *nodo_lista_ejecucion2 = (ESI*) malloc(sizeof(ESI));
							nodo_lista_ejecucion2 = list_remove_by_condition(listos,identificador_ESI);
							nodo_lista_ejecucion2->cantidadDeLineas = 0;
							laWeaReplanificadoraFIFO(muertos,ejecucion);
							_exit_with_error(nodo_lista_ejecucion2->socket_esi, "La ESI en listos murio", NULL);
							free(nodo_lista_ejecucion2);
						}
						else{
							printf("NO ESTA EN LISTOS\n");
						}
						resultado_satisfy = list_any_satisfy(bloqueados, (void*)identificador_ESI);
						if(resultado_satisfy==1){
							ESI *nodo_lista_ejecucion2 = (ESI*) malloc(sizeof(ESI));
							nodo_lista_ejecucion2 = list_remove_by_condition(bloqueados,identificador_ESI);
							nodo_lista_ejecucion2->cantidadDeLineas = 0;
							laWeaReplanificadoraFIFO(muertos,ejecucion);
							_exit_with_error(nodo_lista_ejecucion2->socket_esi, "La ESI en bloqueados murio", NULL);
							free(nodo_lista_ejecucion2);
						}
						else{
							printf("NO ESTA EN BLOQUEADOS\n");
						}
					}*/

				 else if(contestacionESI == 2) {
					//Si es 2, entonces resto 1 a cada linea faltante y sumo 1 por cada ejecucion de sentencia a las sentencias ejecutadas
					nodo_lista_ejecucion->cantidadDeLineas --;
					nodo_lista_ejecucion->lineas_ejecutadas ++;

				} else {
					//agrego a bloqueados en caso de recibir otra contestacion de la ESI
					log_info(logger,"La ESI %d que se encontraba en ejecucion se pasara a BLOQUEADOS",nodo_lista_ejecucion->id_ESI);
					//Sumo uno a las lineas a ejecutar ya que intento ejecutar una sentencia aunque no pudo y cuenta segun issue foro: #1131
					nodo_lista_ejecucion->lineas_ejecutadas ++;
					log_info(logger, "lineas ejecutadas so far: %d", nodo_lista_ejecucion->lineas_ejecutadas);
					//estimo la rafaga que va a tener ahora que ya ejecuto algunas sentencias
					nodo_lista_ejecucion->rafaga = calculoProxRafaga((float)alpha,nodo_lista_ejecucion->estimacion_rafaga,(float)nodo_lista_ejecucion->lineas_ejecutadas);
					nodo_lista_ejecucion->estimacion_rafaga = nodo_lista_ejecucion->rafaga;
					log_info(logger, "Calculo de rafaga: %f", nodo_lista_ejecucion->rafaga);

					laWeaReplanificadoraFIFO(bloqueados,ejecucion);
					replanificar = 1;
					break;
				}

			} else {
				//Si la cantidad de lineas es menor a 0, muevo la ESI a la cola de terminados
				//printf("Entre a 1\n");
				laWeaReplanificadoraFIFO(terminados,ejecucion);
				estadoListas();
				replanificar = 1;
			}

			//ESI_STORE(claveGlobal);
			//free(clave_temporal);
			//break;
		}
		if(list_size(listos)!=0){
		//si el valor de replanificar cambia, se sale del while y se evalua si la lista de ejecucion no esta vacia
		lista_vacia = list_is_empty(ejecucion);
		printf("lista vacia %d\n",lista_vacia);
		if(lista_vacia != 1){
			//De ser afirmativo
			//estimo la rafaga que va a tener ahora que ya ejecuto algunas sentencias
			nodo_lista_ejecucion->rafaga = calculoProxRafaga((float)alpha,nodo_lista_ejecucion->estimacion_rafaga,(float)nodo_lista_ejecucion->lineas_ejecutadas);
			nodo_lista_ejecucion->estimacion_rafaga = nodo_lista_ejecucion->rafaga;
			log_info(logger, "Calculo de rafaga: %f", nodo_lista_ejecucion->rafaga);

			//Y se mueve la esi que estaba ejecutando a listos para su replanificacion
			laWeaReplanificadoraFIFO(listos,ejecucion);
			replanificar = 1;					//
			sem_post(&new_process);
			//free(nodo_lista_ejecucion);
		}

		}
		else{
			replanificar=0;
		}

}
}

float calculoProxRafaga(float alpha,float estimacion_rafaga, float rafaga_real){
	printf("alpha es: %f\n",alpha);
	printf("alpha es: %d\n",estimacion_inicial);
	printf("estimacion rafaga es %f\n",estimacion_rafaga);
	printf("rafaga real es: %f\n",rafaga_real);
	float resultado = ((alpha/100)*rafaga_real + (1-(alpha/100))*estimacion_rafaga);
	return resultado;
}


bool sort_by_estimacion(void * data1, void * data2){
	ESI * esi1 = (ESI*) data1;
	ESI * esi2 = (ESI*) data2;
	if(esi1->rafaga <= esi2->rafaga) {
			return true;
		}
		return false;
}

bool identificador_ESI(void * data){
	printf("Entre a identificador_ESI\n");
	//claves *esi1= (claves*) data; //recibo estructura de la lista?
	ESI *esi1= (ESI*) data;
	printf("ID de la ESI1: %d\n",esi1->id_ESI);
	printf("ID de la ESI2: %d\n",id_esi_global);
	if(esi1->id_ESI == id_esi_global) {
		return true;
	}
	return false;
}

bool identificador_clave(void * data){
	claves *clave1= (claves*) data; //recibo estructura de la lista?
	log_info(logger, "La clave a ejecutar es '%s'", clave1->claveAEjecutar);
	log_info(logger, "La clave a comparar es '%s'", clave_bloqueada_global);


	if(strcmp(clave1->claveAEjecutar,clave_bloqueada_global)==0) {
		return true;
	}
	return false;
}

bool identificador_clave_por_idESI(void * data){
	claves *clave1= (claves*) data; //recibo estructura de la lista?
	printf("Clave liberada: %s\n",clave1->claveAEjecutar);
	log_info(logger, "El id a ejecutar es '%d'", clave1->id_ESI);
	log_info(logger, "El id a comparar es '%d'", id_esi_global);
	if(clave1->id_ESI == id_esi_global) {
		return true;
	}
	return false;
}

void element_destroyer(void * data){
	free(data);
}

void nodo_lista_claves_destroyer(claves * data){
	free(data->claveAEjecutar);
	free(data);
}

void ESI_GET(char * claveAEjecutar, int id_ESI, unsigned char respuesta_ESI){

	claves* clave1 = (claves*) malloc(sizeof(claves));
	strcpy(clave1->claveAEjecutar,claveAEjecutar);
	clave1->id_ESI = id_ESI;

	if(esi_bloqueada_de_entrada==1){
		strcpy(clave_bloqueada_global,claveAEjecutar);
		printf("CLAVE GLOBAL ES %s\n",clave_bloqueada_global);

		if(dictionary_has_key(claves_bloqueadas,clave_bloqueada_global)){
				printf("Entre en 1\n");
				t_queue * queue_clave = dictionary_get(claves_bloqueadas,clave_bloqueada_global);
				//Si la queue ya existe, se pushea el nuevo id_ESI en la cola de la clave bloqueada
				queue_push(queue_clave, (int*)id_ESI);
				log_info(logger, "Inserte la esi %d en la que de claves bloqueadas, para la clave '%s'", id_ESI, clave_bloqueada_global);
				//dictionary_put(claves_bloqueadas, claveAEjecutar, queue_clave);
		} else {
				//Si no existe la clave, creo la cola asociada, pusheo el id_ESI y agrego la clave con su cola asociada
				printf("Entre en 2\n");
				t_queue * queue_clave = queue_create();
				queue_push(queue_clave, (int*)id_ESI);
				dictionary_put(claves_bloqueadas, claveAEjecutar, queue_clave);
				log_info(logger, "Inserte la esi %d en la que de claves bloqueadas, para la clave '%s'", id_ESI, clave_bloqueada_global);
		}

	}

	else{
		if(respuesta_ESI==2){
			printf("Entre en 3\n");
			list_add(claves_tomadas, (claves*)clave1);
		}
		else if(dictionary_has_key(claves_bloqueadas,claveAEjecutar)){
			printf("Entre en 1\n");
			t_queue * queue_clave = dictionary_get(claves_bloqueadas,claveAEjecutar);
			//Si la queue ya existe, se pushea el nuevo id_ESI en la cola de la clave bloqueada
			queue_push(queue_clave, (int*)id_ESI); //REVISAR si ese (int*) funca bien
		} else {
			//Si no existe la clave, creo la cola asociada, pusheo el id_ESI y agrego la clave con su cola asociada
			printf("Entre en 2\n");
			t_queue * queue_clave = queue_create();
			queue_push(queue_clave, (int*)id_ESI);
			dictionary_put(claves_bloqueadas, claveAEjecutar, queue_clave);
		}
	}
}

void ESI_STORE(char *claveAEjecutar){
	int queue_vacia = 0;
    int id_esi_desbloqueado = 0;
    int key_existente = 1;
    int resultado_lista_satisfy = 0;
    strcpy(clave_bloqueada_global,claveAEjecutar);
    resultado_lista_satisfy = list_any_satisfy(claves_tomadas, (void*)identificador_clave);
    if(resultado_lista_satisfy==1){
    	//////////elimino de lista de claves tomadas la ESI y hago un Store avisando que otra clave puede pasarse a ready
		printf("ENTRO ACAAAAAAAA\n");
		list_remove_and_destroy_by_condition(claves_tomadas,(void*)identificador_clave,(void*)clave_destroy);
		//printf("Clave removida de la lista %s\n",clave_temporal->claveAEjecutar);
		//free(clave_temporal);
		log_info(logger,"Clave eliminada correctamente de la lista de tomados\n");
    }

    //reviso si la clave existe en el diccionario
    if(dictionary_has_key(claves_bloqueadas,claveAEjecutar)){
		t_queue * queue_clave = dictionary_get(claves_bloqueadas,claveAEjecutar);
	    queue_vacia = queue_is_empty(queue_clave);

	    //reviso si la queue no esta vacia
		if(!queue_vacia){
			printf("Entre a queue NO vacia\n");
			//hago un pop de la queue, que sera la proxima esi a salir de bloqueados
			id_esi_desbloqueado = (int)queue_pop(queue_clave);
			log_info(logger,"id ESI desbloqueado de la queue asociada a la key en el diccionario %d\n",id_esi_desbloqueado);
			ESI* esi1 = (ESI*) malloc(sizeof(ESI));
			//asigno la esi a la variable global para utilizar en la funcion para remover de lista por condicion
			id_esi_global = id_esi_desbloqueado;

	/*lo que se hace a continuacion es para ver si la esi que saque de la cola del diccionario esta en la cola de bloqueados. De ser asi, la
	 muevo a listos y replanifico. En caso de no encontrarla, sigue buscando la proxima, ya que dicha ESI no estaria bloqueada,
	 por lo que replanificar no tendria sentido
	 */
			resultado_lista_satisfy= list_any_satisfy(bloqueados, (void*)identificador_ESI);
			while(key_existente!=1){
					if(resultado_lista_satisfy ==1){
						esi1 = list_remove_by_condition(bloqueados, (void*)identificador_ESI);//recorre la lista y remueve bajo condicion
						log_info(logger,"Procesos removido de bloqueados %d",esi1->id_ESI);
						list_add(listos, (ESI*)esi1);
						log_info(logger,"Procesos agregado a la lista de listos %d\n",esi1->id_ESI);
						//seteo los semaforos para el sjfcd
						key_existente=1;
						replanificar = 1;
						sem_post(&new_process);
						}
				//Si esta vacia, la esi no existe en la cola de bloqueados
					else{

						log_error(logger,"No existe la esi en la cola de bloqueados %d",id_esi_global);
						//evaluo la proxima ESI que sigue en la queue
						queue_vacia = queue_is_empty(queue_clave);
						if(!queue_vacia){
							id_esi_desbloqueado = (int)queue_pop(queue_clave);
							log_info(logger,"id ESI desbloqueado de la queue asociada a la key en el diccionario %d",id_esi_desbloqueado);
							id_esi_global = id_esi_desbloqueado;
							resultado_lista_satisfy= list_any_satisfy(bloqueados, (void*)identificador_ESI);
						}
						else{
							break;
						}
					}
			}
	   }
	    //Si la queue esta vacia, entonces la clave asociada no esta tomada (STORE innecesario)
	    else{
				printf("la clave no esta tomada\n");
				if(!list_is_empty(listos)){
					sem_post(&new_process);
					replanificar = 1;
	    		}
	    	}
    	}
	else{
		printf("La clave no existe en el diccionario, nada que desbloquear\n");
	}

}

void desbloquear_del_diccionario(char *claveAEjecutar, int socket){
	int queue_vacia = 0;
	int id_esi_desbloqueado = 0;
	int key_existente = 0;
	int resultado_lista_satisfy = 0;
	unsigned char mensaje_coord = 34;
	int tamanio_clave = 0;
	strcpy(clave_bloqueada_global,claveAEjecutar);
	resultado_lista_satisfy = list_any_satisfy(claves_tomadas, (void*)identificador_clave);
	if(resultado_lista_satisfy==1){
	//////////elimino de lista de claves tomadas la ESI y hago un Store avisando que otra clave puede pasarse a ready
	    printf("ENTRO ACAAAAAAAA\n");
	    list_remove_and_destroy_by_condition(claves_tomadas,(void*)identificador_clave,(void*)clave_destroy);
	    //printf("Clave removida de la lista %s\n",clave_temporal->claveAEjecutar);
	    //free(clave_temporal);
	    log_info(logger,"Clave eliminada correctamente de la lista de tomados\n");
	    }

    //reviso si la clave existe en el diccionario
    if(dictionary_has_key(claves_bloqueadas,claveAEjecutar)){
		t_queue * queue_clave = dictionary_get(claves_bloqueadas,claveAEjecutar);
	    queue_vacia = queue_is_empty(queue_clave);
	//reviso si la queue no esta vacia
	    if(!queue_vacia){
	    	printf("Entre a queue NO vacia\n");
	    	//hago un pop de la queue, que sera la proxima esi a salir de bloqueados
			id_esi_desbloqueado = (int)queue_pop(queue_clave);
			log_info(logger,"id ESI desbloqueado de la queue asociada a la key en el diccionario %d\n",id_esi_desbloqueado);
			ESI* esi1 = (ESI*) malloc(sizeof(ESI));
			//asigno la esi a la variable global para utilizar en la funcion para remover de lista por condicion
			id_esi_global = id_esi_desbloqueado;

	/*lo que se hace a continuacion es para ver si la esi que saque de la cola del diccionario esta en la cola de bloqueados. De ser asi, la
	 muevo a listos y replanifico. En caso de no encontrarla, sigue buscando la proxima, ya que dicha ESI no estaria bloqueada,
	 por lo que replanificar no tendria sentido
	 */
			resultado_lista_satisfy= list_any_satisfy(bloqueados, (void*)identificador_ESI);
			while(key_existente!=1){
					if(resultado_lista_satisfy ==1){
						esi1 = list_remove_by_condition(bloqueados, (void*)identificador_ESI);//recorre la lista y remueve bajo condicion
						log_info(logger,"Procesos removido de bloqueados %d",esi1->id_ESI);
						list_add(listos, (ESI*)esi1);
						log_info(logger,"Procesos agregado a la lista de listos %d\n",esi1->id_ESI);
						//seteo los semaforos para el sjfcd
						key_existente=1;
						replanificar = 1;
						sem_post(&new_process);
						}
				//Si esta vacia, la esi no existe en la cola de bloqueados
					else{

						log_error(logger,"No existe la esi en la cola de bloqueados %d",id_esi_global);
						//evaluo la proxima ESI que sigue en la queue
						queue_vacia = queue_is_empty(queue_clave);
						if(!queue_vacia){
							id_esi_desbloqueado = (int)queue_pop(queue_clave);
							log_info(logger,"id ESI desbloqueado de la queue asociada a la key en el diccionario %d",id_esi_desbloqueado);
							id_esi_global = id_esi_desbloqueado;
							resultado_lista_satisfy= list_any_satisfy(bloqueados, (void*)identificador_ESI);
						}
						else{
							break;
						}
					}

			}
		}
	    else{
	    	log_info(logger,"Enviandole al coordinador un mensaje para liberar la clave");
	    	send(socket,&mensaje_coord,1,0);
			//send(socket,&tamanio_queue,sizeof(tamanio_queue),0);
			tamanio_clave = strlen(claveAEjecutar);
			send(socket,&tamanio_clave,sizeof(tamanio_clave),0);
			send(socket,claveAEjecutar,tamanio_clave,0);
			replanificar = 1;
			sem_post(&new_process);
	    	}
	    }
	else{
		printf("La clave no existe en el diccionario, nada que desbloquear\n");
	}
}
/*BACKUP - NO BORRAR HASTA TESTEAR
void ESI_STORE(char *claveAEjecutar){
	int queue_vacia = 0;
    int list_vacia = 0;
    int id_esi_desbloqueado = 0;
    //reviso si la clave existe en el diccionario
	if(dictionary_has_key(claves_bloqueadas,claveAEjecutar)){
		t_queue * queue_clave = dictionary_get(claves_bloqueadas,claveAEjecutar);
	    queue_vacia = queue_is_empty(queue_clave);
	//reviso si la queue no esta vacia
	    if(queue_vacia!=1){
	    	//hago un pop de la queue, que sera la proxima esi a salir de bloqueados
			id_esi_desbloqueado = queue_pop(queue_clave);
			printf("id ESI bloqueado %d\n",id_esi_desbloqueado);
			ESI* esi1 = (ESI*) malloc(sizeof(ESI));
			//asigno la esi a la variable global para utilizar en la funcion para remover de lista por condicion
			id_esi_global = id_esi_desbloqueado;
			t_list * lista_temporal = list_filter(bloqueados,(void*)identificador_ESI);
			list_vacia = list_is_empty(lista_temporal);
			//chequeo si la lista no esta vacia, remuevo la esi de bloqueados y la muevo a listos
			if(list_vacia != 1){
				esi1 = list_remove_by_condition(bloqueados, (void*)identificador_ESI);//recorre la lista y remueve bajo condicion
				printf("Procesos removido de bloqueados %d\n",esi1->id_ESI);
				list_add(listos, (ESI*)esi1);
				//seteo los semaforos para el sjfcd
				replanificar = 1;
				sem_post(&new_process);
				}
			//Si esta vacia, la esi no existe en la cola de bloqueados
			else{
				dictionary_remove_and_destroy(claves_bloqueadas, claveAEjecutar, (void*)clave_destroy);
				printf("No existe la esi en la cola de bloqueados %d\n",esi1->id_ESI);
				}
	    	}
	    //Si la queue esta vacia, entonces la clave asociada no esta tomada (STORE innecesario)
	    else{
	    	printf("la clave no esta tomada\n");
	    	}
	    }
	else{
		printf("La clave no existe en el diccionario\n");
	}
}*/

void clave_dictionary_destroy(t_dictionary *data){
	free(data);
}

void clave_destroy(claves *self) {
    //free(self->claveAEjecutar);
    free(self);
}

void coord_communication(int socket_ESI, int id_ESI ,unsigned char estado_esi){
	printf("Entre a Coord Comm \n");
	printf("ID ESI RECIBIDO %d\n",id_ESI);
	int size_clave = 0, rc = 0;
	char * clave = NULL;
	unsigned char id_mensaje_coord = 32;
	send(socket_coord, &id_mensaje_coord,1,0);
	//primero hago un recv del coordinador, que me indica que operacion voy a realizar
	rc = recv(socket_coord, &id_mensaje_coord,1, 0);
	printf("Recibi %d bytes del coordinador \n", rc);
	printf("Recibi del coordinador %d \n", id_mensaje_coord);
	if(id_mensaje_coord != 25) {
		recv(socket_coord, &size_clave, 4, 0);
		clave = (char *) malloc (size_clave +1);
		recv(socket_coord, clave, size_clave, 0);
		clave[size_clave] = '\0';
		printf("Recibi la clave %s \n", clave);
	}

	switch (id_mensaje_coord) {
		case 24:
			printf("Entre al get \n");
			ESI_GET(clave, id_ESI, estado_esi);
			break;
		case 26:
			printf("Entre al store \n");
			ESI_STORE(clave);
			break;
		case 25:
			printf("Entre al set \n");
			break;
		default:
			//TODO: Aca hace algo negro
			break;
	}
	//TODO: Fijate si esto es necesario o explota
	free(clave);
}

void get_keys_bloqueadas_de_entrada(int socket){

	t_queue * queue_clave_inicio = queue_create();
	int tamanio_queue = 0;
	int tamanio_clave = 0;
	unsigned char mensaje_coord = 33;
	char* string = (char*) malloc(sizeof(config_get_string_value(config_file, "claves_bloqueadas")));
	//char token[40];
	char *token;
	const char comma[2] = ",";
	//char string[100] = config_get_string_value(config_file, "claves_bloqueadas");
	esi_bloqueada_de_entrada = 1;
	string = config_get_string_value(config_file, "claves_bloqueadas");

	token = strtok(string, comma);
	while( token != NULL ) {
	//printf( " %s\n", token );
	queue_push(queue_clave_inicio,token);
	token = strtok(NULL, comma);
	   }

	tamanio_queue = queue_size(queue_clave_inicio);
	send(socket,&mensaje_coord,1,0);
	send(socket,&tamanio_queue,sizeof(tamanio_queue),0);

	while(!queue_is_empty(queue_clave_inicio)){
		token = queue_pop(queue_clave_inicio);
		tamanio_clave = strlen(token);
		send(socket,&tamanio_clave,sizeof(int),0);
		send(socket,token,tamanio_clave,0);
		ESI_GET(token,-1,0);
	}
	//free(token);
	queue_destroy(queue_clave_inicio);
	esi_bloqueada_de_entrada = 0;
}

/*--------------FIFO------------------------------------
void fifo(){
	//sem_init(&mutex_ejecucion, 0, 1);
	//pthread_mutex_init(&mutex_ejecucion,NULL);
	//int lista_vacia = list_is_empty(ejecucion);

	unsigned char permisoDeEjecucion = 1;
	unsigned char contestacionESI = 0;


	while(1){
		ESI *nodo_lista_ejecucion = NULL;
		int sem_value = 0;
		printf("Esperando que haya un nuevo proceso encolado en listos\n");
		estadoListas();

		//espero a que me digan que hay algo en la cola de listos
		sem_wait(&new_process);
		printf("Nuevo elemento en la cola de listos\n");
		estadoListas();

		//Muevo de la lista de listos, el primer nodo a la lista de ejecucion
		laWeaReplanificadoraFIFO(ejecucion,listos);
		printf("Nodo de listos movido a Ejecucion\n ");
		nodo_lista_ejecucion =  (ESI*) list_get(ejecucion, 0);

		//Mientras la cantidadDeLineas de la ESI en ejecucion sea mayor a 0
		while(nodo_lista_ejecucion->cantidadDeLineas >0){
			sem_getvalue(&sem_pausar_planificacion,&sem_value);
			if(sem_value<1){
			sem_wait(&sem_pausar_algoritmo);
			}

		//Envio al socket de la esi que esta en ejecucion, que puede ejecutarse
			send(nodo_lista_ejecucion->socket_esi, &permisoDeEjecucion, 1, 0);
		//Espero que la esi me conteste
			recv(nodo_lista_ejecucion->socket_esi, &contestacionESI, 1,0);
			if (result_connection <= 0) {
						_exit_with_error(nodo_lista_ejecucion->socket_esi, "La ESI en ejecucion murio", NULL);
						nodo_lista_ejecucion->cantidadDeLineas = 0;
						}
		//Si es 1, entonces espero que me envie la nueva cantidad de Lineas que tiene
			if(contestacionESI == 2){
				//recibo de la esi la cantidad de lineas
				recv(nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->cantidadDeLineas, sizeof(nodo_lista_ejecucion->cantidadDeLineas),0);
				printf("Cantidad de lineas por ejecutar: %d\n", nodo_lista_ejecucion->cantidadDeLineas);
			}
			else{
				//nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->claveAEjecutar, sizeof(nodo_lista_ejecucion->claveAEjecutar),0);
				//
				//agregar a la estructura
				printf("ESTOY BLOQUEANDO\n");
				laWeaReplanificadoraFIFO(bloqueados,ejecucion);
			}
			//magia con el coord (Recibe si es store/get y realiza acorde)
			coord_communication(nodo_lista_ejecucion->socket_esi,nodo_lista_ejecucion->id_ESI ,contestacionESI);

			}
		free(nodo_lista_ejecucion);
		//free(clave1);//REVISAR SI ESTO SE HACE ACA
		//limpio la lista de ejecucion una vez que termino de ejecutar la ESI
		list_clean(ejecucion);
		printf("Limpio lista\n");
		}
	}
*/

/*SJFCD PELADO SIN KEYS
void sjfcd(){
	//sem_init(&mutex_ejecucion, 0, 1);
	//pthread_mutex_init(&mutex_ejecucion,NULL);
	//int lista_vacia = list_is_empty(ejecucion);

	unsigned char permisoDeEjecucion = 1;
	unsigned char contestacionESI = 0;
	int sem_value = 0;
	int lista_vacia = 0;
	int result_connection = 0;

	while(1){
		ESI *nodo_lista_ejecucion = NULL;//(ESI*) malloc(sizeof(ESI));
		printf("Estas en SJFCD\n");
		printf("Esperando que haya un nuevo proceso encolado en listos\n");

		//espero a que me digan que hay un nuevo proceso en listos
		sem_wait(&new_process);

		printf("Nuevo elemento en la cola de listos\n");
		estadoListas();

		//replanifico dependiendo de la rafaga
		if(replanificar == 1){
		printf("Replanificando\n");
		list_sort(listos, (void*)sort_by_estimacion);
		replanificar = 0;
		}

		//Muevo de la lista de listos, el primer nodo a la lista de ejecucion
		laWeaReplanificadoraFIFO(ejecucion,listos);
		printf("Nodo de listos movido a Ejecucion\n ");
		nodo_lista_ejecucion =  (ESI*) list_get(ejecucion, 0);
		printf("ID de la ESI a ejecutar %d\n", nodo_lista_ejecucion->id_ESI);

		//sem_getvalue(&new_process,&sem_value);
		//Ejecuto la esi seleccionada hasta recibir algun evento que necesite replanificar(nueva esi en listos, de bloqueado a listos, etc).
		while(replanificar == 0){
			sem_getvalue(&sem_pausar_planificacion,&sem_value);
			if(sem_value<1){
				sem_wait(&sem_pausar_algoritmo);
			}
		//Si la cantidadDeLineas de la ESI en ejecucion sea mayor a 0
		if(nodo_lista_ejecucion->cantidadDeLineas >0){

		//Envio al socket de la esi que esta en ejecucion, que puede ejecutarse
				send(nodo_lista_ejecucion->socket_esi, &permisoDeEjecucion, 1, 0);
		//Espero que la esi me conteste
				result_connection = recv(nodo_lista_ejecucion->socket_esi, &contestacionESI, 1,0);
				if (result_connection <= 0) {
				    _exit_with_error(nodo_lista_ejecucion->socket_esi, "La ESI en ejecucion murio", NULL);
				    nodo_lista_ejecucion->cantidadDeLineas = 0;
				  }
				//printf("contestacionESI %d\n",contestacionESI);
		//Si es 2, entonces resto 1 a cada linea faltante y resto 1 por cada ejecucion de sentencia a la rafaga
				else if(contestacionESI == 2){
					//recibo de la esi la cantidad de lineas
					//recv(nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->cantidadDeLineas, sizeof(nodo_lista_ejecucion->cantidadDeLineas),0);
					nodo_lista_ejecucion->cantidadDeLineas --;
					printf("Cantidad de lineas por ejecutar: %d\n", nodo_lista_ejecucion->cantidadDeLineas);
					nodo_lista_ejecucion->rafaga --;
				}
				else{
					//nodo_lista_ejecucion->socket_esi, &nodo_lista_ejecucion->claveAEjecutar, sizeof(nodo_lista_ejecucion->claveAEjecutar),0);
					//agregar a la estructura
					//agrego a bloqueados en caso de recibir otra contestacion de la ESI
					printf("ESTOY BLOQUEANDO\n");
					laWeaReplanificadoraFIFO(bloqueados,ejecucion);
					break;
				}

				//magia con el coord (Recibe si es store/get y realiza acorde)
				coord_communication(nodo_lista_ejecucion->socket_esi,nodo_lista_ejecucion->id_ESI ,contestacionESI);
		}
		//Si la cantidad de lineas es menor a 0, muevo la ESI a la cola de terminados
		else{
			//printf("Entre a 1\n");
			laWeaReplanificadoraFIFO(terminados,ejecucion);
			estadoListas();
			break;					//
		}
	}
		//si el valor de replanificar cambia, se sale del while y se evalua si la lista de ejecucion no esta vacia
		lista_vacia = list_is_empty(ejecucion);
		printf("lista vacia %d\n",lista_vacia);
		if(lista_vacia != 1){

		//De ser afirmativo, se mueve la esi que estaba ejecutando a listos para su replanificacion
			//printf("Entre a 3\n");
			laWeaReplanificadoraFIFO(listos,ejecucion);
			replanificar = 1;					//
			sem_post(&new_process);
			//free(nodo_lista_ejecucion);
		}
}
}
*/
