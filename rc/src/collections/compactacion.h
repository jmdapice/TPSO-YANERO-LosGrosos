/*
 * compactacion.h
 *
 *  Created on: 17/05/2012
 *      Author: Florencia Reales
 */

#ifndef COMPACTACION_H_
#define COMPACTACION_H_
#include "logRc.h"

bool compactar(t_table_info* self, int pos,t_log* logger,int frecuencia);
int compactacion(t_table_info* self, int frecuencia, int size_data,
		char* alg_busq, char* alg_reemp, t_log* logger);
int first_fit(t_table_info* self, int size_data);
int best_fit(t_table_info* self, int size_data);
void dump_particionesDinamicas(t_table_info* self, int table_index,FILE*);




#endif /* COMPACTACION_H_ */
