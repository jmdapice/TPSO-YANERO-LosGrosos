/*
 * Inode.c
 *
 *  Created on: 05/05/2012
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "Inode.h"
#include "Grupos.h"
#include "../commons/bitarray.h"
#include "Rfs.h"

extern pthread_mutex_t mt_buscarInodo;

uint32_t buscarBloque(FILE*, const uint32_t, const uint32_t, const uint32_t, const size_t);

void escribirCero(FILE *ext2fs, const uint32_t bloque, const uint32_t pos, const uint32_t tamPuntero, const size_t block_size);

void inicializarBloqdePtros(FILE* ext2fs, const uint32_t bloque, const uint32_t tamPuntero, const uint32_t cantPunteros, const size_t block_size);

Inode *Inode_leerTabla(FILE* ext2fs, const uint32_t nroGrupo, const uint32_t cantInodos, Group_descriptor* bgdt, const size_t block_size) {

	Inode *tabla;
	tabla = (Inode*)malloc(sizeof(*tabla)*cantInodos);
	fseek(ext2fs,(bgdt[nroGrupo].bg_inode_table)*block_size,SEEK_SET);
	if(fread_unlocked(tabla,sizeof(*tabla)*cantInodos,1,ext2fs)==0) {
		puts("Error de lectura\n");
		exit(0);
	}
	return tabla;

}


int32_t Inode_buscarLibre (FILE* ext2fs, Superblock *sb, const Group_descriptor *bgdt) {

	//Busco un inodo libre en todos los grupos
	//Devuelve 0 en caso de no haber inodos libres

	uint32_t nroInodo;
	uint32_t nroGrupo;
	uint32_t cantGrupos;

	cantGrupos= 1 +(sb->s_blocks_count-sb->s_first_data_block) / sb->s_blocks_per_group;

	for(nroGrupo = 0;nroGrupo <= cantGrupos - 1; nroGrupo++) {
		if (bgdt[nroGrupo].bg_free_inodes_count != 0) {
			nroInodo = Inode_buscarLibrePorGrupo(ext2fs,nroGrupo,sb,bgdt);
			return ( nroGrupo  *  sb->s_inodes_per_group ) + 1 + nroInodo;
		}

	}

	return -1;
}


uint32_t Inode_buscarLibrePorGrupo(FILE* ext2fs, const uint32_t nroGrupo, Superblock *sb, const Group_descriptor *tabla){

	//Busca un inodo libre en un grupo determinado

	t_bitarray *inode_bitmap;
	uint32_t inodo;
	inode_bitmap = Grupos_leerInodeBitmap(ext2fs, nroGrupo,sb,tabla);
	inodo = bitarray_buscarPosicionLibre(inode_bitmap);
	bitarray_destroy(inode_bitmap);
	return inodo; //Retorno Numero de Inodo Libre dentro del Grupo nroGrupo
}

t_permisos Inode_leerPermisos(uint16_t imodo) {
    t_permisos *permisos = (t_permisos*)&imodo;
    return *permisos;
}

uint32_t Inode_leerBloque(FILE *ext2fs, uint32_t nroBloque, Inode *inodo, size_t block_size) {


	uint32_t pos,pos2,pos3,bloque;
	uint32_t tamPuntero = sizeof(uint32_t);
	uint32_t cantPunteros = block_size/tamPuntero;

	if(nroBloque<12) {
		return inodo->i_block[nroBloque];
	} else {
		nroBloque = nroBloque - 12;
		if(nroBloque<cantPunteros) {
			bloque = inodo->i_block[12];
			pos = nroBloque;
			return buscarBloque(ext2fs,bloque,pos,tamPuntero,block_size);
		} else {
			nroBloque = nroBloque - cantPunteros;
			if (nroBloque < cantPunteros*cantPunteros) {
				pos2 = nroBloque % cantPunteros; //Posicion del puntero al dato
				pos = nroBloque / cantPunteros; //Posicion del puntero al bloque de punteros a datos
				bloque = inodo->i_block[13];
				return buscarBloque(ext2fs,buscarBloque(ext2fs,bloque,pos,tamPuntero,block_size),pos2,tamPuntero,block_size);
			} else {
				nroBloque = nroBloque - cantPunteros*cantPunteros;
				if (nroBloque < cantPunteros*cantPunteros*cantPunteros) {
					pos3 = nroBloque % cantPunteros; //Posicion del puntero al dato
					pos2 = (nroBloque / cantPunteros) % cantPunteros; //Posicion del puntero al dato
					pos = (nroBloque / cantPunteros) / cantPunteros; //Posicion del puntero al bloque de punteros a datos
					bloque = inodo->i_block[14];
					return buscarBloque(ext2fs,buscarBloque(ext2fs,buscarBloque(ext2fs,bloque,pos,tamPuntero,block_size),pos2,tamPuntero,block_size),pos3,tamPuntero,block_size);
				}
			}
		}

	}
	return 0;
}

uint32_t buscarBloque(FILE *ext2fs, const uint32_t bloque, const uint32_t pos, const uint32_t tamPuntero, const size_t block_size) {

	uint32_t bloqueAdevolver,offset;
	offset = (bloque * block_size) + pos * tamPuntero;
	fseek(ext2fs, offset, SEEK_SET);
	fread_unlocked(&bloqueAdevolver, tamPuntero, 1, ext2fs);
	return bloqueAdevolver;
}

Inode *Inode_leerInodo(FILE *ext2fs, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const uint32_t nroInodo, const size_t block_size) {

	//nroInodo es desde 1 hasta n; siendo n = sb.s_inodes_count.

	Inode *inodo;
	uint32_t grupo_inodo, indiceEnGrupo;
	uint32_t tamano_inodo;
	uint32_t inicio_tabla_inodo;
    uint32_t offset;

	inodo = (Inode*)malloc(sizeof(Inode));
	tamano_inodo = sizeof(*inodo); //128
	grupo_inodo  = (nroInodo - 1) / inodos_por_grupo;
	indiceEnGrupo = (nroInodo - 1) % inodos_por_grupo;
	offset = (indiceEnGrupo * tamano_inodo);
	inicio_tabla_inodo = (bgdt[grupo_inodo].bg_inode_table) * block_size;
	fseek(ext2fs,inicio_tabla_inodo + offset,SEEK_SET);
	fread_unlocked(inodo,tamano_inodo,1, ext2fs);
	return inodo;
}

void Inode_escribirInodo(FILE *ext2fs, const uint32_t inodos_por_grupo, const Group_descriptor *bgdt, const uint32_t nroInodo, const size_t block_size, Inode *inodo) {

	uint32_t grupo_inodo, indiceEnGrupo;
	uint32_t tamano_inodo;
	uint32_t inicio_tabla_inodo;
    uint32_t offset;

	tamano_inodo = sizeof(Inode); //128
	grupo_inodo  = (nroInodo - 1) / inodos_por_grupo;
	indiceEnGrupo = (nroInodo - 1) % inodos_por_grupo;
	offset = (indiceEnGrupo * tamano_inodo);
	inicio_tabla_inodo = (bgdt[grupo_inodo].bg_inode_table) * block_size;
	fseek(ext2fs,inicio_tabla_inodo + offset,SEEK_SET);
	fwrite_unlocked(inodo,tamano_inodo,1, ext2fs);

}

void Inode_liberarBloque(FILE *ext2fs, uint32_t nroBloque, Inode *inodo, Superblock *sb, Group_descriptor *bgdt) {

	uint32_t pos,pos2,pos3,bloque1,bloque2,bloque3,bloque4;
	uint32_t tamPuntero = sizeof(uint32_t);
	size_t block_size = 1024 << sb->s_log_block_size;
	uint32_t cantPunteros = block_size/tamPuntero;

	if(nroBloque<12) {
		bloque1 = inodo->i_block[nroBloque];
		inodo->i_block[nroBloque] = 0;
		Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque1);// LIBERAR BITMAP BLOQUE1
	} else {
		nroBloque = nroBloque - 12;
		if(nroBloque<cantPunteros) {
			pos = nroBloque;
			bloque1 = inodo->i_block[12];
			bloque2 = buscarBloque(ext2fs,bloque1,pos,tamPuntero,block_size);
			if(pos == 0) {//si pos es 0 el bloque es el ultimo del array entonces libero el puntero que apunta al array
				inodo->i_block[12] = 0;
				Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque1);
			}
			escribirCero(ext2fs,bloque1,pos,tamPuntero,block_size);//ESCRIBIR EN DISCO UN CERO EN POSICION DE BLOQUE1
			Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque2);
		} else {
			nroBloque = nroBloque - cantPunteros;
			if (nroBloque < cantPunteros*cantPunteros) {
				pos2 = nroBloque % cantPunteros; //Posicion del puntero al dato
				pos = nroBloque / cantPunteros; //Posicion del puntero al bloque de punteros a datos
				bloque1 = inodo->i_block[13];
				bloque2 = buscarBloque(ext2fs,bloque1,pos,tamPuntero,block_size);
				bloque3 = buscarBloque(ext2fs,bloque2,pos2,tamPuntero,block_size);
				if(pos2 == 0){ // si pos2 es 0 el bloque es el ultimo del array entonces libero el puntero que apunta al array
					escribirCero(ext2fs,bloque1,pos,tamPuntero,block_size);//ESCRIBIR EN DISCO UN CERO EN POSICION DE BLOQUE1
					Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque2);
					if(pos == 0){//si pos tb era 0 entonces se acabo el indireccionamiento
						inodo->i_block[13] = 0;
						Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque1);
					}
				}
				escribirCero(ext2fs,bloque2,pos2,tamPuntero,block_size);//ESCRIBIR EN DISCO UN CERO EN POSICION DE BLOQUE2
				Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque3);
			} else {
				nroBloque = nroBloque - cantPunteros*cantPunteros;
				if (nroBloque < cantPunteros*cantPunteros*cantPunteros) {
					pos3 = nroBloque % cantPunteros; //Posicion del puntero al dato
					pos2 = (nroBloque / cantPunteros) % cantPunteros; //Posicion del puntero al dato
					pos = (nroBloque / cantPunteros) / cantPunteros; //Posicion del puntero al bloque de punteros a datos
					bloque1 = inodo->i_block[14];
					bloque2 = buscarBloque(ext2fs,bloque1,pos,tamPuntero,block_size);
					bloque3 = buscarBloque(ext2fs,bloque2,pos2,tamPuntero,block_size);
					bloque4 = buscarBloque(ext2fs,bloque3,pos3,tamPuntero,block_size);
					if(pos3 == 0){// si pos3 es 0 el bloque es el ultimo del array entonces libero el puntero que apunta al array
						escribirCero(ext2fs,bloque2,pos2,tamPuntero,block_size);
						Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque3);
						if(pos2 == 0){// si TAMBIEN pos2 es 0 el bloque es el ultimo del array entonces libero el puntero que apunta al array
							escribirCero(ext2fs,bloque1,pos,tamPuntero,block_size);
							Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque2);
							if(pos == 0){//si pos tb era 0 entonces se acabo el indireccionamiento
								inodo->i_block[14] = 0;
								Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque1);
							}
						}
					}
					escribirCero(ext2fs,bloque3,pos3,tamPuntero,block_size);//ESCRIBIR EN DISCO UN CERO EN POSICION DE BLOQUE3
					Grupos_liberarBloqueEnBitmap(ext2fs,sb,bgdt,bloque4);

				}
			}
		}

	}

}

void escribirCero(FILE *ext2fs, const uint32_t bloque, const uint32_t pos, const uint32_t tamPuntero, const size_t block_size) {

		uint32_t cero = 0;
	    uint32_t offset;
		offset = (bloque * block_size) + pos * tamPuntero;
		fseek(ext2fs, offset, SEEK_SET);
		fwrite_unlocked(&cero, sizeof(uint32_t), 1, ext2fs);
}

void Inode_truncarAbajo(FILE *ext2fs,Inode *inodo,Superblock *sb,Group_descriptor *bgdt,uint32_t nuevoTam, uint32_t nroInodo){

	uint32_t cantBloques,cantBloquesNuevo,i;
	size_t block_size = 1024 << sb->s_log_block_size;
	cantBloques = (inodo->i_size)/block_size;
	if((inodo->i_size)%block_size >0) cantBloques++;
	cantBloquesNuevo = nuevoTam/block_size;
	if((nuevoTam)%block_size >0) cantBloquesNuevo++;
	for(i=cantBloques;i>cantBloquesNuevo;i--){
		Inode_liberarBloque(ext2fs,i-1,inodo,sb,bgdt);
	}
	//actualizar? inode->i_blocks (bloques de 512b)
	inodo->i_size=nuevoTam;

}

void escribirPtroBloq(FILE *ext2fs, const uint32_t bloque, const uint32_t pos, const uint32_t tamPuntero, const size_t block_size, const uint32_t bloqAbs) {

	    uint32_t offset;
		offset = (bloque * block_size) + pos * tamPuntero;
		fseek(ext2fs, offset, SEEK_SET);
		fwrite_unlocked(&bloqAbs, sizeof(uint32_t), 1, ext2fs);
}

void inicializarBloqdePtros(FILE* ext2fs, const uint32_t bloque, const uint32_t tamPuntero, const uint32_t cantPunteros, const size_t block_size){

	uint32_t offset;
	offset = (bloque * block_size);
	char *buf = malloc(block_size);
	memset(buf,0,block_size);
	fseek(ext2fs, offset, SEEK_SET);
	fwrite_unlocked(buf, block_size, 1, ext2fs);
	free(buf);
}

uint32_t esCero(FILE* ext2fs, const uint32_t bloque,const size_t block_size,const uint32_t pos,const uint32_t tamPuntero){

	uint32_t offset = (bloque * block_size) + pos * tamPuntero;
	uint32_t lectura;
	fseek(ext2fs, offset, SEEK_SET);
	fread_unlocked(&lectura, sizeof(uint32_t),1,ext2fs);

	return lectura;
}

int8_t Inode_asignarBloque(FILE *ext2fs, uint32_t nroBloque, Inode *inodo, Superblock *sb, Group_descriptor *bgdt) {

	//funcion que retorna la cantidad de bloques q se reservaron al inodo;
	//esta funcion basicamente lo que hace es ver cuandos bloques necesita, y los pide todos juntos

	uint32_t pos,pos2,pos3;
	int32_t bloque1,bloque2,bloque3,bloque4;
	uint32_t tamPuntero = sizeof(uint32_t);
	size_t block_size = 1024 << sb->s_log_block_size;
	uint32_t cantPunteros = block_size/tamPuntero;

	if(nroBloque<12) {

		if(sb->s_free_blocks_count < 1) return -1;

		bloque1 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
		inodo->i_block[nroBloque] = bloque1;

	} else {

		nroBloque = nroBloque - 12;
		if(nroBloque<cantPunteros) {

			pos = nroBloque;

			if (inodo->i_block[12] == 0){ //Si es el primer bloque del indireccionamiento

				if(sb->s_free_blocks_count < 2) return -1;

				bloque1 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
				inodo->i_block[12] = bloque1;
				bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

				inicializarBloqdePtros(ext2fs,bloque1,tamPuntero,cantPunteros,block_size);

				escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);//Escribir puntero a bloque de datos

			}else{

				if(sb->s_free_blocks_count < 1) return -1;

				bloque1 = inodo->i_block[12]; //bloque de punteros lo tengo desde antes

				bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

				escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);//Escribir puntero a bloque de datos

			}

		} else {
			nroBloque = nroBloque - cantPunteros;
			if (nroBloque < cantPunteros*cantPunteros) {
				pos2 = nroBloque % cantPunteros; //Posicion del puntero al dato
				pos = nroBloque / cantPunteros; //Posicion del puntero al bloque de punteros a datos

				if (inodo->i_block[13] == 0){ //Si es el primer bloque del indireccionamiento

					if(sb->s_free_blocks_count < 3) return -1;

					bloque1 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
					inodo->i_block[13] = bloque1;
					bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
					bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

					inicializarBloqdePtros(ext2fs,bloque1,tamPuntero,cantPunteros,block_size);
					inicializarBloqdePtros(ext2fs,bloque2,tamPuntero,cantPunteros,block_size);
					escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);//Escribir puntero a bloque de datos
					escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);//Escribir puntero a bloque de datos

				}else{
					bloque1 = inodo->i_block[13]; //bloque de punteros lo tengo desde antes
					if(esCero(ext2fs,bloque1,block_size,pos,tamPuntero) == 0){

						if(sb->s_free_blocks_count < 2) return -1;

						bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
						bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

						inicializarBloqdePtros(ext2fs,bloque2,tamPuntero,cantPunteros,block_size);
						escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);//Escribir puntero a bloque de punteros
						escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);

					}else{

						if(sb->s_free_blocks_count < 1) return -1;

						bloque2 = buscarBloque(ext2fs,bloque1,pos,tamPuntero,block_size);

						bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

						escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);//Escribir puntero a bloque de datos

					}
				}


			} else {
				nroBloque = nroBloque - cantPunteros*cantPunteros;
				if (nroBloque < cantPunteros*cantPunteros*cantPunteros) {
					pos3 = nroBloque % cantPunteros; //Posicion del puntero al dato
					pos2 = (nroBloque / cantPunteros) % cantPunteros; //Posicion del puntero al dato
					pos = (nroBloque / cantPunteros) / cantPunteros; //Posicion del puntero al bloque de punteros a datos

					if(inodo->i_block[14] == 0){

						if(sb->s_free_blocks_count < 4) return -1;

						bloque1 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
						inodo->i_block[14] = bloque1;
						bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
						bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
						bloque4 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

						inicializarBloqdePtros(ext2fs,bloque1,tamPuntero,cantPunteros,block_size);
						inicializarBloqdePtros(ext2fs,bloque2,tamPuntero,cantPunteros,block_size);
						inicializarBloqdePtros(ext2fs,bloque3,tamPuntero,cantPunteros,block_size);

						escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);//Escribir puntero a bloque de punteros
						escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);//Escribir puntero a bloque de punteros
						escribirPtroBloq(ext2fs,bloque3,pos3,tamPuntero,block_size,bloque4);//Escribir puntero a bloque de datos

					}else{

						bloque1 = inodo->i_block[14]; //bloque de punteros lo tengo desde antes
						if(esCero(ext2fs,bloque1,block_size,pos,tamPuntero) == 0){

							if(sb->s_free_blocks_count <3) return -1;

							bloque2 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
							bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
							bloque4 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

							inicializarBloqdePtros(ext2fs,bloque2,tamPuntero,cantPunteros,block_size);
							inicializarBloqdePtros(ext2fs,bloque3,tamPuntero,cantPunteros,block_size);

							escribirPtroBloq(ext2fs,bloque1,pos,tamPuntero,block_size,bloque2);
							escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);//Escribir puntero a bloque de punteros
							escribirPtroBloq(ext2fs,bloque3,pos3,tamPuntero,block_size,bloque4);//Escribir puntero a bloque de datos

						}else{ //Quiere decir que ese bloque ya existia

							bloque2 = buscarBloque(ext2fs,bloque1,pos,tamPuntero,block_size);

							if(esCero(ext2fs,bloque2,block_size,pos2,tamPuntero) == 0){

								if(sb->s_free_blocks_count < 2) return -1;

								bloque3 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);
								bloque4 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

								inicializarBloqdePtros(ext2fs,bloque3,tamPuntero,cantPunteros,block_size);

								escribirPtroBloq(ext2fs,bloque2,pos2,tamPuntero,block_size,bloque3);
								escribirPtroBloq(ext2fs,bloque3,pos3,tamPuntero,block_size,bloque4);//Escribir puntero a bloque de datos


							}else{

								if(sb->s_free_blocks_count <1) return -1;

								bloque3 = buscarBloque(ext2fs,bloque2,pos2,tamPuntero,block_size);
								//No chequeo que el puntero (bloque3,pos3) sea 0 porq se supone q lo es
								bloque4 =  Grupos_pedirBloqueLibre(ext2fs,sb,bgdt);

								escribirPtroBloq(ext2fs,bloque3,pos3,tamPuntero,block_size,bloque4);//Escribir puntero a bloque de datos

							}

						}//

					}//


				}//
			}
		}
	}

	return 0;
}


int8_t Inode_truncarArriba(FILE *ext2fs,Inode *inodo,Superblock *sb,Group_descriptor *bgdt,uint32_t nuevoTam, uint32_t nroInodo){

	// El problema de que si el nuevo tamaño entra o no en el disco se soluciona de la siguiente manera:
	// si no llega a haber bloques libres, directamente se retorna error y se hace un truncarAbajo para
	// volver el archivo a su estado original.


	uint32_t cantBloques,cantBloquesNuevo,i;
	size_t block_size = 1024 << sb->s_log_block_size;
	int8_t error = 0;

	uint32_t tamanio_anterior = 0; //Usado para casos de error

	cantBloques = (inodo->i_size)/block_size;
	if((inodo->i_size)%block_size >0) cantBloques++;
	cantBloquesNuevo = nuevoTam/block_size;
	if((nuevoTam)%block_size >0) cantBloquesNuevo++;

	for(i=cantBloques; i<cantBloquesNuevo && error == 0;i++){ //Esta i me da, el nuevo tamaño del Inodo por cada vuelta
			error = Inode_asignarBloque(ext2fs,i,inodo,sb,bgdt);
	}

	//Nota al pie para entender el for con un ejemplo
	// 0 1 2 3 4  son los 5 bloques anteriores
	// 0 1 2 3 4 5 6 7 son los 8 bloques nuevos
	// entonces yo parto desde cantBloques (5) y asigno bloques mientras bloque i sea menor a cantBloquesNuevo (o sea, va hasta 7) ;)

	//actualizar? inode->i_blocks (bloques de 512b)


	if(error == -1) {
		tamanio_anterior = inodo->i_size;
		inodo->i_size = (i-1)*block_size;
		Inode_truncarAbajo(ext2fs,inodo,sb,bgdt,tamanio_anterior,nroInodo);
		return -1;
	}

	inodo->i_size=nuevoTam;
	return 0;

}

int32_t Inode_pedirInodoLibre(FILE* ext2fs, Superblock* sb, Group_descriptor* bgdt){

	int32_t inodoADevolver;

	pthread_mutex_lock(&mt_buscarInodo);
		inodoADevolver = Inode_buscarLibre(ext2fs,sb,bgdt);
		Grupos_utilizarInodoEnBitmap(ext2fs,sb,bgdt,inodoADevolver);// Seteo el bitmap de inodos
	pthread_mutex_unlock(&mt_buscarInodo);

	return inodoADevolver;

}

int32_t Inode_asignarInodo(FILE *ext2fs,Superblock *sb,Group_descriptor *bgdt, uint16_t mode, uint16_t links) {

	int32_t nroInodo;
	Inode *inodo;
	size_t block_size = 1024 << sb->s_log_block_size;
	nroInodo = Inode_pedirInodoLibre(ext2fs,sb,bgdt);
	if(nroInodo == -1) return -1; //Si devolvio -1 no hay mas inodos
	inodo=Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size);
	inodo->i_links_count=links;
	inodo->i_mode= mode;
	Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size,inodo);
	free(inodo);
	return nroInodo;
}





