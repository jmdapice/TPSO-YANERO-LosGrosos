/*
 * Inotify.c
 *
 *  Created on: 19/06/2012
 *      Author: utnso
 */


#include "Inotify.h"

int Inotify_iniciar(char *path) {

	int file_descriptor = inotify_init();

	if (file_descriptor < 0) {
		perror("inotify_init");
		exit(0);
	}

	//char *cwd;
	//cwd = getcwd(NULL,255);
	//inotify_add_watch(file_descriptor, cwd, IN_MODIFY);

	inotify_add_watch(file_descriptor, path, IN_MODIFY);

	return file_descriptor;
}
