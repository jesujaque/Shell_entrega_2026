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
job* lista;

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
#include <dirent.h>
#include <stdio.h>

void traverse_proc(void) {
    DIR *d; 
    struct dirent *dir;
    char buff[2048];
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            sprintf(buff, "/proc/%s/stat", dir->d_name); 
            FILE *fd = fopen(buff, "r");
            if (fd){
                long pid;     // pid
                long ppid;    // ppid
                char state;   // estado: R (runnable), S (sleeping), T(stopped), Z (zombie)

                // La siguiente línea lee pid, state y ppid de /proc/<pid>/stat
                if(fscanf(fd, "%ld %s %c %ld", &pid, buff, &state, &ppid)==4){
					if(state=='Z'){
						printf("%ld\n",pid);
					}	
				
				}
				
                fclose(fd);
				
            }
        }
        closedir(d);
    }
}

void registrar_tarea_background(int pid,char* comando){
	block_SIGCHLD();
	add_job(lista,new_job(pid,comando,BACKGROUND));
	unblock_SIGCHLD();
}

/* void manejador_tarea_zombie(int signum){
	int pid_hijo;
	int status;
	char* estado;
	while((pid_hijo=waitpid(-1,&status,WUNTRACED|WNOHANG))>0){
		block_SIGCHLD();
		job* job_manejado=get_item_bypid(lista,pid_hijo);
		if(job_manejado!=NULL){
			if (WIFEXITED(status)){
				delete_job(lista,job_manejado);
			} 
			else if (WIFSIGNALED(status)) {
				delete_job(lista,job_manejado);	
			} 
		}
		
        
		unblock_SIGCHLD();
	}
} */

void manejador_tarea_zombie(int signum){
	int pid_hijo;
	int status;
	
	block_SIGCHLD();
	int i=1;
	while(i <= list_size(lista)){
		job* tarea=get_item_bypos(lista,i);

		if(tarea!=NULL){
			pid_hijo = waitpid(tarea->pgid, &status, WUNTRACED | WNOHANG);
            
            if (pid_hijo > 0) { // Si el proceso ha cambiado de estado
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    delete_job(lista, tarea);
                    // Como acabamos de borrar el elemento 'i', el siguiente elemento 
                    // de la lista cae en esta misma posición. Por tanto, no sumamos 'i'.
                    continue; 
                }
            }
		}
		i++;
	}
	
	unblock_SIGCHLD();
	
}

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; 	/* pid for created and waited process */
	int status;             	/* status returned by wait */
	char *file_in, *file_out; 	/* file names for redirection */
	int info;
	char* estado;
	int gpid;	
	int gpid_hijo;
	int gpid_padre;
	char* nombre_lista="Lista de tareas";
	lista=new_list(nombre_lista);

	signal(SIGCHLD, manejador_tarea_zombie);


	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   	
		
		ignore_terminal_signals();
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		char *file_in,*file_out;
		parse_redirections(args,&file_in,&file_out);
		
		if(args[0]==NULL) continue;   // if empty command

		
		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

		if(strcmp(args[0],"zjobs")==0){
			block_SIGCHLD();
			traverse_proc();
			unblock_SIGCHLD();

			continue;
		}
	
		
		if(strcmp(args[0],"deljob")==0) {
			block_SIGCHLD();

			job* tarea=get_item_bypos(lista,1);
			if (tarea == NULL) {
				printf("No hay trabajo actual\n");
				unblock_SIGCHLD();

				continue;
			}else{
				if(tarea->state==BACKGROUND){
					printf("Borrando trabajo actual de la lista de jobs: PID=%d command=%s\n", tarea->pgid, tarea->command);

					delete_job(lista,tarea);
					unblock_SIGCHLD();
					continue;
				}
				
				if(tarea->state==STOPPED){
					printf("No se permiten borrar trabajos en segundo plano suspendidos\n");
					unblock_SIGCHLD();
					continue;
				}
			}
			
			
		}

		if(strcmp(args[0],"cd")==0) {
			chdir(args[1]);
			continue;
		}

		if(strcmp(args[0],"jobs")==0){
			print_job_list(lista);
			continue;
		}

		if(strcmp(args[0],"fg")==0){
			int pos;

			if (args[1] == NULL) {
				pos = 1; // Por defecto sacamos el primero
			} else {
				pos = atoi(args[1]); // Convertimos el texto a número
			}

			job* tarea=get_item_bypos(lista,pos);

			if (tarea == NULL) {
				printf("Error: Tarea no encontrada\n");
				continue;	
			}
			tcsetpgrp(STDIN_FILENO,tarea->pgid);

			tarea->state=FOREGROUND;

			killpg(tarea->pgid,SIGCONT);

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
					estado="Suspended";
                }
			tcsetpgrp(STDIN_FILENO,getpid());
			continue;

		}
		if(strcmp(args[0],"bg")==0){
			int pos;
			if (args[1] == NULL) {
				pos = 1; // Por defecto sacamos el primero
			} else {
				pos = atoi(args[1]); // Convertimos el texto a número
			}

			job* tarea=get_item_bypos(lista,pos);

			if (tarea == NULL) {
				printf("Error: Tarea no encontrada\n");
				continue;	
			}
			tarea->state=BACKGROUND;
			
			killpg(tarea->pgid,SIGCONT);

			continue;


		}


		pid_fork=fork();

		if(pid_fork>0){//Si es !=0 es padre y si es ==0 es hijo, en este caso abarcamos la zona del padre 
			gpid_padre=setpgid(pid_fork,0);



			if(background==0){//segundo plano
				tcsetpgrp(STDIN_FILENO,pid_fork);  

				waitpid(pid_fork,&status,WUNTRACED);

				if (WIFEXITED(status)) {
                    info = WEXITSTATUS(status);
					estado="Exited";
                } 
                else if (WIFSIGNALED(status)) {
                    info = WTERMSIG(status);
					estado="Signaled";
                } 
                else if (WIFSTOPPED(status)) {
                    info = WSTOPSIG(status);
					estado="Suspended";
					job* tarea=new_job(pid_fork,args[0],STOPPED);
					block_SIGCHLD();
	
					add_job(lista,tarea);
					unblock_SIGCHLD();
                }
				//recuperamos el terminal para el grupo shell
				tcsetpgrp(STDIN_FILENO,getpid());
				
				if(info!=255){//print a hacer: Foreground pid: 5615, command: ls, Exited, info: 0
					printf("Foreground pid: %d,	 command: %s, %s, info: %d\n",pid_fork,args[0],estado,info);
				}

			}else{//background
				
				registrar_tarea_background(pid_fork,args[0]);

				//print a hacer: Background job running... pid: 5622, command: sleep
				printf("Background job running... pid: %d,command: %s\n",pid_fork,args[0]);

			}
			
		}else{//aquí abarcamos la zona del hijo
			gpid_hijo=setpgid(pid_fork,0);


			if(background==0){
				tcsetpgrp(STDIN_FILENO,getpid());
			}
			restore_terminal_signals();
			if(!(file_out==NULL)){
				// 1. Fabricar la caja (Abrir el archivo)
				// O_WRONLY: Solo para escribir.
				// O_CREAT: Si no existe, créalo.
				// O_TRUNC: Si ya existe y tiene texto, bórralo y empieza de cero.
				// 0644: Los permisos del archivo (Lectura/Escritura para el dueño, lectura para el resto).
				int fd_out = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				// Siempre hay que comprobar si hubo un error al abrir el archivo
				if (fd_out < 0) {
					perror("Error abriendo el archivo de salida");
					exit(-1);
				}

				// 2. Cambiar el cable
				// Le decimos al sistema: "Haz que el cable 1 (STDOUT_FILENO) apunte a mi archivo (fd_out)"
				dup2(fd_out, STDOUT_FILENO);

				// 3. Cerrar la tapa
				// Ya hemos enchufado STDOUT_FILENO, así que el descriptor original ya no nos hace falta.
				close(fd_out);
			}
			if(!(file_in==NULL)){
				int fd_in = open(file_in, O_RDONLY, 0644);
				if (fd_in < 0) {
					perror("Error abriendo el archivo de entrada");
					exit(-1);
				}
				dup2(fd_in, STDIN_FILENO);
				close(fd_in);


			}

			execvp(args[0],args);
			printf("Error, command not found: %s\n",args[0]);
			exit(-1);
		}
		

	} // end while
}

