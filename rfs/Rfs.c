/*
 * Rfs.c
 *
 *  Created on: 14/06/2012
 *      Author: utnso
 */

#include "Rfs.h"

t_queue *colaPedidos;
pthread_mutex_t mt_cola = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_delay = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mt_lista_inodos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_buscarBloque = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_buscarInodo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_sb = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_bgdt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_cache = PTHREAD_MUTEX_INITIALIZER;

t_list* listaInodos;

sem_t pedidoEnCola;
__useconds_t delay=0;

memcached_st* st_cache;


t_socket_client *buscarFd(t_list* lista,int32_t fd) {

	int i,encontro;
	t_socket_client *cliente;

	encontro = 0;
	for(i=0;!encontro && i<lista->elements_count;i++) {
		cliente = (t_socket_client *)list_get(lista,i);
		if(cliente->socket->desc == fd) encontro = 1;
	}
	return cliente;
}

void eliminarFd(t_list* lista, int32_t fd) {

	int i,encontro;
	t_socket_client *cliente;

	encontro = 0;
	for(i=0;!encontro && i<lista->elements_count;i++) {
		cliente = (t_socket_client *)list_get(lista,i);
		if(cliente->socket->desc == fd) encontro = 1;
	}
	cliente = list_remove(lista,i-1);
	sockets_destroy_client(cliente);
}

Pedido *crearPedido(t_log *s_log,t_socket_client *client,void *buffer,char tipoPaquete, uint16_t payloadLength, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs) {

	Pedido *s_pedido;
	s_pedido = (Pedido *)malloc(sizeof(Pedido));
	s_pedido->buffer = malloc(payloadLength);
	s_pedido->log = s_log;
	s_pedido->client = client;
	memcpy(s_pedido->buffer,buffer,payloadLength);
	s_pedido->tipoPaquete = tipoPaquete;
	s_pedido->payloadLength = payloadLength;
	s_pedido->sb = sb;
	s_pedido->bgdt = bgdt;
	s_pedido->ext2fs = ext2fs;
	return s_pedido;

}

void procesarPedido(void *arg) { //Funcion del threadPool

	Pedido *s_pedido;
	FILE *ext2fsCopy;
	char *nombreArchivo = (char *)arg;
	ext2fsCopy = abrirArchivoExt2(nombreArchivo);

	while(1) {

		sem_wait(&pedidoEnCola);
		pthread_mutex_lock(&mt_cola);
		s_pedido = queue_pop(colaPedidos);
		pthread_mutex_unlock(&mt_cola);
		switch (s_pedido->tipoPaquete) {

			case TIPOGETATTR: {
				usleep(delay);
				procesarGetattr(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPOREADDIR: {
				usleep(delay);
				procesarReaddir(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPOREAD: {
				procesarRead(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy,s_pedido->payloadLength);
				break;
			}
			case TIPOOPEN: {
				usleep(delay);
				procesarOpen(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPOTRUNCATE: {
				usleep(delay);
				procesarTruncate(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy,s_pedido->payloadLength);
				break;
			}
			case TIPOWRITE: {
				usleep(delay);
				procesarWrite(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy,s_pedido->payloadLength);
				break;
			}
			case TIPOCREATE: {
				usleep(delay);
				procesarCreate(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPOUNLINK: {
				usleep(delay);
				procesarUnlink(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPOMKDIR: {
				usleep(delay);
				procesarMkdir(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			case TIPORMDIR: {
				usleep(delay);
				procesarRmdir(s_pedido->log,s_pedido->client,s_pedido->buffer,s_pedido->sb,s_pedido->bgdt,ext2fsCopy);
				break;
			}
			default:
				usleep(delay);
				free(s_pedido->buffer);
				break;
		}//FIN SWITCH
	free(s_pedido);
	}//FIN WHILE
}//FIN FUNCION


void procesarReaddir(t_log *s_log,t_socket_client *client,void *buffer,Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	uint32_t nroInodo;
	size_t block_size = 1024 << sb->s_log_block_size;
	t_list *listaDir;
	SerReadDir_resp respuesta;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito READDIR: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer; //payload = path
	nroInodo = Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
	}else{
		Sincro_monitorLock(nroInodo,listaInodos,TIPOREAD);
			listaDir = Dir_listar(ext2fs,nroInodo,sb->s_inodes_per_group,bgdt,block_size);
		Sincro_monitorUnlock(nroInodo,listaInodos);
		respuesta = serializar_readdir_Rta(listaDir);
		sockets_send(client,respuesta.paquete,respuesta.tamano+HEADERSIZE);
		list_destroy_and_destroy_elements(listaDir,(void *) Dir_destroy);
		free(respuesta.paquete);
	}
	free(path);
}

void procesarGetattr(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	uint32_t nroInodo;
	size_t block_size = 1024 << sb->s_log_block_size;
	Inode *inodo;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito GETATTR: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer; //payload = path
	nroInodo = Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
	}else{
		Sincro_monitorLock(nroInodo,listaInodos,TIPOREAD);
			inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size);
		Sincro_monitorUnlock(nroInodo,listaInodos);
		paqueteRta = (void *)serializar_Gettattr_Rta(inodo->i_mode,inodo->i_links_count,inodo->i_size);
		sockets_send(client,paqueteRta,sizeof(mode_t) + sizeof(nlink_t) + sizeof(off_t)+3);
		free(paqueteRta);
		free(inodo);
	}
	free(path);
}

void procesarRead(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength){

	Desread s_read;
	void *buffread;
	char *paqueteRta;
	uint32_t nroInodo;
	size_t block_size = 1024 << sb->s_log_block_size;
	Inode *inodo;

	s_read = deserializar_Read_Pedido(payloadLength,buffer);
	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito READ: %s - Offset: %d - Size: %d",client->socket->desc,s_read.path,s_read.offset, s_read.size);
	pthread_mutex_unlock(&mt_log);
	nroInodo = Dir_buscarPath(ext2fs,s_read.path,sb->s_inodes_per_group,bgdt,block_size);
	Sincro_monitorLock(nroInodo,listaInodos,TIPOREAD);
	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
		Sincro_monitorUnlock(nroInodo, listaInodos);
	}else{
		inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size);
		if(inodo->i_size < (s_read.offset+s_read.size)) {
			s_read.size=inodo->i_size-s_read.offset;
		}
		buffread = Dir_leerArchivo(ext2fs,inodo,s_read.offset,s_read.size,block_size);
		Sincro_monitorUnlock(nroInodo, listaInodos);
		paqueteRta = serializar_Read_Rta(s_read.size,buffread);
		sockets_send(client,paqueteRta,s_read.size+HEADERSIZE);
		free(inodo);
		free(buffread);
		free(s_read.path);
		free(paqueteRta);
	}
	free(buffer);
}


void procesarOpen(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	uint32_t nroInodo;
	size_t block_size = 1024 << sb->s_log_block_size;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito OPEN: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer;
	nroInodo = Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
	}else{
		paqueteRta = serializar_Result_Rta(0,TIPOOPEN);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		free(paqueteRta);

		//Sincro_agregarALista(nroInodo,listaInodos);

	}
	free(path);
}

void procesarTruncate(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength){

	Destrunc s_truncate;
	char *paqueteRta;

	int8_t error = 0;

	uint32_t nroInodo;
	size_t block_size = 1024 << sb->s_log_block_size;
	Inode *inodo;

	s_truncate = deserializar_Truncate_Pedido(payloadLength,buffer);
	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito TRUNCATE: %s offset: %d",client->socket->desc,s_truncate.path,s_truncate.offset);
	pthread_mutex_unlock(&mt_log);
	//*************************************************
	nroInodo = Dir_buscarPath(ext2fs,s_truncate.path,sb->s_inodes_per_group,bgdt,block_size);

	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
	}else{
		Sincro_monitorLock(nroInodo,listaInodos,TIPOWRITE);
			inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size);
			if(s_truncate.offset>inodo->i_size) { //Truncar arriba
				error = Inode_truncarArriba(ext2fs,inodo,sb,bgdt,s_truncate.offset,nroInodo);
			}

			if(s_truncate.offset<inodo->i_size) { //Truncar abajo
				Inode_truncarAbajo(ext2fs,inodo,sb,bgdt,s_truncate.offset,nroInodo);
			}
			if(error==0) { //no hubo error
				Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt, nroInodo,block_size,inodo);
			}
		Sincro_monitorUnlock(nroInodo,listaInodos);
		Superblock_escribir(ext2fs,sb);
		Grupos_escribirTabla(ext2fs,sb,bgdt);
		paqueteRta = serializar_Result_Rta(error,TIPOTRUNCATE);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		free(inodo);
		free(s_truncate.path);
		free(paqueteRta);
	}
	free(buffer);
}

void procesarWrite(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs,uint16_t payloadLength){

	Deswrite s_write;
	char *paqueteRta;
	uint32_t nroInodo;
	int32_t cantEscrita=0;
	size_t block_size = 1024 << sb->s_log_block_size;
	Inode *inodo;
	bool truncate=false;

	s_write = deserializar_Write_Pedido(payloadLength,buffer);
	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito WRITE: %s - Offset: %d - Size: %d",client->socket->desc,s_write.path, s_write.offset, s_write.size);
	pthread_mutex_unlock(&mt_log);
	nroInodo = Dir_buscarPath(ext2fs,s_write.path,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodo==0) { //el path no se encontro
		procesarError(client);
	}else{
		Sincro_monitorLock(nroInodo,listaInodos,TIPOWRITE);
			inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodo,block_size);
			if((s_write.offset+s_write.size)>inodo->i_size) truncate=true; //Se va a agrandar el archivo
			cantEscrita = Dir_escribirArchivo(ext2fs,inodo,s_write.offset,s_write.size,s_write.buf,sb,bgdt,nroInodo);
			if(cantEscrita!=-1 && truncate){//Si entra aca hubo truncate, sino entra no hubo espacio para agrandarlo o no se agrando
				Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt, nroInodo,block_size,inodo);
			}
			if(cantEscrita==-1) cantEscrita= -ENOSPC;
		Sincro_monitorUnlock(nroInodo,listaInodos);
		Superblock_escribir(ext2fs,sb);
		Grupos_escribirTabla(ext2fs,sb,bgdt);
		paqueteRta = serializar_Write_Rta(cantEscrita);
		sockets_send(client,paqueteRta,sizeof(int32_t)+HEADERSIZE);
		free(inodo);
		free(s_write.buf);
		free(s_write.path);
		free(paqueteRta);
	}
	free(buffer);
}


void procesarCreate(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	PathSeparado st_paths;
	uint32_t nroInodoDir,nroInodoCreado,error=1;
	int respuesta=0;
	size_t block_size = 1024 << sb->s_log_block_size;
	uint16_t mode = S_IFREG | 0444;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito CREATE: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer;

	st_paths = separarPath(path);
	//ver si ya existe
	nroInodoCreado = Inode_asignarInodo(ext2fs,sb,bgdt,mode, 1);
	if(nroInodoCreado != -1){
		nroInodoDir = Dir_buscarPath(ext2fs,st_paths.pathDir,sb->s_inodes_per_group,bgdt,block_size);
		error=nroInodoDir;
		if(error!=0) {
			Sincro_monitorLock(nroInodoDir,listaInodos,TIPOWRITE);
				if(Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size)!=0) {
					error=0; //Si entra aca el archivo ya existe, entonces aborta la creacion y devuelve el inodo
				}
				if(Dir_crearNuevaEntrada(ext2fs,st_paths.nombreArch,nroInodoDir,nroInodoCreado,sb,bgdt)==-1 && error!=0) {
					error=0; //Entra aca si el directorio contenedor fue borrado (libera el inodo y manda error)
				}
			Sincro_monitorUnlock(nroInodoDir,listaInodos);
		}
	}else{
		respuesta = -ENOSPC;
	}
	if(error==0) { //el path no se encontro
		Grupos_liberarInodoEnBitmap(ext2fs,sb,bgdt,nroInodoCreado);
		procesarError(client);
	}else{
		paqueteRta = serializar_Result_Rta(respuesta,TIPOCREATE);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		free(paqueteRta);
	}
	Superblock_escribir(ext2fs,sb);
	Grupos_escribirTabla(ext2fs,sb,bgdt);
	free(st_paths.pathDir);
	free(st_paths.nombreArch);
	free(path);
}

void procesarUnlink(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	PathSeparado st_paths;
	uint32_t nroInodoDir,nroInodoArch,error=1;
	Inode *inodo;
	int respuesta=0;
	size_t block_size = 1024 << sb->s_log_block_size;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito UNLINK: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer;

	st_paths = separarPath(path);
	nroInodoArch = Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size);
	nroInodoDir = Dir_buscarPath(ext2fs,st_paths.pathDir,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodoArch!=0 && nroInodoDir!=0) {
		Sincro_monitorLock(nroInodoDir,listaInodos,TIPOWRITE);
		Sincro_monitorLock(nroInodoArch,listaInodos,TIPOWRITE);
			inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoArch,block_size);
			if(inodo->i_mode != 0) {
				Dir_borrarEntrada(ext2fs,nroInodoDir,nroInodoArch,sb,bgdt);
				//modificar inodo archivo
				Inode_truncarAbajo(ext2fs,inodo,sb,bgdt,0,nroInodoArch);
				inodo->i_links_count=0;
				inodo->i_mode=0;
				Grupos_liberarInodoEnBitmap(ext2fs,sb,bgdt,nroInodoArch);
				Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoArch,block_size,inodo);
			}else{
				error=0;
			}
			free(inodo);
		Sincro_monitorUnlock(nroInodoArch,listaInodos);
		Sincro_monitorUnlock(nroInodoDir,listaInodos);
	}else {
		error=0;
	}

	if(error==0) { //el path no se encontro
		procesarError(client);
	}else{
		paqueteRta = serializar_Result_Rta(respuesta,TIPOUNLINK);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		Superblock_escribir(ext2fs,sb);
		Grupos_escribirTabla(ext2fs,sb,bgdt);
		free(paqueteRta);
	}
	free(st_paths.pathDir);
	free(st_paths.nombreArch);
	free(path);
}

void procesarMkdir(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	PathSeparado st_paths;
	uint32_t nroInodoDir,nroInodoCreado,error=1;
	int respuesta=0;
	size_t block_size = 1024 << sb->s_log_block_size;
	uint16_t mode = S_IFDIR | 0755;
	Inode* inodo;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito MKDIR: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer;

	st_paths = separarPath(path);
	nroInodoCreado = Inode_asignarInodo(ext2fs,sb,bgdt, mode, 2);
	if(nroInodoCreado != -1){
		nroInodoDir = Dir_buscarPath(ext2fs,st_paths.pathDir,sb->s_inodes_per_group,bgdt,block_size);
		error=nroInodoDir;
		if(error!=0) {
			Sincro_monitorLock(nroInodoDir,listaInodos,TIPOWRITE);
				if(Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size)!=0) {
					error=0; //Si entra aca el dir nuevo ya existe, entonces aborta la creacion y devuelve el inodo
				}
				if(Dir_crearNuevaEntrada(ext2fs,st_paths.nombreArch,nroInodoDir,nroInodoCreado,sb,bgdt)==-1 && error!=0) {
					error=0; //Entra aca si el directorio contenedor fue borrado (libera el inodo y manda error)
				}else{
					Dir_crearNuevaEntrada(ext2fs,".",nroInodoCreado,nroInodoCreado,sb,bgdt);
					Dir_crearNuevaEntrada(ext2fs,"..",nroInodoCreado,nroInodoDir,sb,bgdt);
					inodo = Inode_leerInodo(ext2fs, sb->s_inodes_per_group,bgdt,nroInodoDir,block_size);
					inodo->i_links_count ++;
					Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoDir,block_size,inodo);
					bgdt[nroInodoDir/sb->s_inodes_per_group].bg_used_dirs_count++;
				}
			Sincro_monitorUnlock(nroInodoDir,listaInodos);
		}
	}else{
		respuesta = -ENOSPC;
	}
	if(error==0) { //el path no se encontro
		Grupos_liberarInodoEnBitmap(ext2fs,sb,bgdt,nroInodoCreado);
		procesarError(client);
	}else{
		paqueteRta = serializar_Result_Rta(respuesta,TIPOMKDIR);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		free(paqueteRta);
	}
	Superblock_escribir(ext2fs,sb);
	Grupos_escribirTabla(ext2fs,sb,bgdt);
	free(path);
	free(st_paths.pathDir);
	free(st_paths.nombreArch);
}

void procesarRmdir(t_log *s_log,t_socket_client *client,void *buffer, Superblock *sb, Group_descriptor *bgdt,FILE *ext2fs){

	void *path;
	char *paqueteRta;
	PathSeparado st_paths;
	uint32_t nroInodoDir,nroInodoArch,error=1;
	Inode *inodo;
	int respuesta=0;
	size_t block_size = 1024 << sb->s_log_block_size;

	pthread_mutex_lock(&mt_log);
		log_debug(s_log,"El Cliente ID: %d solicito RMDIR: %s",client->socket->desc,(char *)buffer);
	pthread_mutex_unlock(&mt_log);
	path = (char *)buffer;

	st_paths = separarPath(path);
	nroInodoArch = Dir_buscarPath(ext2fs,path,sb->s_inodes_per_group,bgdt,block_size);
	nroInodoDir = Dir_buscarPath(ext2fs,st_paths.pathDir,sb->s_inodes_per_group,bgdt,block_size);
	if(nroInodoArch!=0 && nroInodoDir!=0) {
		Sincro_monitorLock(nroInodoDir,listaInodos,TIPOWRITE);
		Sincro_monitorLock(nroInodoArch,listaInodos,TIPOWRITE);
			inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoArch,block_size);
			if(Dir_cantDirectorios(inodo,ext2fs,sb->s_inodes_per_group,bgdt,block_size) == 2){
				Dir_borrarEntrada(ext2fs,nroInodoDir,nroInodoArch,sb,bgdt);
				//modificar inodo archivo
				Inode_truncarAbajo(ext2fs,inodo,sb,bgdt,0,nroInodoArch);
				inodo->i_links_count=0;
				inodo->i_mode=0;
				Grupos_liberarInodoEnBitmap(ext2fs,sb,bgdt,nroInodoArch);
				Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoArch,block_size,inodo);
				free(inodo);
				inodo = Inode_leerInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoDir,block_size);
				inodo->i_links_count --;
				Inode_escribirInodo(ext2fs,sb->s_inodes_per_group,bgdt,nroInodoDir,block_size,inodo);
				free(inodo);
				bgdt[nroInodoArch/sb->s_inodes_per_group].bg_used_dirs_count--;
			}else{
				respuesta = -ENOTEMPTY;
				if(inodo->i_mode == 0) {
					respuesta = -ENOENT;
				}
				free(inodo);
			}
		Sincro_monitorUnlock(nroInodoArch,listaInodos);
		Sincro_monitorUnlock(nroInodoDir,listaInodos);
	}else {
		error=0;
	}

	if(error==0) { //el path no se encontro
		procesarError(client);
	}else{
		paqueteRta = serializar_Result_Rta(respuesta,TIPORMDIR);
		sockets_send(client,paqueteRta,sizeof(int)+HEADERSIZE);
		free(paqueteRta);
		Superblock_escribir(ext2fs,sb);
		Grupos_escribirTabla(ext2fs,sb,bgdt);
	}
	free(path);
}

void procesarError(t_socket_client *client) {

	char *paqueteRta;
	paqueteRta = (void *)serializar_error();
	sockets_send(client,paqueteRta,HEADERSIZE);
	free(paqueteRta);
}

void levantarCfg(char **ip, int32_t *port,t_log_level *log_level, uint8_t *maxThreads, char **nombreArchivo, char** ipCache, int32_t *portCache) {

	char *log_level_str;
	t_config *s_cfg = config_create("cfgRfs.txt");

	if(config_has_property(s_cfg,"port")) {
		*port = config_get_int_value(s_cfg,"port");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"ip")) {
		*ip = config_get_string_value(s_cfg,"ip");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"log")) {
		log_level_str = config_get_string_value(s_cfg,"log");
		*log_level = log_level_from_string(log_level_str);
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"hilos")) {
		 *maxThreads = config_get_int_value(s_cfg,"hilos");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"archivo")) {
		*nombreArchivo = config_get_string_value(s_cfg,"archivo");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"delay")) { //delay var global
		delay = (config_get_int_value(s_cfg,"delay"))*1000;
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"ipCache")) { //delay var global
		*ipCache  = config_get_string_value(s_cfg,"ipCache");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
	if(config_has_property(s_cfg,"portCache")) { //delay var global
		*portCache = config_get_int_value(s_cfg,"portCache");
	}else{
		puts("Faltan campos en el cfg");
		exit(0);
	}
}

void actualizarDelay(int fd) {

	char buffer[1024];
	read( fd, buffer, 1024 ); //leo para q el select no entre otra vez

	t_config *s_cfg = config_create("cfgRfs.txt");
	if(config_has_property(s_cfg,"delay")) {
		delay = (config_get_int_value(s_cfg,"delay"))*1000;
	}
}

void crearThreads(uint8_t maxThreads, char *arg) {

	int i;
	pthread_t tid;
	pthread_attr_t th_attr;

	pthread_attr_init(&th_attr);
	pthread_attr_setdetachstate(&th_attr,PTHREAD_CREATE_DETACHED);

	for(i=0;i<maxThreads;i++) {
		if(pthread_create(&tid,&th_attr,(void*)&procesarPedido,arg) == -1) {
			perror("Error al crear thread");
			exit(0);
		}
	}
	pthread_attr_destroy(&th_attr);

}

FILE *abrirArchivoExt2(char *nombre) {

	FILE *ext2fs;
	char *cwd;
	int32_t fd;

	cwd = getcwd(NULL,255);
	if((ext2fs = fopen(nombre,"r+")) == NULL) {
		puts("Archivo no encontrado...\n");
		printf("Tienen que crear el archivo ext2 en \"%s\" ...\n\n", cwd);
		exit(0);
	}
	free(cwd);
	setvbuf(ext2fs,NULL,_IONBF,0);//BUFFER DESACTIVADO
	//Convierto stream a descriptor para hacer el fadvise
	if((fd = fileno(ext2fs))== -1) {
		puts("Error en funcion fileno");
		exit(0);
	}
	posix_fadvise(fd,0,0,POSIX_FADV_RANDOM);

	return ext2fs;
}

PathSeparado separarPath(char *path){

	uint16_t i=0;
	size_t len=strlen(path);
	PathSeparado st_ps;
	for(i=len;path[i]!='/';i--);
	if(i==0){ //es el dir raiz
		st_ps.pathDir=malloc(2);
		st_ps.pathDir[0]='/';
		st_ps.pathDir[1]='\0';
		st_ps.nombreArch=malloc(len-i);
		strncpy(st_ps.nombreArch,path+i+1,len-i-1);
		st_ps.nombreArch[len-i-1]='\0';
	}else{
		st_ps.pathDir=malloc(i+1);
		st_ps.nombreArch=malloc(len-i);
		strncpy(st_ps.pathDir,path,i);
		st_ps.pathDir[i]='\0';
		strncpy(st_ps.nombreArch,path+i+1,len-i-1);
		st_ps.nombreArch[len-i-1]='\0';
	}
	return st_ps;
}
