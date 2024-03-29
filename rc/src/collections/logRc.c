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

#include "logRc.h"
#include "../../../commons/string.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/timeb.h>

#ifndef LOG_MAX_LENGTH_MESSAGE
#define LOG_MAX_LENGTH_MESSAGE 1024
#endif

#define LOG_MAX_LENGTH_BUFFER LOG_MAX_LENGTH_MESSAGE + 100
#define LOG_ENUM_SIZE 5

static char *enum_names[LOG_ENUM_SIZE] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR"};

/**
 * Private Functions
 */
static void logRc_write_in_level(t_log* logger, t_log_level level, const char* message_template, va_list arguments);
static bool isEnableLevelInLoggerRc(t_log* logger, t_log_level level);

/**
 * Public Functions
 */


/**
 * @NAME: log_create
 * @DESC: Crea una instancia de logger, tomando por parametro
 * el nombre del programa, el nombre del archivo donde se van a generar los logs,
 * el nivel de detalle minimo a loguear y si además se muestra por pantalla lo que se loguea.
 */
t_log* logRc_create(char* file, char *program_name, bool is_active_console, t_log_level detail) {
	t_log* logger = malloc(sizeof(t_log));

	if (logger == NULL) {
		perror("Cannot create logger");
		return NULL;
	}

	FILE *file_opened = NULL;

	if (file != NULL) {
		file_opened = fopen(file, "a");

		if (file_opened == NULL) {
			perror("Cannot create/open log file");
			free(logger);
			return NULL;
		}
	}

	logger->file = file_opened;
	logger->is_active_console = is_active_console;
	logger->detail = detail;
	logger->pid = getpid();
	logger->program_name = strdup(program_name);
	return logger;
}


/**
 * @NAME: log_destroy
 * @DESC: Destruye una instancia de logger
 */
void logRc_destroy(t_log* logger) {
//	free(logger->program_name);
//	fclose(logger->file);
//	free(logger);
}



/**
 * @NAME: log_debug
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [DEBUG] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void logRc_debug(t_log* logger, const char* message_template, ...) {
	va_list arguments;
	va_start(arguments, message_template);
	logRc_write_in_level(logger, LOG_LEVEL_DEBUG, message_template, arguments);
	va_end(arguments);
}

/**
 * @NAME: log_info
 * @DESC: Loguea un mensaje con el siguiente formato
 *
 * [INFO] hh:mm:ss:mmmm PROCESS_NAME/(PID:TID): MESSAGE
 *
 */
void logRc_info(t_log* logger, const char* message_template, ...) {
	va_list arguments;
	va_start(arguments, message_template);
	logRc_write_in_level(logger, LOG_LEVEL_INFO, message_template, arguments);
	va_end(arguments);
}



/**
 * @NAME: log_level_as_string
 * @DESC: Convierte un t_log_level a su representacion en string
 */
char *logRc_level_as_string(t_log_level level) {
	return enum_names[level];
}

/**
 * @NAME: log_level_from_string
 * @DESC: Convierte un string a su representacion en t_log_level
 */
t_log_level logRc_level_from_stringRc(char *level) {
	int i;

	for (i = 0; i < LOG_ENUM_SIZE; i++) {
		if (string_equals_ignore_case(level, enum_names[i])){
			return i;
		}
	}

	return -1;
}

/** PRIVATE FUNCTIONS **/

static void logRc_write_in_level(t_log* logger, t_log_level level, const char* message_template, va_list list_arguments) {

	if (isEnableLevelInLoggerRc(logger, level)) {
		char message[LOG_MAX_LENGTH_MESSAGE + 1];
		char buffer[LOG_MAX_LENGTH_BUFFER + 1];
		unsigned int thread_id;


		vsprintf(message, message_template, list_arguments);

		time_t log_time;
		struct tm *log_tm;
		struct timeb tmili;
		char str_time[100];
		strcpy(str_time,"hh:mm:ss:mmmm");
		log_time = time(NULL);
		log_tm = localtime(&log_time);
		ftime(&tmili);
		char partial_time[100];
		strcpy(partial_time,"hh:mm:ss");

		strftime(partial_time, 127, "%H:%M:%S", log_tm);

		sprintf(str_time, "%s:%hu", partial_time, tmili.millitm);


		thread_id = pthread_self();


		sprintf(buffer, "[%s] %s %s/(%d:%d): %s\n",
				logRc_level_as_string(level), str_time,
				logger->program_name, logger->pid, thread_id,
				message);

		if (logger->file != NULL) {
			fprintf(logger->file, "%s", buffer);
			fflush(logger->file);
		}

		if (logger->is_active_console) {
			printf("%s", buffer);
			fflush(stdout);
		}

	}
}

static bool isEnableLevelInLoggerRc(t_log* logger, t_log_level level) {
	return level >= logger->detail;
}
