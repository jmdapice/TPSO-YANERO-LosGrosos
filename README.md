TPSO-YANERO-LosGrosos
=====================

Para compilar por consola se hace un checkout completo con "svn checkout"

Se entra en la carpeta debug de cada proyecto y se tira el comando "make"

El makefile de la rc apunta a los include de memcached en /home/utnso/memcached-1.6 si la ponen en otro lado hay q cambiar el makefile

Para compilar y correr en eclipse hay que agregar los siguientes parametros:

*Fsc:

-lpthread -lfuse -lmemcached -DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64

argumentos carpeta_a_montar -f (para que muestre el log) -s (un solo thread)

archivo de configuracion donde va a correr 

se crea con el comando: 

touch ext2.disk # Crear el archivo vacío
mkfs.ext2 -F -O none,sparse_super -b 1024 ext2.disk 30000 # Formatearlo

*Rfs:

-lpthread -lmemcached

archivo de configuracion donde va a correr

archivo de disco ext2 seteado en archivo de configuracion



*Rc:

configurar el proyecto como una shared library

-I path_a_memcached-1.6/include -lpthread

para ejecutar se pone el archivo de configuracion en la carpeta de memcached-1.6, se compila memcached y se corre apuntado como engine a la libreria compilada

