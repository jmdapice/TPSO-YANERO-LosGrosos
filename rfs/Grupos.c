/*
 * Grupos.c
 *
 *  Created on: 02/05/2012
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Grupos.h"
#include "Superblock.h"
#include "../commons/bitarray.h"
#include "../commons/log.h"
#include <pthread.h>



extern pthread_mutex_t mt_buscarBloque;
extern pthread_mutex_t mt_buscarInodo;
extern pthread_mutex_t mt_bgdt;

Group_descriptor *Grupos_leerTabla(FILE* ext2fs, const Superblock sb) {

	Group_descriptor *tabla;
	size_t block_size;
	uint32_t byteComienzo,cantGrupos;

	block_size = 1024 << sb.s_log_block_size;
	byteComienzo = block_size*(sb.s_first_data_block + 1) ;
	cantGrupos= 1 +(sb.s_blocks_count-sb.s_first_data_block) / sb.s_blocks_per_group;


	tabla = (Group_descriptor*)malloc(sizeof(*tabla)*cantGrupos);
	fseek(ext2fs,byteComienzo,SEEK_SET);
	if(fread_unlocked(tabla,sizeof(*tabla)*cantGrupos,1,ext2fs)==0) {
		puts("Error de lectura\n");
		exit(0);
	}

	if(tabla==NULL) {
		perror("Error leyendo bgdt\n");
		exit(0);
	}
	return tabla;
}

void Grupos_escribirTabla(FILE* ext2fs,Superblock *sb, Group_descriptor *bgdt) {

	size_t block_size;
	uint32_t byteComienzo,cantGrupos;

	block_size = 1024 << sb->s_log_block_size;
	byteComienzo = block_size*(sb->s_first_data_block + 1) ;
	cantGrupos= 1 +(sb->s_blocks_count-sb->s_first_data_block) / sb->s_blocks_per_group;

	pthread_mutex_lock(&mt_bgdt);
	fseek(ext2fs,byteComienzo,SEEK_SET);
	if(fwrite_unlocked(bgdt,sizeof(Group_descriptor)*cantGrupos,1,ext2fs)==0) {
		puts("Error escribiendo bgdt\n");
		exit(0);
	}
	pthread_mutex_unlock(&mt_bgdt);

}



t_bitarray *Grupos_leerBlockBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla) {

	t_bitarray *block_bitmap;
	size_t block_size,tamArray;
	char *bitarray;
	block_size = 1024 << sb->s_log_block_size;
	uint32_t cantGrupos= 1 +(sb->s_blocks_count-sb->s_first_data_block) / sb->s_blocks_per_group;
	//Si es el ultimo grupo, tiene menos bloques
	if (nroGrupo==cantGrupos-1) {
		tamArray=sb->s_blocks_count-((cantGrupos-1)*sb->s_blocks_per_group);
	} else {//Si es otro grupo, tiene lo que dice el sb
		tamArray=sb->s_blocks_per_group;
	}
	tamArray=tamArray/8; //1 byte = 8 bits
	if(tamArray%8>0) tamArray++;//redondeo para arriba
	bitarray = malloc(tamArray);
	block_bitmap = bitarray_create(bitarray, tamArray);
	fseek(ext2fs,(tabla[nroGrupo].bg_block_bitmap)*block_size, SEEK_SET);
	fread_unlocked(block_bitmap->bitarray,tamArray,1,ext2fs);

	return block_bitmap;

}

void Grupos_escribirBlockBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla,t_bitarray *block_bitmap) {

	size_t block_size = 1024 << sb->s_log_block_size;
	fseek(ext2fs,(tabla[nroGrupo].bg_block_bitmap)*block_size, SEEK_SET);
	fwrite_unlocked(block_bitmap->bitarray,block_bitmap->size,1,ext2fs);
}

void Grupos_liberarBloqueEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroBloque) {

	uint32_t nroGrupo;
	nroBloque-=sb->s_first_data_block;
	nroGrupo=nroBloque/sb->s_blocks_per_group;
	//if((nroBloque%sb->s_blocks_per_group)>0) nroGrupo++;
	pthread_mutex_lock(&mt_buscarBloque);
		t_bitarray *block_bitmap = Grupos_leerBlockBitmap(ext2fs,nroGrupo,sb,bgdt);
		bitarray_clean_bit(block_bitmap,nroBloque-(nroGrupo*sb->s_blocks_per_group));
		// t_log *log2=log_create("log2.txt","blk",true,LOG_LEVEL_DEBUG);
		// log_debug(log2,"BLK Liberado bloque %d",nroBloque);
		// log_destroy(log2);
		Grupos_escribirBlockBitmap(ext2fs,nroGrupo,sb,bgdt,block_bitmap);
		bitarray_destroy(block_bitmap);
		sb->s_free_blocks_count++;
		bgdt[nroGrupo].bg_free_blocks_count++;
	pthread_mutex_unlock(&mt_buscarBloque);
}

void Grupos_utilizarBloqueEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroBloque) {

	uint32_t nroGrupo;
	nroBloque-=sb->s_first_data_block;
	nroGrupo=nroBloque/sb->s_blocks_per_group;
	//if((nroBloque%sb->s_blocks_per_group)>0) nroGrupo++;
	t_bitarray *block_bitmap = Grupos_leerBlockBitmap(ext2fs,nroGrupo,sb,bgdt);
	bitarray_set_bit(block_bitmap,nroBloque-(nroGrupo*sb->s_blocks_per_group));
	// t_log *log2=log_create("log2.txt","blk",true,LOG_LEVEL_DEBUG);
	// log_debug(log2,"BLK Ocupado bloque %d",nroBloque);
	// log_destroy(log2);
	Grupos_escribirBlockBitmap(ext2fs,nroGrupo,sb,bgdt,block_bitmap);
	sb->s_free_blocks_count--;
	bgdt[nroGrupo].bg_free_blocks_count--;

	bitarray_destroy(block_bitmap);

}


t_bitarray *Grupos_leerInodeBitmap(FILE* ext2fs, const uint32_t nroGrupo, Superblock *sb, const Group_descriptor *tabla) {

	t_bitarray *inode_bitmap;
	size_t block_size,tamArray;
	char *bitarray;
	block_size = 1024 << sb->s_log_block_size;

	tamArray=sb->s_inodes_per_group;

	tamArray=tamArray/8; //1 byte = 8 bits
	if(tamArray%8>0) tamArray++;//redondeo para arriba

	bitarray = malloc(tamArray);
	inode_bitmap = bitarray_create(bitarray, tamArray);
	fseek(ext2fs,(tabla[nroGrupo].bg_inode_bitmap)*block_size, SEEK_SET);
	fread_unlocked(inode_bitmap->bitarray,tamArray,1,ext2fs);

	return inode_bitmap;

}

void Grupos_escribirInodeBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla,t_bitarray *inode_bitmap) {

	size_t block_size = 1024 << sb->s_log_block_size;
	fseek(ext2fs,(tabla[nroGrupo].bg_inode_bitmap)*block_size, SEEK_SET);
	fwrite_unlocked(inode_bitmap->bitarray,inode_bitmap->size,1,ext2fs);
}

void Grupos_liberarInodoEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroInodo) {

	uint32_t nroGrupo;
	nroInodo--;
	nroGrupo=nroInodo/sb->s_inodes_per_group;
	pthread_mutex_lock(&mt_buscarInodo);
		t_bitarray *inode_bitmap = Grupos_leerInodeBitmap(ext2fs,nroGrupo,sb,bgdt);
		bitarray_clean_bit(inode_bitmap,nroInodo-(nroGrupo*sb->s_inodes_per_group));
		Grupos_escribirInodeBitmap(ext2fs,nroGrupo,sb,bgdt,inode_bitmap);
		bitarray_destroy(inode_bitmap);
		sb->s_free_inodes_count++;
		bgdt[nroGrupo].bg_free_inodes_count++;
	pthread_mutex_unlock(&mt_buscarInodo);
}

void Grupos_utilizarInodoEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroInodo) {

	uint32_t nroGrupo;
	nroInodo--; //Resto 1 pq los inodos empiezan del 1
	nroGrupo=nroInodo/sb->s_inodes_per_group;
	t_bitarray *inode_bitmap = Grupos_leerInodeBitmap(ext2fs,nroGrupo,sb,bgdt);
	bitarray_set_bit(inode_bitmap,nroInodo-(nroGrupo*sb->s_inodes_per_group));
	Grupos_escribirInodeBitmap(ext2fs,nroGrupo,sb,bgdt,inode_bitmap);
	sb->s_free_inodes_count--;
	bgdt[nroGrupo].bg_free_inodes_count--;

	bitarray_destroy(inode_bitmap);

}



uint32_t Grupos_buscarBloqueLibrePorGrupo(FILE* ext2fs,const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla){

	//Busco un bloque libre según el grupo

	t_bitarray *block_bitmap;
	uint32_t bloque;

	//uint32_t cantGrupos= 1 +(sb->s_blocks_count-sb->s_first_data_block) / sb->s_blocks_per_group;
	block_bitmap = Grupos_leerBlockBitmap(ext2fs,nroGrupo,sb,tabla);
	bloque = bitarray_buscarPosicionLibre(block_bitmap);
	bitarray_destroy(block_bitmap);
	return bloque; //Retorno Numero de Bloque Libre dentro del Grupo nroGrupo
}

int32_t Grupos_buscarBloqueLibre(FILE* ext2fs, const Superblock *sb, const Group_descriptor *bgdt){

	//Busco eun bloque libre sin importarme el Grupo
	//Devuelve -1 en caso de no haber bloque libre

	uint32_t nroBloque;
	uint32_t nroGrupo;
	uint32_t cantGrupos;

	cantGrupos= 1 +(sb->s_blocks_count-sb->s_first_data_block) / sb->s_blocks_per_group;

	for(nroGrupo = 0; nroGrupo < cantGrupos;nroGrupo++){

		if (bgdt[nroGrupo].bg_free_blocks_count != 0) {

			nroBloque = Grupos_buscarBloqueLibrePorGrupo(ext2fs,nroGrupo,sb,bgdt);
			return ( nroGrupo  *  sb->s_blocks_per_group ) + nroBloque + sb->s_first_data_block;
		}

	}

	return -1; //Retorno Numero de Bloque Libre dentro del Grupo nroGrupo
}

int32_t Grupos_pedirBloqueLibre(FILE* ext2fs, Superblock* sb, Group_descriptor* bgdt){

	int32_t bloqueADevolver;

	//Se decide agregar un mutex para buscar un bloque, debido a que si 2 bloques buscaran a la vez un bloque
	//libre podrian agarrar los 2 el mismo. Sobre todo porque el bloque libre que me trae va a ser al azar

	pthread_mutex_lock(&mt_buscarBloque);
		bloqueADevolver = Grupos_buscarBloqueLibre(ext2fs,sb,bgdt); //Le asigno a bloque1 el nuevo bloque de punteros
		Grupos_utilizarBloqueEnBitmap(ext2fs,sb,bgdt,bloqueADevolver);// Seteo el bitmap de bloques
	pthread_mutex_unlock(&mt_buscarBloque);

	return bloqueADevolver;

}
