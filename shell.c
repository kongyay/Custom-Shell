#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

int main(int argc, char **argv)
{
    int pid,
        status;
    int
        newfd;
    char** cmd;
	system("clear");
	printf("\x1B[31m 😃 Welcome eiei\n");

    // interactive
    if (argc < 2) 
    {
        printf("\x1B[35m interactive mode\n");

        while(1) {


            	char line_command[20];
            	printf("\x1B[32m \nprompt> ");
		printf("\x1B[0m");
		fflush(stdin);
		gets(line_command);


		    cmd = str_split(line_command, ' ');
		    int i=0;
		    for(i=0;cmd[i];i++) {
			if(strcmp(cmd[i],"exit")==0)
				exit(1);
			
		    }


		int pid = fork();
		if(pid==0) {
			int status = execvp(cmd[0], cmd);
			printf("\x1B[31m");
			//perror(cmd[0]); /* execvp failed */
			if(status<0)
				printf("Command Not Found: %s",cmd[0]);
			printf("\x1B[0m");
			exit(0);
		} else {
			waitpid(pid);
		}
		

            for(i=0;cmd[i];i++)
		free(cmd[i]);

                
        }
        //fprintf(stderr, "usage: %s output_file\n",                argv[0]);
        
        exit(0);
    }

    // batch
    else {
	    if ((newfd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644)) <  0)
	    {
		perror(argv[1]); /* open failed */
		exit(1);
	    }
    }


    exit(1);
}
