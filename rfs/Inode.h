/*
 * Inode.h
 *
 *  Created on: 05/05/2012
 *      Author: utnso
 */

#ifndef INODE_H_
#define INODE_H_
#include <stdint.h>
#include "Grupos.h"
#include "../commons/bitarray.h"


typedef struct s_inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	uint32_t i_faddr;
	uint32_t i_osd2a;
	uint32_t i_osd2b;
	uint32_t i_osd2c;
} Inode;

typedef struct {
    bool otherExec  :1;
    bool otherWrite :1;
    bool otherRead  :1;
    bool groupExec  :1;
    bool groupWrite :1;
    bool groupRead  :1;
    bool userExec   :1;
    bool userWrite  :1;
    bool userRead   :1;
} t_permisos;

Inode *Inode_leerTabla(FILE*, const uint32_t, const uint32_t, Group_descriptor*, const size_t);
uint32_t Inode_buscarLibrePorGrupo(FILE*, const uint32_t, Superblock*, const Group_descriptor*);
t_permisos Inode_leerPermisos(uint16_t);
uint32_t Inode_leerBloque(FILE*, uint32_t ,Inode*, size_t);
Inode *Inode_leerInodo(FILE*, const uint32_t, const Group_descriptor*, const uint32_t, const size_t);
int32_t Inode_buscarLibre (FILE*, Superblock*, const Group_descriptor*);
void Inode_truncarAbajo(FILE *ext2fs,Inode *inodo,Superblock *sb,Group_descriptor *bgdt,uint32_t nuevoTam, uint32_t nroInodo);
int8_t Inode_truncarArriba(FILE *ext2fs,Inode *inodo,Superblock *sb,Group_descriptor *bgdt,uint32_t nuevoTam, uint32_t nroInodo);
void Inode_escribirInodo(FILE *ext2fs, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const uint32_t nroInodo, const size_t block_size, Inode *inodo);
int8_t Inode_asignarBloque(FILE *ext2fs, uint32_t nroBloque, Inode *inodo, Superblock *sb, Group_descriptor *bgdt);
int32_t Inode_pedirInodoLibre(FILE* ext2fs, Superblock* sb, Group_descriptor* bgdt);
int32_t Inode_asignarInodo(FILE *ext2fs,Superblock *sb,Group_descriptor *bgdt, uint16_t mode, uint16_t links);
#endif /* INODE_H_ */
