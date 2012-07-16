/*
 * Copyright (C) 2012 Sistemas Operativos - UTN FRBA. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include "bitarray.h"

/*
 * @NAME: bitarray_create
 * @DESC: Crea y devuelve un puntero a una estructura t_bitarray
 * @PARAMS:
 * 		bitarray
 *		size - Tamaño en bytes del bit array
 */
t_bitarray *bitarray_create(char *bitarray, size_t size) {
	t_bitarray *self = malloc(sizeof(t_bitarray));

	self->bitarray = bitarray;
	self->size = size;

	return self;
}

/*
 * @NAME: bitarray_test_bit
 * @DESC: Devuelve el valor del bit de la posicion indicada
 */
bool bitarray_test_bit(t_bitarray *self, off_t bit_index) {
	return((self->bitarray[BIT_CHAR(bit_index)] & BIT_IN_CHAR(bit_index)) != 0);
}

/*
 * @NAME: bitarray_set_bit
 * @DESC: Setea el valor del bit de la posicion indicada
 */
void bitarray_set_bit(t_bitarray *self, off_t bit_index) {
	self->bitarray[BIT_CHAR(bit_index)] |= BIT_IN_CHAR(bit_index);
}

/*
 * @NAME: bitarray_clean_bit
 * @DESC: Limpia el valor del bit de la posicion indicada
 */
void bitarray_clean_bit(t_bitarray *self, off_t bit_index){
    unsigned char mask;

    /* create a mask to zero out desired bit */
    mask =  BIT_IN_CHAR(bit_index);
    mask = ~mask;

    self->bitarray[BIT_CHAR(bit_index)] &= mask;
}

/*
 * @NAME: bitarray_get_max_bit
 * @DESC: Devuelve la cantidad de bits en el bitarray
 */
size_t bitarray_get_max_bit(t_bitarray *self) {
	return self->size * CHAR_BIT;
}

/*
 * @NAME: bitarray_destroy
 * @DESC: Destruye el bit array
 */
void bitarray_destroy(t_bitarray *self) {
	free(self->bitarray);
	free(self);
}

int32_t bitarray_buscarPosicionLibre(t_bitarray *self){

	int32_t pos;
	for(pos=0;bitarray_test_bit(self,pos) && pos<(self->size * 8);pos++);
	if(pos == self->size*8) return -1; // Devuelve -1 si el bitmap esta lleno
	return pos; //Retorno Numero de Inodo Libre dentro del Grupo nroGrupo
}

/*
uint32_t bitarray_posic (const uint32_t nroBit) {

	uint32_t byte, bitsignif, bit_teorico, pos_bitarray;

	byte = nroBit / 8; //Obtengo el byte del Bitarray
	bitsignif = byte * 8;
	bit_teorico = nroBit % 8;
	pos_bitarray = bitsignif + 7 - bit_teorico; // 7 es el bit menos significativo del byte

	return pos_bitarray;
} */


