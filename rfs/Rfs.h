/*
 * Rfs.h
 *
 *  Created on: 14/06/2012
 *      Author: utnso
 */

#ifndef RFS_H_
#define RFS_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "Superblock.h"
#include "Grupos.h"
#include "../commons/bitarray.h"
#include "../commons/sockets.h"
#include "../commons/log.h"
#include "../commons/config.h"
#include "../commons/collections/queue.h"
#include "serial_rfs.h"
#include "Inode.h"
#include "Dir.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/inotify.h>
#include "sincro.h"
#include "cacheInterface.h"

typedef struct {
	t_log *log;
	t_socket_client *client;
	void *buffer;
	char tipoPaquete;
	uint16_t payloadLength;
	Superblock *sb;
	Group_descriptor *bgdt;
	FILE *ext2fs;
} Pedido;

typedef struct {
	char *pathDir;
	char *nombreArch;
} PathSeparado;


PathSeparado separarPath(char *path);
FILE *abrirArchivoExt2(char *nombre);
void crearThreads(uint8_t maxThreads,char *);
void levantarCfg(char **ip, int32_t *port, t_log_level *log_level, uint8_t *maxThreads, char **nombreArchivo, char** ipCache, int32_t *portCache);
void actualizarDelay(int);
void eliminarFd(t_list* lista, int32_t fd);
t_socket_client *buscarFd(t_list* lista,int32_t fd);
void procesarPedido(void *);
Pedido *crearPedido(t_log *s_log,t_socket_client *client,void *buffer,char tipoPaquete, uint16_t payloadLength, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarGetattr(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarReaddir(t_log *s_log,t_socket_client *client,void *buffer,Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarRead(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength);
void procesarOpen(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarTruncate(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength);
void procesarWrite(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength);
void procesarCreate(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarUnlink(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarMkdir(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarRmdir(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs);
void procesarError(t_socket_client *client);

#endif /* RFS_H_ */
