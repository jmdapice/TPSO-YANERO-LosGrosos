/*
 * Dir.h
 *
 *  Created on: 21/05/2012
 *      Author: utnso
 */

#ifndef DIR_H_
#define DIR_H_

#define OFF_NAME 8
#define INODO_RAIZ 2

#include <stdint.h>
#include "Grupos.h"
#include "../commons/collections/list.h"
#include "Inode.h"
#include "cacheInterface.h"
#include <libmemcached/memcached.h>
#include <pthread.h>

typedef struct {
	uint32_t d_inode; //Inodo donde se encuentra el archivo
	uint16_t d_rec_len; //Longitud del registro
	uint8_t  d_name_len; //Longitud del nombre del archivo
	uint8_t  d_filetype; //No lo usamos
	char     *d_name; //Nombre del archivo
} Dir;


extern __useconds_t delay;
extern memcached_st* st_cache;
extern pthread_mutex_t mt_cache;

t_list *Dir_listar(FILE *,uint32_t, const uint32_t, const Group_descriptor *, const size_t);
void Dir_destroy(Dir *);
uint32_t Dir_buscarPath(FILE *, char *, const uint32_t , const Group_descriptor *, const size_t );
void *Dir_leerArchivo(FILE *,Inode *, off_t , size_t , size_t);
int32_t Dir_escribirArchivo(FILE *,Inode *,off_t ,size_t ,void *,Superblock *,Group_descriptor *, uint32_t);
int8_t Dir_crearNuevaEntrada(FILE *,char *,uint32_t , uint32_t,Superblock *,Group_descriptor *);
int8_t Dir_borrarEntrada(FILE *ext2fs, uint32_t nroInodoDir, uint32_t nroInodoArch, Superblock *sb,Group_descriptor *bgdt);
int32_t Dir_cantDirectorios(Inode* inodo, 	FILE *ext2fs, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const size_t block_size);


#endif /* DIR_H_ */
