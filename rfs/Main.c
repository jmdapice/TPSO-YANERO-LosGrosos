
#include "Rfs.h"
#include "Inotify.h"

extern t_queue *colaPedidos;
extern pthread_mutex_t mt_cola;
extern sem_t pedidoEnCola;
extern t_list* listaInodos;
extern pthread_mutex_t mt_log;
extern memcached_st* st_cache;

int main(void) {

	fd_set master,copy;
	FILE *ext2fs;
	int32_t i,maxfd,bytesRecv,port,inotFd;
	char tipoPaquete;
	char *buffer;
	char *ip=NULL;
	char *ipCache = NULL;
	int32_t portCache;
	char *nombreArchivo=NULL;
	uint8_t maxThreads;
	uint16_t payloadLength;
	Group_descriptor *bgdt;
	Superblock sb;
	t_log_level log_level;
	t_socket_server *server;
	t_socket_client *client;
	t_list *listaClientes;
	t_log *s_log;
	t_config *s_cfg;
	Pedido *s_pedido;


	listaInodos = list_create(); //Creo la lista que va a contener los semaforos de inodos

	levantarCfg(&ip,&port,&log_level,&maxThreads,&nombreArchivo,&ipCache,&portCache);
	inotFd = Inotify_iniciar("cfgRfs.txt");
	s_log = log_create("logRfs.txt","rfs",true,log_level);
	ext2fs = abrirArchivoExt2(nombreArchivo);
	sb = Superblock_leer(ext2fs);
	bgdt=Grupos_leerTabla(ext2fs,sb);
	colaPedidos = queue_create(); //Creo cola para los pedidos (la cola es global)
	sem_init(&pedidoEnCola,0,0);
	st_cache = cache_crearCacheServer(ipCache,portCache);
	crearThreads(maxThreads,nombreArchivo);
	server = sockets_createServer(ip,port);

	if(sockets_listen(server) == -1) {
		perror("Listen");
		exit(0);
	}

	FD_ZERO(&master);
	FD_SET(server->socket->desc,&master);
	FD_SET(inotFd,&master);

	if(inotFd<server->socket->desc)
		maxfd = server->socket->desc;
	else
		maxfd = inotFd;

	//SERVER ON
	pthread_mutex_lock(&mt_log);
		log_info(s_log,"Servidor escuchando en el puerto %d",port);
	pthread_mutex_unlock(&mt_log);
	listaClientes = list_create();

	while(1) {

		copy = master; //trabajo con una copia del set asi el select no rompe el master
		if(select(maxfd+1,&copy,NULL,NULL,NULL) == -1) {
			perror("Select");
			exit(0);
		}

		for(i=0;i<=maxfd;i++) {
			if(FD_ISSET(i,&copy)) {
				if(i==inotFd) { //cambio en archivo de cfg
					actualizarDelay(i);
					continue;
				}
				if(i==server->socket->desc) { //entro cliente nuevo
					client = sockets_accept(server);
					if(client == NULL) {
						perror("Error en accept");
						continue;
					}
					if(recibirHandshake(client) <=0) {
						sockets_destroy_client(client);
						continue;
					}
					pthread_mutex_lock(&mt_log);
						log_info(s_log,"Nuevo Cliente ID: %d",client->socket->desc);
					pthread_mutex_unlock(&mt_log);
					list_add(listaClientes,client);
					FD_SET(client->socket->desc,&master);
					if(client->socket->desc > maxfd) maxfd = client->socket->desc;
				}else { //entro un recv en un cliente
					client = buscarFd(listaClientes,i);
					buffer = sockets_createBuffer(HEADERSIZE);
					bytesRecv = sockets_recvHeaderNipc(client,buffer);
					if (bytesRecv <= 0) {
						if (bytesRecv == -1) { //si es -1 es error, si es 0 cerro el sock
							pthread_mutex_lock(&mt_log);
								log_info(s_log,"El Cliente ID: %d produjo un error en el recv",client->socket->desc);
							pthread_mutex_unlock(&mt_log);
							//perror("recv");
						}
						pthread_mutex_lock(&mt_log);
							log_info(s_log,"El Cliente ID: %d se ha desconectado",client->socket->desc);
						pthread_mutex_unlock(&mt_log);
						eliminarFd(listaClientes,i);
						FD_CLR(i, &master);
						continue;
					}
					tipoPaquete = buffer[0];
					memcpy(&payloadLength,buffer+1,sizeof(uint16_t));
					sockets_destroyBuffer(buffer);
					buffer = sockets_createBuffer(payloadLength);
					bytesRecv = sockets_recvPayloadNipc(client,buffer,payloadLength);
					if (bytesRecv <= 0) {
						if (bytesRecv == -1) { //si es -1 es error, si es 0 cerro el sock
							pthread_mutex_lock(&mt_log);
								log_info(s_log,"El Cliente ID: %d se ha caido",client->socket->desc);
							pthread_mutex_unlock(&mt_log);
							//perror("recv");
						}
						pthread_mutex_lock(&mt_log);
							log_info(s_log,"El Cliente ID: %d se ha desconectado",client->socket->desc);
						pthread_mutex_unlock(&mt_log);
						eliminarFd(listaClientes,i);
						FD_CLR(i, &master);
						continue;
					}
					//armar struct para la cola (la struct tiene todos los parametros que tenia procesarPedido)
					s_pedido = crearPedido(s_log,client,buffer,tipoPaquete,payloadLength,&sb,bgdt,ext2fs);
					pthread_mutex_lock(&mt_cola);
						queue_push(colaPedidos,s_pedido);
					pthread_mutex_unlock(&mt_cola);
					sem_post(&pedidoEnCola);
					sockets_destroyBuffer(buffer); //destruyo el buffer pq crearPedido lo copia
				} //FIN IF CENTRAL
			}//FIN IF ISSET
		}//FIN FOR
	}//FIN WHILE(1)

	config_destroy(s_cfg);
	return EXIT_SUCCESS;

}
