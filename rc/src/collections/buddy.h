/*
 * buddy.h
 *
 *  Created on: 01/07/2012
 *      Author: utnso
 */

#ifndef BUDDY_H_
#define BUDDY_H_
#include "logRc.h"

int buddysystem(t_table_info* self, int nbytes_data, char* alg_reemp,t_log* logger );
void buddy_create_element(t_table_info *self,const void* key,size_t size_key,int pos,size_t size_data);
void dump_buddy(t_table_info* self, int table_index,FILE*);

#endif /* BUDDY_H_ */
