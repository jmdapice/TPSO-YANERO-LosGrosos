/*
 * Grupos.h
 *
 *  Created on: 02/05/2012
 *      Author: utnso
 */

#ifndef GRUPOS_H_
#define GRUPOS_H_
#include "../commons/bitarray.h"
#include "Superblock.h"
#include <stdint.h>


typedef struct s_group_descriptor {

	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint32_t bg_reserved1;
	uint32_t bg_reserved2;
	uint32_t bg_reserved3;

} Group_descriptor;

Group_descriptor *Grupos_leerTabla(FILE*, const Superblock);
void Grupos_escribirTabla(FILE* ext2fs,Superblock *sb, Group_descriptor *bgdt);
t_bitarray *Grupos_leerBlockBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla);
t_bitarray *Grupos_leerInodeBitmap(FILE*, const uint32_t, Superblock*, const Group_descriptor*);
uint32_t Grupos_buscarBloqueLibrePorGrupo(FILE* ext2fs,const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla);
int32_t Grupos_buscarBloqueLibre(FILE*, const Superblock*, const Group_descriptor*);
void Grupos_escribirBlockBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla,t_bitarray *block_bitmap);
void Grupos_liberarBloqueEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroBloque);
void Grupos_utilizarBloqueEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroBloque);
void Grupos_utilizarInodoEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroInodo);
void Grupos_liberarInodoEnBitmap(FILE *ext2fs, Superblock *sb, Group_descriptor *bgdt, uint32_t nroInodo);
void Grupos_escribirInodeBitmap(FILE* ext2fs, const uint32_t nroGrupo, const Superblock *sb, const Group_descriptor *tabla,t_bitarray *inode_bitmap);
int32_t Grupos_pedirBloqueLibre(FILE*, Superblock*, Group_descriptor*);

#endif /* GRUPOS_H_ */
