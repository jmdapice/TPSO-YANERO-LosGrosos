/*
 * Dir.c
 *
 *  Created on: 21/05/2012
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Inode.h"
#include "Grupos.h"
#include "Dir.h"
#include "../commons/collections/list.h"
#include "../commons/bitarray.h"

uint32_t buscarNombreEnLista(t_list *,char *);
char* serializarDir(Dir* , uint32_t);

t_list *Dir_listar(FILE *ext2fs, uint32_t nroInodo, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const size_t block_size){

	uint32_t bloqueDatos,cantBloques,offset,i;
	Dir *dirRaiz;
	Inode *inodo;
	t_list *lista;

	lista = list_create();
	inodo = Inode_leerInodo(ext2fs,inodos_por_grupo,bgdt,nroInodo,block_size);
	cantBloques = (inodo->i_size)/block_size;
	if((inodo->i_size)%block_size >0) cantBloques++;

	for(i=0;i<cantBloques;i++) {
		bloqueDatos = Inode_leerBloque(ext2fs,i,inodo,block_size);
		offset=0;

		while(offset<block_size) {
			dirRaiz = (Dir *)malloc(sizeof(Dir));
			//leo los primeros 8 bytes
			fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
			fread_unlocked(dirRaiz,OFF_NAME,1,ext2fs);
			//ahora hago un malloc para el nombre
			dirRaiz->d_name =(char *)malloc(dirRaiz->d_name_len+1);
			//y leo el nombre
			fread_unlocked(dirRaiz->d_name,dirRaiz->d_name_len,1,ext2fs);
			dirRaiz->d_name[dirRaiz->d_name_len] = '\0';
			//agrego la struct dirRaiz a una lista
			//antes de agregar a la lista hay q ver si el inodo es = 0
			//si es 0 quiere decir q se borro
			if(dirRaiz->d_inode !=0) {
				list_add(lista,dirRaiz);
			}
			offset+=dirRaiz->d_rec_len;
		}
	}
	free(inodo);
	return lista;
}

//Esta funcion devuelve el numero de inodo de un path que apunte a otro directorio o a un archivo
uint32_t Dir_buscarPath(FILE *ext2fs, char *path, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const size_t block_size) {

	char *aux;
	char *archivo;
	uint32_t nroInodo=INODO_RAIZ;
	t_list *contenidoDir;

	for(archivo=strtok_r(path,"/",&aux);archivo&&nroInodo!=0;archivo=strtok_r(NULL,"/",&aux)) {
		contenidoDir = Dir_listar(ext2fs,nroInodo,inodos_por_grupo,bgdt,block_size);
		nroInodo = buscarNombreEnLista(contenidoDir,archivo);
		list_destroy_and_destroy_elements(contenidoDir,(void *) Dir_destroy);
	}


	return nroInodo;
}

uint32_t buscarNombreEnLista(t_list *lista,char *archivo) {

	uint32_t i,nroInodo;
	uint8_t encontro=1;
	Dir *elemDir;

	for(i=0;encontro!=0 && i<lista->elements_count;i++) {
			elemDir = (Dir *)list_get(lista,i);
			encontro = strcmp(elemDir->d_name,archivo);
		}

	if(encontro!=0) {
		nroInodo=0; //Si el nombre no fue encontrado entonces el archivo no exite, retorno 0 (no existe el inodo 0)
	}else {         //Sino retorno el numero de inodo de esa entrada
		nroInodo = elemDir->d_inode;
	}
	return nroInodo;
}

void Dir_destroy(Dir *self) {
	free(self->d_name);
	free(self);
}

void *Dir_leerArchivo(FILE *ext2fs,Inode *inodo, off_t offset, size_t size, size_t block_size) {

	memcached_return resultado;
	size_t tamPaqCache;
	void *buffTemp = malloc(size+block_size*2);
	void *buffer = malloc(size);
	void *lectCache = malloc(block_size);
	uint32_t nroBloqueComienzo = 0,cantBloques = 0,offsetEnBufferComienzo = 0,bloque = 0,i=0;
	nroBloqueComienzo = offset / block_size;
	offsetEnBufferComienzo = offset % block_size;
	cantBloques = size / block_size;
	if(size%block_size >0) cantBloques++;

	while(cantBloques>0) {
		bloque = Inode_leerBloque(ext2fs,nroBloqueComienzo,inodo,block_size);
		pthread_mutex_lock(&mt_cache);
			lectCache = cache_bajar_infoServer(st_cache, bloque, &tamPaqCache, &resultado);
		pthread_mutex_unlock(&mt_cache);
		if (resultado != MEMCACHED_SUCCESS){
			usleep(delay);
			fseek(ext2fs,bloque*block_size,SEEK_SET);
			fread_unlocked(buffTemp+block_size*i,block_size,1,ext2fs); //leo un bloque entero
			pthread_mutex_lock(&mt_cache);
				resultado = cache_subir_infoServer(st_cache,bloque,buffTemp+block_size*i,block_size);
			pthread_mutex_unlock(&mt_cache);
		}else{
			memcpy(buffTemp+block_size*i,lectCache,block_size);
		}

		nroBloqueComienzo++;
		cantBloques--;
		i++;
	}
	//ahora en buffTemp tengo los bloques del archivo que contienen la info a leer
	//saco los bytes pedidos
	memcpy(buffer,buffTemp+offsetEnBufferComienzo,size);
	free(buffTemp);
	free(lectCache);

	return buffer;
}

int32_t Dir_escribirArchivo(FILE *ext2fs,Inode *inodo,off_t offset,size_t size,void *buf,Superblock *sb,Group_descriptor *bgdt, uint32_t nroInodo) {

	memcached_return retorno;
	int8_t error;
	int32_t cantEscrita=0;
	size_t block_size = 1024 << sb->s_log_block_size;
	uint32_t nroBloqueComienzo=0,offsetEnBloqueComienzo=0,bytesAEscribirEnBloqueComienzo=0;
	uint32_t bloque;

	void* escCache = malloc(block_size);

	nroBloqueComienzo = offset / block_size;
	offsetEnBloqueComienzo = offset % block_size;

	bytesAEscribirEnBloqueComienzo = block_size - (offset % block_size);

	if((offset+size)>inodo->i_size) {//Agrando el archivo
		error=Inode_truncarArriba(ext2fs,inodo,sb,bgdt,offset+size,nroInodo);
		if(error==-1) return -1;
	}
	bloque = Inode_leerBloque(ext2fs,nroBloqueComienzo,inodo,block_size);

	fseek(ext2fs,(bloque*block_size)+offsetEnBloqueComienzo,SEEK_SET);
	//Si lo que hay q escribir entra en un bloque, escribo eso y salgo
	if(size<bytesAEscribirEnBloqueComienzo) {
		bytesAEscribirEnBloqueComienzo = size;
		fwrite_unlocked(buf,bytesAEscribirEnBloqueComienzo,1,ext2fs);
		cantEscrita+=bytesAEscribirEnBloqueComienzo;
		fseek(ext2fs,(bloque*block_size)+offsetEnBloqueComienzo,SEEK_SET);
		fread_unlocked(escCache,block_size,1,ext2fs);
		pthread_mutex_lock(&mt_cache);
			retorno = cache_subir_infoServer(st_cache,bloque,escCache,block_size);
		pthread_mutex_unlock(&mt_cache);
		free(escCache);
		return cantEscrita;
	}

	fwrite_unlocked(buf,bytesAEscribirEnBloqueComienzo,1,ext2fs);
	fseek(ext2fs,(bloque*block_size)+offsetEnBloqueComienzo,SEEK_SET);
	fread_unlocked(escCache,block_size,1,ext2fs);
	pthread_mutex_lock(&mt_cache);
		retorno = cache_subir_infoServer(st_cache,bloque,escCache,block_size);
	pthread_mutex_unlock(&mt_cache);

	cantEscrita+=bytesAEscribirEnBloqueComienzo;
	size-=bytesAEscribirEnBloqueComienzo;
	buf+=bytesAEscribirEnBloqueComienzo; //corro el puntero

	while(size>block_size) { //escribo los demas bloques
		nroBloqueComienzo++;
		bloque = Inode_leerBloque(ext2fs,nroBloqueComienzo,inodo,block_size);
		fseek(ext2fs,bloque*block_size,SEEK_SET);
		fwrite_unlocked(buf,block_size,1,ext2fs); //leo un bloque entero
		fseek(ext2fs,bloque*block_size,SEEK_SET);
		fread_unlocked(escCache,block_size,1,ext2fs);
		pthread_mutex_lock(&mt_cache);
			retorno = cache_subir_infoServer(st_cache,bloque,escCache,block_size);
		pthread_mutex_unlock(&mt_cache);
		cantEscrita+=block_size;
		size-=block_size;
		buf+=block_size;
	}

	//Escribo lo q falta del buffer
	nroBloqueComienzo++;
	bloque = Inode_leerBloque(ext2fs,nroBloqueComienzo,inodo,block_size);
	fseek(ext2fs,bloque*block_size,SEEK_SET);
	fwrite_unlocked(buf,size,1,ext2fs);
	fseek(ext2fs,bloque*block_size,SEEK_SET);
	fread_unlocked(escCache,block_size,1,ext2fs);
	pthread_mutex_lock(&mt_cache);
		retorno = cache_subir_infoServer(st_cache,bloque,escCache,block_size);
	pthread_mutex_unlock(&mt_cache);

	cantEscrita+=size;

	free(escCache);

	return cantEscrita;
}

int8_t Dir_crearNuevaEntrada(FILE *ext2fs,char *nombreArch,uint32_t nroInodoDir, uint32_t nroInodoCreado,Superblock *sb,Group_descriptor *bgdt){


	bool huboLugar=false;
	uint32_t bloqueDatos,cantBloques,offset,i,tamEntrada,tamNuevaEntrada;
	size_t block_size = 1024 << sb->s_log_block_size;
	Dir *entradaDir=NULL;
	Dir *nuevaEntrada=NULL;
	Inode *inodo;
	char* etiqueta;

	nuevaEntrada = (Dir *)malloc(sizeof(Dir));
	nuevaEntrada->d_filetype=0;
	nuevaEntrada->d_inode=nroInodoCreado;
	nuevaEntrada->d_name = malloc(strlen(nombreArch));
	memcpy(nuevaEntrada->d_name,nombreArch,strlen(nombreArch));
	nuevaEntrada->d_name_len=strlen(nombreArch);
	nuevaEntrada->d_rec_len=0;
	tamNuevaEntrada=(OFF_NAME+nuevaEntrada->d_name_len);
	if(tamNuevaEntrada%4 != 0) tamNuevaEntrada = ((tamNuevaEntrada/4) + 1)*4; //tamNuevaEntrada redondeada a multiplo de 4.

	inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoDir,block_size);

	if(inodo->i_mode==0) { //Esto esta aca por si un pillo me borra el directorio antes de tomar el lock
		free(nuevaEntrada->d_name);
		free(nuevaEntrada);
		free(inodo);
		return -1;
	}

	cantBloques = (inodo->i_size)/block_size;
	if((inodo->i_size)%block_size >0) cantBloques++;

	for(i=0;(i<cantBloques)&&!huboLugar;i++) {
		bloqueDatos = Inode_leerBloque(ext2fs,i,inodo,block_size);
		offset=0;

		while((offset<block_size)&&!huboLugar) {
			entradaDir = (Dir *)malloc(sizeof(Dir));
			//leo los primeros 8 bytes
			fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
			fread_unlocked(entradaDir,OFF_NAME,1,ext2fs);

			tamEntrada=(OFF_NAME+entradaDir->d_name_len);
			if(tamEntrada%4 != 0) tamEntrada = ((tamEntrada/4) + 1)*4; //tamEntrada redondeada a multiplo de 4.

			if((entradaDir->d_rec_len-tamEntrada)>=tamNuevaEntrada) {
				huboLugar=true;
			}else{
				offset+=entradaDir->d_rec_len;
			}
		}
	}
	if(huboLugar){
		nuevaEntrada->d_rec_len=entradaDir->d_rec_len-tamEntrada;
		entradaDir->d_rec_len=tamEntrada;
		entradaDir->d_name=NULL;
		etiqueta = serializarDir(entradaDir, OFF_NAME);

		fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
		fwrite_unlocked(etiqueta,OFF_NAME,1,ext2fs);

		free(etiqueta);

		offset+=tamEntrada;

	}else{

		Inode_truncarArriba(ext2fs,inodo,sb,bgdt,inodo->i_size+block_size,nroInodoDir);
		Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt, nroInodoDir,block_size,inodo);
		bloqueDatos = Inode_leerBloque(ext2fs,i,inodo,block_size);
		offset=0;
		nuevaEntrada->d_rec_len=block_size;

	}
	etiqueta = serializarDir(nuevaEntrada, OFF_NAME+nuevaEntrada->d_name_len);

	fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
	fwrite_unlocked(etiqueta,OFF_NAME+nuevaEntrada->d_name_len,1,ext2fs);

	free(etiqueta);
	free(nuevaEntrada);
	if(entradaDir!=NULL) free(entradaDir);
	free(inodo);
	return 0;

}

int8_t Dir_borrarEntrada(FILE *ext2fs,uint32_t nroInodoDir, uint32_t nroInodoBorrar,Superblock *sb,Group_descriptor *bgdt){

	bool encontro=false;
	uint32_t bloqueDatos,cantBloques,offset,i,offsetAnt;
	size_t block_size = 1024 << sb->s_log_block_size;
	Dir *entradaDir, *entradaDirSgte;

	Inode *inodo;
	char* etiqueta;

	inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoDir,block_size);

	if(inodo->i_mode==0) { //Esto esta aca por si un pillo me borra el directorio antes de tomar el lock
		free(inodo);
		return -1;
	}

	cantBloques = (inodo->i_size)/block_size;
	if((inodo->i_size)%block_size >0) cantBloques++;

	entradaDir = (Dir *)malloc(sizeof(Dir));
	entradaDirSgte = (Dir *)malloc(sizeof(Dir));
	for(i=0;(i<cantBloques)&&!encontro;i++) {
		bloqueDatos = Inode_leerBloque(ext2fs,i,inodo,block_size);
		offsetAnt=0;
		offset = 0;
		fseek(ext2fs,(bloqueDatos*block_size),SEEK_SET);
		fread_unlocked(entradaDir,OFF_NAME,1,ext2fs);
		if(entradaDir->d_inode == nroInodoBorrar){
			encontro = true; //En el primer bloque nunca va a dar TRUE
		}else{
			offset+=entradaDir->d_rec_len;
		}
		while((offset<block_size)&&!encontro) {
			//leo los primeros 8 bytes
			fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
			fread_unlocked(entradaDirSgte,OFF_NAME,1,ext2fs);

			if(entradaDirSgte->d_inode == nroInodoBorrar){
				encontro = true;
			}else{
				memcpy(entradaDir,entradaDirSgte,OFF_NAME);
				offsetAnt = offset;
				offset+=entradaDirSgte->d_rec_len;
			}
		}
	}

	if(encontro){

		if(offset == 0 && offsetAnt == 0){
			entradaDir->d_inode = 0;
		}else{
			entradaDir->d_rec_len += entradaDirSgte->d_rec_len;
		}

		entradaDir->d_name = NULL;

		etiqueta = serializarDir(entradaDir, OFF_NAME);

		fseek(ext2fs,(bloqueDatos*block_size)+offsetAnt,SEEK_SET);
		fwrite_unlocked(etiqueta,OFF_NAME,1,ext2fs);

		free(etiqueta);

	}

	free(entradaDir);
	free(entradaDirSgte);
	free(inodo);
	return 0;

}



char* serializarDir(Dir* entrada, uint32_t tamEntrada){

	char* etiqueta;
	uint32_t offmemcpy = 0;

	etiqueta = (char*)malloc(tamEntrada);
	memcpy(etiqueta+offmemcpy, &entrada->d_inode, sizeof(uint32_t));
	offmemcpy +=sizeof(uint32_t);
	memcpy(etiqueta+offmemcpy, &entrada->d_rec_len,sizeof(uint16_t));
	offmemcpy += sizeof(uint16_t);
	memcpy(etiqueta+offmemcpy, &entrada->d_name_len,sizeof(uint8_t));
	offmemcpy += sizeof(uint8_t);
	memcpy(etiqueta+offmemcpy, &entrada->d_filetype, sizeof(uint8_t));
	offmemcpy +=sizeof(uint8_t);

	if(entrada->d_name!=NULL) {
		memcpy(etiqueta+offmemcpy,entrada->d_name,entrada->d_name_len);
	}

	return etiqueta;
}

int32_t Dir_cantDirectorios(Inode* inodo, 	FILE *ext2fs, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const size_t block_size){

		uint32_t bloqueDatos,cantBloques,offset,i;
		Dir *dirRaiz;
		t_list *lista;
		int32_t cantElementos;

		lista = list_create();
		cantBloques = (inodo->i_size)/block_size;
		if((inodo->i_size)%block_size >0) cantBloques++;

		for(i=0;i<cantBloques;i++) {
			bloqueDatos = Inode_leerBloque(ext2fs,i,inodo,block_size);
			offset=0;

			while(offset<block_size) {
				dirRaiz = (Dir *)malloc(sizeof(Dir));
				//leo los primeros 8 bytes
				fseek(ext2fs,(bloqueDatos*block_size)+offset,SEEK_SET);
				fread_unlocked(dirRaiz,OFF_NAME,1,ext2fs);
				//ahora hago un malloc para el nombre
				dirRaiz->d_name =(char *)malloc(dirRaiz->d_name_len+1);
				//y leo el nombre
				fread_unlocked(dirRaiz->d_name,dirRaiz->d_name_len,1,ext2fs);
				dirRaiz->d_name[dirRaiz->d_name_len] = '\0';
				//agrego la struct dirRaiz a una lista
				//antes de agregar a la lista hay q ver si el inodo es = 0
				//si es 0 quiere decir q se borro
				if(dirRaiz->d_inode !=0) {
					list_add(lista,dirRaiz);
				}
				offset+=dirRaiz->d_rec_len;
			}
		}

		cantElementos = lista->elements_count;
	list_destroy_and_destroy_elements(lista,(void *) Dir_destroy);


	return cantElementos;
}
