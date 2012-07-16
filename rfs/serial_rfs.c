/*
 * serial_rfc.c
 *
 *  Created on: 07/06/2012
 *      Author: utnso
 */

#ifndef SERIAL_RFC_C_
#define SERIAL_RFC_C_

#include "serial_rfs.h"

char *serializar_error(void){

	char *paquete = malloc(sizeof(uint8_t)+sizeof(uint16_t));
	uint8_t tipo_error = 0;
	uint16_t tam_error = 0;

	memcpy(paquete,&tipo_error,sizeof(uint8_t));
	memcpy(paquete+sizeof(uint8_t),&tam_error,sizeof(uint16_t));

	return paquete;

}

char *serializar_Nipc(Nipc st_nipc) {

	int offset = 0;
	char *paquete_send;

	paquete_send = malloc(sizeof(uint8_t)+sizeof(uint16_t)+st_nipc.payloadLength);
	memcpy(paquete_send,&st_nipc.tipo,sizeof(uint8_t));
	offset = sizeof(uint8_t);
	memcpy(paquete_send+offset,&st_nipc.payloadLength,sizeof(uint16_t));
	offset = offset+sizeof(uint16_t);
	memcpy(paquete_send+offset,st_nipc.payload, st_nipc.payloadLength);

	return paquete_send;

}

char* serializar_path(char* path, uint8_t tipo){

	char* paquete;
	Nipc st_nipc;


	st_nipc.tipo = tipo;
	st_nipc.payloadLength = strlen(path) + 1;
	st_nipc.payload = malloc(st_nipc.payloadLength);

	memcpy(st_nipc.payload, path, strlen(path) + 1);

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;

}

//------

Desread deserializar_Read_Pedido(uint16_t payloadLenght, void* payload){

	Desread deserial;
	uint16_t tam_path;
	char* p;
	uint16_t off;

	p = payload;

	for(tam_path = 1; *p != '\0'; tam_path++) p++;

	deserial.path = (char*)malloc(tam_path + 1);

	off = tam_path;
	memcpy(deserial.path,payload,off);
	memcpy(&deserial.offset,payload+off,sizeof(off_t));
	off = off + sizeof(off_t);
	memcpy(&deserial.size,payload+off,sizeof(size_t));

	return deserial;
};

Deswrite deserializar_Write_Pedido(uint16_t payloadLenght, void* payload){

	Deswrite deserial;
	uint16_t tam_path;
	char* p;
	uint16_t off;

	p = payload;

	for(tam_path = 1; *p != '\0'; tam_path++) p++;

	deserial.path = (char*)malloc(tam_path + 1);

	off = tam_path;
	memcpy(deserial.path,payload,off);
	memcpy(&deserial.size,payload+off,sizeof(size_t));
	off = off + sizeof(size_t);

	deserial.buf = malloc(deserial.size);

	memcpy(deserial.buf,payload+off,deserial.size);
	off = off + deserial.size;
	memcpy(&deserial.offset,payload+off,sizeof(uint32_t));

	return deserial;
};

Destrunc deserializar_Truncate_Pedido(uint16_t payloadLenght, void* payload){

	Destrunc deserial;
	uint16_t tam_path;
	char* p;
	uint16_t off;

	p = payload;

	for(tam_path = 1; *p != '\0'; tam_path++) p++;

	deserial.path = (char*)malloc(tam_path + 1);

	off = tam_path;
	memcpy(deserial.path,payload,off);
	memcpy(&deserial.offset,payload+off,sizeof(off_t));

	return deserial;
};

//--------------------

char* serializar_Gettattr_Rta(mode_t modo, nlink_t links, off_t total_size){

	char* paquete;
	Nipc st_nipc;
	uint16_t offset = 0;

	st_nipc.tipo = TIPOGETATTR;
	st_nipc.payloadLength = sizeof(mode_t) + sizeof(nlink_t) + sizeof(off_t);
	st_nipc.payload = malloc(st_nipc.payloadLength);
	memcpy(st_nipc.payload,&modo,sizeof(mode_t));
	offset = sizeof(mode_t);
	memcpy(st_nipc.payload+offset,&links,sizeof(nlink_t));
	offset = offset + sizeof(nlink_t);
	memcpy(st_nipc.payload+offset,&total_size,sizeof(off_t));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
};

char* serializar_Read_Rta(uint16_t size, char* buf){

	char* paquete;
	Nipc st_nipc;

	st_nipc.tipo = TIPOREAD;
	st_nipc.payloadLength = size;
	st_nipc.payload = malloc(size);

	memcpy(st_nipc.payload,buf,size);

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
}

char* serializar_Write_Rta(int bytes_escritos){

	char* paquete;
	Nipc st_nipc;

	st_nipc.tipo = TIPOWRITE;
	st_nipc.payloadLength = sizeof(int);
	st_nipc.payload = malloc(st_nipc.payloadLength);

	memcpy(st_nipc.payload,&bytes_escritos,sizeof(int));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
}

char* serializar_Result_Rta(int rta, uint8_t tipo){

	char* paquete;
	Nipc st_nipc;

	st_nipc.tipo = tipo;
	st_nipc.payloadLength = sizeof(int);
	st_nipc.payload = malloc(st_nipc.payloadLength);

	memcpy(st_nipc.payload,&rta,sizeof(int));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;

}

SerReadDir_resp serializar_readdir_Rta(t_list* lista){

	Nipc st_nipc;
	uint16_t offset = 0;
	uint8_t size = 0;
	int32_t tope = lista->elements_count;
	int32_t nodo;
	Dir *dir;
	SerReadDir_resp respuesta;
	st_nipc.payloadLength =0;
	st_nipc.tipo = TIPOREADDIR;
	for(nodo = 0; nodo < tope; nodo++) {

		dir = (Dir*)list_get(lista,nodo);
		st_nipc.payloadLength = st_nipc.payloadLength + strlen(dir->d_name) + 1 + sizeof(uint8_t);
	}

	respuesta.tamano = st_nipc.payloadLength;
	st_nipc.payload = malloc(st_nipc.payloadLength);

	for(nodo = 0; nodo < tope; nodo++) {

		dir = (Dir*)list_get(lista,nodo);
		size = strlen(dir->d_name) + 1;
		memcpy(st_nipc.payload+offset,&size,sizeof(uint8_t));
		offset = offset + sizeof(uint8_t);
		memcpy(st_nipc.payload+offset,dir->d_name,size);
		offset = offset + size;
	}


	respuesta.paquete = serializar_Nipc(st_nipc);
	free(st_nipc.payload);
	return respuesta;

}

#endif /* SERIAL_RFC_C_ */
