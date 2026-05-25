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
job* lista;

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

void registrar_tarea_background(int pid,char* comando,job* lista){
	block_SIGCHLD();
	add_job(lista,new_job(pid,comando,BACKGROUND));
	unblock_SIGCHLD();
}

void manejador_tarea_zombie(int signum){
	int pid_hijo;
	int status;
	char* estado;
	while((pid_hijo=waitpid(-1,&status,WUNTRACED|WNOHANG))>0){
		block_SIGCHLD();
		job* job_manejado=get_item_bypid(lista,pid_hijo);
		if (WIFEXITED(status)) {
			delete_job(lista,job_manejado);
        } 
        else if (WIFSIGNALED(status)) {
			delete_job(lista,job_manejado);	
		} 
        else if (WIFSTOPPED(status)) {
        	
        }
		unblock_SIGCHLD();
	}
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



	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   	
		lista=new_list(nombre_lista);
		signal(SIGCHLD, manejador_tarea_zombie);

		ignore_terminal_signals();
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		
		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

		if(strcmp(args[0],"cd")==0) {
			chdir(args[1]);
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
                }
				//recuperamos el terminal para el grupo shell
				tcsetpgrp(STDIN_FILENO,getpid());
				
				if(info!=255){//print a hacer: Foreground pid: 5615, command: ls, Exited, info: 0
					printf("Foreground pid: %d,	 command: %s, %s, info: %d\n",pid_fork,args[0],estado,info);
				}

			}else{//background
				
				registrar_tarea_background(pid_fork,args[0],lista);

				//print a hacer: Background job running... pid: 5622, command: sleep
				printf("Background job running... pid: %d,command: %s\n",pid_fork,args[0]);

			}
			
		}else{//aquí abarcamos la zona del hijo
			gpid_hijo=setpgid(pid_fork,0);

			if(background==0){
				tcsetpgrp(STDIN_FILENO,getpid());
			}
			restore_terminal_signals();
			execvp(args[0],args);
			printf("Error, command not found: %s\n",args[0]);
			exit(-1);
		}
		

	} // end while
}

