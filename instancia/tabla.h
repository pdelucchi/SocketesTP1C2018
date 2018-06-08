#ifndef TABLA_H_
#define TABLA_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/node.h>
#include "Dato.h"



void* buscar(t_list *tabla,char* claveBuscada);

void* liberar_recursos(t_list** tabla);
/*
 * get_key busca por clave en la memoria y devuelve la posicion de memoria en donde
 * se encuentra dicho valor de la clave
 * */

void registrar_dato_en_tabla(t_list** tabla,struct Dato* unDato);

int actualizarTabla(t_list** tabla, int bytesDeDatosASobreescribir);


#endif /* TABLA_H_ */
