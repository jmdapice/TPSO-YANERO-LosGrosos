/*
 * sincro.c
 *
 *  Created on: 26/06/2012
 *      Author: utnso
 */

#include "sincro.h"

void Sincro_agregarALista(uint32_t nroInodo, t_list* listaInodos){

	//Esta función se ejecuta por cada OPEN

	t_nodoInodo* nodo;

	nodo = (t_nodoInodo*) malloc(sizeof(t_nodoInodo));
	nodo->cantHilos = 0;
	pthread_rwlock_init(&nodo->mt_inodo,0);
	nodo->nroInodo = nroInodo;
	list_add(listaInodos,nodo);

}

void Sincro_crearMonitor(t_list* listaInodos){

	listaInodos = list_create();

}

void Sincro_monitorLock(uint32_t nroInodo, t_list* listaInodos, uint8_t operacion){

	//Operaciones: 5 = Lectura (TIPOREAD)
    //             6 = Escritura (TIPOWRITE)

	int i;
	uint8_t encontro = 0;

	t_nodoInodo* nodo;


	pthread_mutex_lock(&mt_lista_inodos);

		if(listaInodos->elements_count > 0){

			for(i=0;i<listaInodos->elements_count && encontro == 0; i++){

				nodo = (t_nodoInodo*)list_get(listaInodos,i);
				if(nodo->nroInodo==nroInodo) encontro = 1;

			}

		}

		if(encontro==1) {
			nodo->cantHilos++;
		}else{
			nodo = (t_nodoInodo*) malloc(sizeof(t_nodoInodo));
			nodo->cantHilos = 1;
			pthread_rwlock_init(&nodo->mt_inodo,0);
			nodo->nroInodo = nroInodo;
			list_add(listaInodos,nodo);
		};

	pthread_mutex_unlock(&mt_lista_inodos);


	if(operacion == TIPOREAD)
		pthread_rwlock_rdlock(&nodo->mt_inodo);
	else if(operacion == TIPOWRITE)
		pthread_rwlock_wrlock(&nodo->mt_inodo);

}

void Sincro_monitorUnlock(uint32_t nroInodo, t_list* listaInodos){

	int i;
	uint8_t salir = 0;

	t_nodoInodo* nodo;


	pthread_mutex_lock(&mt_lista_inodos);

		if(listaInodos->elements_count > 0){

			for(i=0;i<listaInodos->elements_count && salir == 0; i++){

				nodo = (t_nodoInodo*)list_get(listaInodos,i);
				if(nodo->nroInodo==nroInodo){
					salir = 1;
					nodo->cantHilos--;
					if(nodo->cantHilos == 0){
						list_remove(listaInodos,i);
						free(nodo);
					}else{
						pthread_rwlock_unlock(&nodo->mt_inodo);
					}
				}
			}

		}
	pthread_mutex_unlock(&mt_lista_inodos);
}

