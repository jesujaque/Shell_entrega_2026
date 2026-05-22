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

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

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

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
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

		pid_fork=fork();

		if(pid_fork>0){//Si es !=0 es padre y si es ==0 es hijo, en este caso abarcamos la zona del padre 


			if(background==0){//segundo plano

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
				
				if(info!=255){//print a hacer: Foreground pid: 5615, command: ls, Exited, info: 0
					printf("\nForeground pid: %d, command: %s, %s, info: %d\n",pid_fork,args[0],estado,info);
				}

			}else{
				//print a hacer: Background job running... pid: 5622, command: sleep
				printf("\nBackground job running... pid: %d,command: %s\n",pid_fork,args[0]);

			}
			
		}else{//aquí abarcamos la zona del hijo
			execvp(args[0],args);
			printf("\nError, command not found: %s\n",args[0]);
			exit(-1);
		}
		

	} // end while
}
