/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h> // Para atoi() y exit()

// === INICIO AMPLIACIÓN EJ3 (Librerías extra necesarias) ===
// #include <ctype.h> 
// #include <math.h>
// #include <dirent.h>
// #include <stdio.h>
// === FIN AMPLIACIÓN EJ3 ===

//-- PARA EJ 4 -> isdigit() (saber si es numero o no)--//
// #include <ctype.h>

job* lista; //esta variable tiene que ser global por narices para que los manejadores asincronos la vean

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// === INICIO AMPLIACIÓN EJ1 (Manejador SIGHUP) ===
// void manejador_SIGHUP(int signal){
// 	FILE *fp;
// 	fp=fopen("hup.txt","a"); 
// 	if (fp) { 
// 		fprintf(fp, "SIGHUP recibido.\n"); 
// 		fclose(fp);
// 	}
// }
// === FIN AMPLIACIÓN EJ1 ===


// === INICIO AMPLIACIÓN EJ3 (Comando zjobs - Función traverse_proc) ===
// /* NOTA PARA EXAMEN: Esta es la versión corregida que da el 10/10 (con filtro ppid y isdigit) */
// void traverse_proc(void) {
//     DIR *d; 
//     struct dirent *dir;
//     char ruta[512];
//     char nombre[512]; 
//     d = opendir("/proc");
//     if (d) {
//         while ((dir = readdir(d)) != NULL) {
//             if (dir->d_name[0] < '0' || dir->d_name[0] > '9') continue; //filtramos para no abrir carpetas raras del sistema
//             sprintf(ruta, "/proc/%s/stat", dir->d_name); 
//             FILE *fd = fopen(ruta, "r");
//             if (fd){
//                 long pid;     
//                 long ppid;    
//                 char state;   
//                 if(fscanf(fd, "%ld %s %c %ld", &pid, nombre, &state, &ppid)==4){
// 					if(state=='Z' && ppid == getpid()){ //clave aqui: mirar si es zombie y si somos su padre real
// 						printf("%ld\n",pid);
// 					}	
// 				}
//                 fclose(fd);
//             }
//         }
//         closedir(d);
//     }
// }
// === FIN AMPLIACIÓN EJ3 ===


void registrar_tarea_background(int pid,char* comando){
	block_SIGCHLD(); //bloqueamos señales para evitar movidas de concurrencia si muere alguien justo ahora
	add_job(lista,new_job(pid,comando,BACKGROUND));
	unblock_SIGCHLD();
}

/* MANEJADOR BASE ORIGINAL (Descomentado y activo) */
void manejador_tarea_zombie(int signum){
	int pid_hijo;
	int status;
	char* estado;
	//el -1 es la aspiradora: pilla a cualquier hijo. WNOHANG hace que no se quede colgado esperando
	//usamos un while y no un if por si se nos mueren 3 hijos a la vez, para recogerlos todos del tiron
	while((pid_hijo=waitpid(-1,&status,WUNTRACED|WNOHANG))>0){
		block_SIGCHLD(); 
		job* job_manejado=get_item_bypid(lista,pid_hijo);
		if (WIFEXITED(status)) { //si muere de forma natural (ha terminado su codigo)
			delete_job(lista,job_manejado);
        } 
        else if (WIFSIGNALED(status)) { //si le pegan un tiro (ejemplo kill -9)
			delete_job(lista,job_manejado);	
		} 
        else if (WIFSTOPPED(status)) {
        	//aqui no hacemos nada porque si se para por ctrl+z ya lo gestiona el waitpid del padre abajo
        }
		unblock_SIGCHLD();
	}
}

// === INICIO AMPLIACIÓN EJ3 (Manejador alternativo para zjobs) ===
// /* NOTA PARA EXAMEN: Si te piden el zjobs, tienes que COMENTAR el manejador_tarea_zombie 
//    original de arriba y DESCOMENTAR este, que pasa lista en vez de usar -1 */
// void manejador_tarea_zombie_EJ3(int signum){
// 	int pid_hijo;
// 	int status;
// 	
// 	block_SIGCHLD();
// 	int i=1;
// 	while(i <= list_size(lista)){ //pasamos lista uno a uno solo a los que tenemos apuntados
// 		job* tarea=get_item_bypos(lista,i);
// 
// 		if(tarea!=NULL){
// 			pid_hijo = waitpid(tarea->pgid, &status, WUNTRACED | WNOHANG);
//             
//             if (pid_hijo > 0) { 
//                 if (WIFEXITED(status) || WIFSIGNALED(status)) {
//                     delete_job(lista, tarea);
//                     continue; //ojo, como borramos el plato, el de arriba cae a esta misma posicion
//                 }
//             }
// 		}
// 		i++;
// 	}
// 	unblock_SIGCHLD();
// }
// === FIN AMPLIACIÓN EJ3 ===


// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; 
	int background;             
	char *args[MAX_LINE/2];     
	
	int pid_fork, pid_wait; 	
	int status;             	
	char *file_in, *file_out; 	
	int info;
	char* estado;
	int gpid;	
	int gpid_hijo;
	int gpid_padre;
	char* nombre_lista="Lista de tareas";
	lista=new_list(nombre_lista);

	// === INICIO AMPLIACIÓN EJ1 ===
	// signal(SIGHUP,manejador_SIGHUP);
	// === FIN AMPLIACIÓN EJ1 ===

	signal(SIGCHLD, manejador_tarea_zombie); //enchufamos el manejador al arrancar el programa
	// NOTA: Si usas la ampliación 3, cambia manejador_tarea_zombie por manejador_tarea_zombie_EJ3 en la línea anterior.

	while (1)   
	{   	
		
		ignore_terminal_signals();
		printf("COMMAND->");
		fflush(stdout); //vaciamos el buffer a la fuerza para que el print salga en pantalla si o si
		get_command(inputBuffer, MAX_LINE, args, &background);  
		

		char *file_in,*file_out;
		
		parse_redirections(args,&file_in,&file_out); //esta funcion ya nos busca si el usuario puso < o >

		// === INICIO AMPLIACIÓN EJ5 (Redirección Append >>) PARTE 1 ===
		// //IMPORTANTE: Esto tiene que ir SIEMPRE debajo del parse_redirections original 
		// //para que no nos machaque la variable file_out poniendola a NULL.
		// int es_append = 0;
		// if (args[0] != NULL) {
		// 	for (int i = 0; args[i] != NULL; i++) {
		// 		if (strcmp(args[i], ">>") == 0) {
		// 			es_append = 1; 
		// 			file_out = args[i + 1]; 
		// 			
		// 			int j = i;
		// 			while (args[j + 2] != NULL) {
		// 				args[j] = args[j + 2];
		// 				j++;
		// 			}
		// 			args[j] = NULL; 
		// 			args[j + 1] = NULL;
		// 			break; 
		// 		}
		// 	}
		// }
		// === FIN AMPLIACIÓN EJ5 PARTE 1 ===
		
		if(args[0]==NULL) continue;   //evitamos que pete si el usuario le da a enter sin escribir nada


		// === INICIO AMPLIACIÓN EJ7 (Comando lanzabg) ===
		// if(strcmp(args[0], "lanzabg") == 0) {
		// 	//forzamos que vaya en segundo plano (simulamos el ampersand)
		// 	background = 1; 
		// 	
		// 	//desplazamos todo el array hacia la izquierda para machacar la palabra "lanzabg"
		// 	//asi el shell sigue funcionando normal creyendo que escribieron solo el comando a lanzar
		// 	int i = 0;
		// 	while(args[i + 1] != NULL) {
		// 		args[i] = args[i + 1];
		// 		i++;
		// 	}
		// 	args[i] = NULL;
		// 	
		// 	//si el usuario solo escribio "lanzabg" y dio enter, petaria, asi que lo reiniciamos
		// 	if (args[0] == NULL) continue;
		// }
		// === FIN AMPLIACIÓN EJ7 ===


		// === INICIO AMPLIACIÓN EJ6 (Comando exit) ===
		if (strcmp(args[0], "exit") == 0) {
			int exit_status = 0;
			
			//si hay un argumento despues de exit...
			if (args[1] != NULL) {
				//atoi es magico: si le pasas "123" te da 123, si le pasas "abc" o basura te da 0
				//que es exactamente lo que pide el guion para fallos
				exit_status = atoi(args[1]);
			}
			
			//la funcion exit de C termina el programa devolviendo ese valor al sistema operativo
			exit(exit_status);
		}
		// === FIN AMPLIACIÓN EJ6 ===

		
		// === INICIO AMPLIACIÓN EJ3 (Comando zjobs) ===
		// if(strcmp(args[0],"zjobs")==0){
		// 	block_SIGCHLD();
		// 	traverse_proc();
		// 	unblock_SIGCHLD();
		// 	continue;
		// }
		// === FIN AMPLIACIÓN EJ3 ===

		// === INICIO AMPLIACIÓN EJ3 (Comando deljob) ===
		// if(strcmp(args[0],"deljob")==0) {
		// 	block_SIGCHLD();
		// 	job* tarea=get_item_bypos(lista,1);
		// 	if (tarea == NULL) {
		// 		printf("No hay trabajo actual\n");
		// 		unblock_SIGCHLD();
		// 		continue;
		// 	}else{
		// 		if(tarea->state==BACKGROUND){
		// 			printf("Borrando trabajo actual de la lista de jobs: PID=%d command=%s\n", tarea->pgid, tarea->command);
		// 			delete_job(lista,tarea); //importante el orden: primero imprimimos, despues liberamos memoria
		// 			unblock_SIGCHLD();
		// 			continue;
		// 		}
		// 		if(tarea->state==STOPPED){
		// 			printf("No se permiten borrar trabajos en segundo plano suspendidos\n");
		// 			unblock_SIGCHLD();
		// 			continue;
		// 		}
		// 	}
		// }
		// === FIN AMPLIACIÓN EJ3 ===

		// === INICIO AMPLIACIÓN EJ4 (Comando bgteam) ===
		// if(strcmp(args[0],"bgteam")==0){
		// 	//comprobamos que al menos haya escrito bgteam N comando
		// 	if(args[1]==NULL || args[2]==NULL){
		// 		printf("El comando bgteam requiere dos argumentos\n");
		// 		continue;
		// 	}
		// 
		// 	//validacion "purista" sin usar break para el examen
		// 	int es_valido = 1;
		// 	int i = 0;
		// 	//el bucle sigue mientras no lleguemos al final de la palabra Y siga siendo un numero valido
		// 	while (args[1][i] != '\0' && es_valido == 1) {
		// 		if (!isdigit(args[1][i])) {
		// 			es_valido = 0; //al ponerlo a 0, la condicion del while falla y el bucle termina naturalmente
		// 		}
		// 		i++;
		// 	}
		// 	
		// 	if (es_valido == 0) continue; //ignoramos silenciosamente
		// 
		// 	//pasamos el texto a numero de verdad
		// 	int n_jobs = atoi(args[1]);
		// 	if (n_jobs <= 0) continue; //por si acaso es 0, ignoramos tambien
		// 
		// 	//arrancamos la fabrica de hacer forks
		// 	for (int j = 0; j < n_jobs; j++) {
		//	//literalmente un copypaste del tratamiento de procesos que hacemos
		// 		int pid_hijo_bg = fork();
		// 
		// 		if (pid_hijo_bg == 0) {
		// 			//--- zona del hijo clonado ---
		// 			setpgid(0, 0); 
		// 			restore_terminal_signals(); 
		// 			
		// 			execvp(args[2], &args[2]);
		// 			
		// 			printf("Error, command not found: %s\n", args[2]);
		// 			exit(-1);
		// 		} 
		// 		else if (pid_hijo_bg > 0) {
		// 			//--- zona del padre shell ---
		// 			setpgid(pid_hijo_bg, 0); 
		// 			registrar_tarea_background(pid_hijo_bg, args[2]); 
		// 			printf("Background job running... pid: %d,command: %s\n", pid_hijo_bg, args[2]);
		// 		}
		// 	}
		// 	continue; 
		// }
		// === FIN AMPLIACIÓN EJ4 ===

		// === INICIO AMPLIACIÓN EJ2 (Comando currjob) ===
		// if(strcmp(args[0],"currjob")==0){
		// 	job* ultima_tarea=get_item_bypos(lista,1);
		// 	if(ultima_tarea != NULL) {
		// 		printf("Trabajo actual: PID=%d command=%s\n",ultima_tarea->pgid,ultima_tarea->command);
		// 	} else {
		// 		printf("No hay trabajo actual\n");
		// 	}
		// 	continue;
		// }
		// === FIN AMPLIACIÓN EJ2 ===


		//los comandos internos NUNCA hacen fork, los ejecuta el padre directamente
		if(strcmp(args[0],"cd")==0) {
			chdir(args[1]); //si lo hiciese el hijo, al morir el hijo el shell seguiria en la misma carpeta
			continue;
		}

		if(strcmp(args[0],"jobs")==0){
			print_job_list(lista);
			continue; //el continue hace que salte al while de arriba y no siga hacia abajo para hacer fork
		}

		if(strcmp(args[0],"fg")==0){
			int pos;

			if (args[1] == NULL) {
				pos = 1; 
			} else {
				pos = atoi(args[1]); 
			}

			job* tarea=get_item_bypos(lista,pos);

			if (tarea == NULL) {
				printf("Error: Tarea no encontrada\n");
				continue;	
			}
			tcsetpgrp(STDIN_FILENO,tarea->pgid); //le damos el control del teclado al grupo de ese hijo

			tarea->state=FOREGROUND;

			killpg(tarea->pgid,SIGCONT); //señal para que despierte y siga currando

			//flag WUNTRACED clave para examen: te avisa si el usuario vuelve a meterle un ctrl+z al proceso
			waitpid(tarea->pgid,&status,WUNTRACED);
				if (WIFEXITED(status)) {
                    info = WEXITSTATUS(status);
					estado="Exited";
					delete_job(lista,tarea);
                } 
                else if (WIFSIGNALED(status)) {
                    info = WTERMSIG(status);
					estado="Signaled";
					delete_job(lista,tarea);

                } 
                else if (WIFSTOPPED(status)) {
                    info = WSTOPSIG(status);
					estado="Suspended"; //si le da a ctrl+z se queda en la lista, no lo borramos
                }
			tcsetpgrp(STDIN_FILENO,getpid()); //el padre recupera los mandos del teclado
			continue;

		}
		if(strcmp(args[0],"bg")==0){
			int pos;
			if (args[1] == NULL) {
				pos = 1; 
			} else {
				pos = atoi(args[1]); 
			}

			job* tarea=get_item_bypos(lista,pos);

			if (tarea == NULL) {
				printf("Error: Tarea no encontrada\n");
				continue;	
			}
			tarea->state=BACKGROUND;
			
			killpg(tarea->pgid,SIGCONT); //lo despertamos pero NUNCA hacemos waitpid porque es de fondo

			continue;


		}

		pid_fork=fork(); //aqui se divide la historia: nace el hijo clonado

		if(pid_fork>0){ //si es mayor que cero estamos en el bloque del padre (shell original)
			
			//metemos al hijo en su propio grupo para que no le afecten los ctrl+c que iban pal shell
			gpid_padre=setpgid(pid_fork,0); 

			if(background==0){ //si no hay un ampersand, es en primer plano
				tcsetpgrp(STDIN_FILENO,pid_fork);  

				waitpid(pid_fork,&status,WUNTRACED); //nos quedamos congelados esperando al hijo

				if (WIFEXITED(status)) {
                    info = WEXITSTATUS(status);
					estado="Exited";
                } 
                else if (WIFSIGNALED(status)) {
                    info = WTERMSIG(status);
					estado="Signaled";
                } 
                else if (WIFSTOPPED(status)) { //si entra por aqui es que el pavo le ha dado a ctrl+z en pantalla
                    info = WSTOPSIG(status);
					estado="Suspended";
					job* tarea=new_job(pid_fork,args[0],STOPPED);
					block_SIGCHLD();
	
					add_job(lista,tarea); //lo apuntamos en nuestra libreta porque esta en pausa
					unblock_SIGCHLD();
                }
				
				tcsetpgrp(STDIN_FILENO,getpid()); //el shell papi vuelve a coger el teclado
				
				if(info!=255){
					printf("Foreground pid: %d,	 command: %s, %s, info: %d\n",pid_fork,args[0],estado,info);
				}

			}else{ //es un background
				
				registrar_tarea_background(pid_fork,args[0]); //se mete en la lista del tiron

				printf("Background job running... pid: %d,command: %s\n",pid_fork,args[0]);

			}
			
		}else{ //si es ==0 estamos en la zona exclusiva del proceso hijo nuevo
			
			gpid_hijo=setpgid(pid_fork,0); //hacemos esto a la vez que el padre para evitar condiciones de carrera

			if(background==0){
				tcsetpgrp(STDIN_FILENO,getpid());
			}
			
			restore_terminal_signals(); //le devolvemos el comportamiento por defecto de que el ctrl+c lo mate
			
			if(!(file_out==NULL)){
				//creamos la caja del archivo de salida. O_TRUNC es pa que se borre si ya habia algo
				int fd_out = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				if (fd_out < 0) {
					perror("Error abriendo el archivo de salida");
					exit(-1);
				}

				//el cambiazo maestro: le decimos que su ranura 1 (stdout) ahora es el archivo 
				dup2(fd_out, STDOUT_FILENO);

				//ya no nos hace falta el descriptor viejo asi que tapamos el agujero
				close(fd_out);
			}

			// === INICIO AMPLIACIÓN EJ5 (Redirección Append >>) PARTE 2 ===
			// /* COMENTA EL IF ORIGINAL DE FILE_OUT Y DESCOMENTA ESTE PARA EL EJ5 */
			// if (!(file_out == NULL)) {
			// 	int fd_out;
			// 	if (es_append) {
			// 		// O_APPEND es la clave del examen aqui: añade al final en vez de borrar
			// 		fd_out = open(file_out, O_WRONLY | O_CREAT | O_APPEND, 0644);
			// 	} else {
			// 		// O_TRUNC es el comportamiento clasico de machacar el fichero
			// 		fd_out = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			// 	}
			// 
			// 	if (fd_out < 0) {
			// 		perror("Error abriendo el archivo de salida");
			// 		exit(-1);
			// 	}
			// 	dup2(fd_out, STDOUT_FILENO);
			// 	close(fd_out);
			// }
			// === FIN AMPLIACIÓN EJ5 PARTE 2 ===

			if(!(file_in==NULL)){
				int fd_in = open(file_in, O_RDONLY, 0644);
				if (fd_in < 0) {
					perror("Error abriendo el archivo de entrada");
					exit(-1);
				}
				dup2(fd_in, STDIN_FILENO);
				close(fd_in);
			}

			execvp(args[0],args); //este comando pisa la memoria del hijo con el nuevo programa, no tiene retorno
			//si llegamos aqui es que el execvp ha fallado estrepitosamente porque no existe el comando
			printf("Error, command not found: %s\n",args[0]);
			exit(-1);
		}
		

	} // end while
}