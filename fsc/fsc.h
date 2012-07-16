/*
 * fsc.h
 *
 *  Created on: 25/05/2012
 *      Author: utnso
 */

#ifndef FSC_H_
#define FSC_H_

#include "serializar_y_deserializar.h"
#include "../commons/sockets.h"

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

#define TAMANIOPATH 40

#define OCUPADO 1;
#define DESOCUPADO 0;

#define CACHEGETATR 'a'
#define CACHEREADDIR 'd'

//typedef struct {
//	t_socket_client* socket;
//	uint8_t estado;
//}pool_sockets;

#endif /* FSC_H_ */
