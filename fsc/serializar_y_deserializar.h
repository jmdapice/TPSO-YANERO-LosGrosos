/*
 * serializar_y_deserializar.h
 *
 *  Created on: 27/05/2012
 *      Author: utnso
 */

#ifndef SERIALIZAR_Y_DESERIALIZAR_H_
#define SERIALIZAR_Y_DESERIALIZAR_H_

#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "../commons/collections/list.h"

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
       uint32_t d_inode; //Inodo donde se encuentra el archivo
       uint16_t d_rec_len; //Longitud del registro
       uint8_t  d_name_len; //Longitud del nombre del archivo
       uint8_t  d_filetype; //No lo usamos
       char     *d_name; //Nombre del archivo
} Dir;


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

char* serializar_path(char*, uint8_t); //segundo argumento es el TIPO_OPERACIÓN

char* serializar_Read_Pedido(char*, off_t, size_t);

char* serializar_Write_Pedido(char*, size_t, char*, off_t);

char* serializar_Truncate_Pedido(char*, off_t);



DesAttr_resp deserializar_Gettattr_Rta(void*);

DesReadDir_resp deserializar_Readdir_Rta(uint16_t, void*);

char* deserializar_Read_Rta(uint16_t, void*);

int deserializar_Write_Rta(void*);

int deserializar_Result_Rta(void*);

//------------------------------


#endif /* SERIALIZAR_Y_DESERIALIZAR_H_ */
