/*
 * serializar_y_deserializar.c
 *
 *  Created on: 27/05/2012
 *      Author: Boaglio Pablo
 */


#include "serializar_y_deserializar.h"

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

char* serializar_Read_Pedido(char* path, off_t off, size_t size){

	char* paquete;
	Nipc st_nipc;
	uint16_t offset = 0;
	uint32_t offRead = (uint32_t) off;

	st_nipc.tipo = TIPOREAD;
	st_nipc.payloadLength = strlen(path) + 1 + sizeof(uint32_t) + sizeof(size_t);
	st_nipc.payload = malloc(st_nipc.payloadLength);

	memcpy(st_nipc.payload,path,strlen(path)+1);
	offset = strlen(path) + 1;
	memcpy(st_nipc.payload + offset, &offRead, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(st_nipc.payload + offset, &size, sizeof(size_t));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
}

char* serializar_Write_Pedido(char* path, size_t size, char* buf, off_t off){

	char* paquete;
	Nipc st_nipc;
	uint16_t offset = 0;
	uint32_t offWrite = (uint32_t) off;

	st_nipc.tipo = TIPOWRITE;
	st_nipc.payloadLength = strlen(path) + 1 + sizeof(uint32_t) + sizeof(size_t) + size;
	st_nipc.payload = malloc(st_nipc.payloadLength);

	memcpy(st_nipc.payload,path,strlen(path)+1);
	offset = strlen(path) + 1;
	memcpy(st_nipc.payload + offset, &size, sizeof(size_t));
	offset = offset + sizeof(size_t);
	memcpy(st_nipc.payload + offset, buf, size);
	offset = offset + size;
	memcpy(st_nipc.payload + offset, &offWrite, sizeof(uint32_t));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
}

char* serializar_Truncate_Pedido(char* path, off_t off){

	char* paquete;
	Nipc st_nipc;
	uint16_t offset = 0;

	st_nipc.tipo = TIPOTRUNCATE;
	st_nipc.payloadLength = strlen(path) + 1 + sizeof(off_t);
	st_nipc.payload = malloc(st_nipc.payloadLength);

	offset = strlen(path) + 1;
	memcpy(st_nipc.payload,path,offset);
	memcpy(st_nipc.payload + offset, &off, sizeof(off_t));

	paquete = serializar_Nipc(st_nipc);

	free(st_nipc.payload);

	return paquete;
}
//--------------------

DesReadDir_resp deserializar_Readdir_Rta(uint16_t payloadLength, void* paquete) {

	DesReadDir_resp respuesta;

	respuesta.tamano = payloadLength;
	respuesta.lista_nombres = malloc(respuesta.tamano);
	memcpy(respuesta.lista_nombres,paquete,respuesta.tamano);

	return respuesta;

};

char* deserializar_Read_Rta(uint16_t lectura, void* paquete){

	char* buf;
//	size_t size_buf;
//	uint16_t off;
//
//	off = 0;
//	memcpy(&size_buf, paquete, sizeof(size_t) );
//
//	off = sizeof(size_t);

	buf = malloc(lectura);

	memcpy(buf,paquete,lectura);

	return buf;
};

int deserializar_Write_Rta(void* paquete){

	int cant_bytes;

	memcpy(&cant_bytes,paquete,sizeof(int));

	return cant_bytes;
};

int deserializar_Result_Rta(void* paquete){

	int rta;

	memcpy(&rta,paquete,sizeof(int));

	return rta;
};

DesAttr_resp deserializar_Gettattr_Rta(void* paquete){

	DesAttr_resp attr;
	uint16_t off = 0;
	mode_t modo;
	nlink_t nlink;
	uint32_t total_size;

	memcpy(&modo,paquete,sizeof(mode_t));
	off = sizeof(mode_t);
	memcpy(&nlink,paquete+off,sizeof(nlink_t));
	off = off + sizeof(nlink_t);
	memcpy(&total_size, paquete+off,sizeof(uint32_t));


	attr.modo = modo;
	attr.nlink = nlink;
	attr.total_size = (off_t)total_size;

	return attr;
};
