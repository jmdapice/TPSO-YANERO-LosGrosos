/*
 * Superblock.c
 *
 *  Created on: 22/04/2012
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "Rfs.h"
#include "Superblock.h"

extern pthread_mutex_t mt_sb;

Superblock Superblock_leer(FILE* ext2fs) {

	Superblock sb;

	fseek(ext2fs,1024,SEEK_SET); //Muevo puntero a donde empieza el sb

	if(fread_unlocked(&sb,sizeof(sb),1,ext2fs) == 0) {
		puts("Error de lectura\n");
		exit(0);
	}

	return sb;
}

void Superblock_escribir(FILE* ext2fs,Superblock *sb) {

	pthread_mutex_lock(&mt_sb);
	fseek(ext2fs,1024,SEEK_SET); //Muevo puntero a donde empieza el sb
	if(fwrite_unlocked(sb,sizeof(Superblock),1,ext2fs) == 0) {
		puts("Error de escritura en Superblock\n");
		exit(0);
	}
	pthread_mutex_unlock(&mt_sb);
}
