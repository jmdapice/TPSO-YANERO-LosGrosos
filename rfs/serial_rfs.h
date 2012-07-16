/*
 * serial_rfc.h
 *
 *  Created on: 07/06/2012
 *      Author: utnso
 */

#ifndef SERIAL_RFC_H_
#define SERIAL_RFC_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "../commons/collections/list.h"
#include "Dir.h"

#define TIPOGETATTR 1
#define TIPOREADDIR 2
#define TIPOOPEN 3
#define TIPORELEASE 4
#define TIPOREAD 5
#define TIPOWRITE 6
#define TIPOCREATE 7
#define TIPOUNLINK 8
#define TIPOTRUNCATE 9
#define TIPORMDIR 10
#define TIPOMKDIR 11

//Tipos de datos a utilizar:

typedef struct {
	uint8_t tipo;
	uint16_t payloadLength;
	void *payload;
} Nipc;

typedef struct {
	char *path;
	off_t offset;
	size_t size;
} Desread;

typedef struct {
	char *path;
	size_t size;
	char* buf;
	off_t offset;
} Deswrite;

typedef struct {
	char *path;
	off_t offset;
} Destrunc;

typedef struct {
	uint16_t tamano;
	char* lista_nombres;
}DesReadDir_resp;

typedef struct {
	mode_t modo;
	nlink_t nlink;
	off_t total_size;
}DesAttr_resp;

typedef struct {
	uint16_t tamano;
	void* paquete;
}SerReadDir_resp;

//------------------------------

//Funciones:

char *serializar_Nipc(Nipc);

char *serializar_error(void);
//------

Desread deserializar_Read_Pedido(uint16_t, void*);

Deswrite deserializar_Write_Pedido(uint16_t , void*);

Destrunc deserializar_Truncate_Pedido(uint16_t , void*);

//------

char* serializar_Gettattr_Rta(mode_t, nlink_t, off_t);

char* serializar_Read_Rta(uint16_t, char*);

char* serializar_Write_Rta(int);

char* serializar_Result_Rta(int, uint8_t);

SerReadDir_resp serializar_readdir_Rta(t_list* lista);

//------

#endif /* SERIAL_RFC_H_ */
