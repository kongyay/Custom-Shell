#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

int split(char *str, char *result[], int max_size)
{
    char *p, *start_of_word;
    int c;
    enum states { DULL, IN_STRING, IN_WORD } state = DULL;
    int argc = 0;
    char quotetype;

    for (p = str; argc < max_size && *p != '\0'; p++) {
        c = (char) *p;
        //printf("[%d]%c\n",state,c);
        switch (state) {
        case DULL:
            if (isspace(c)) {
                continue;
            }

            if (c == '\"' || c == '\'') {
                state = IN_STRING;
                start_of_word = p + 1; 
                quotetype = c;
                continue;
            }
            state = IN_WORD;
            start_of_word = p;
            continue;

        case IN_STRING:
            if (c == quotetype) {
                *p = 0;
                result[argc++] = start_of_word;
                state = DULL;
            } else if(isspace(c)) {
                *p = '+';
            }
            continue;

        case IN_WORD:
            if (isspace(c)) {
                *p = 0;
                result[argc++] = start_of_word;
                state = DULL;
            }
            if (c == '\"' || c == '\'') {
                state = IN_STRING;
                quotetype = c;
                *p = '+';
                continue;
            }
            continue;
        }
    }

    if (state != DULL && argc < max_size)
        result[argc++] = start_of_word;

    return argc;
}

int splitcmd (char* str, char ***result, char splitter)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0')
    {
        if (*p == splitter)
            count++;
        p++;
    }

    *result = (char**) malloc(sizeof(char*) * count);
    if (*result == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == splitter)
        {
            (*result)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*result)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*result)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*result)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*result)[i]);
    while (*p != '\0')
    {
        if (*p != splitter && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*result)[i]);
        }
        p++;
    }

    return count;
}

int forkExec(char** cmd) {
    int i;
    for (i = 0; cmd[i]; i++)
    {
        if (strcmp(cmd[i], "quit") == 0) {
            printf("\x1B[31mExit shell.. \n\x1B[0m");
            exit(0);
        }
    } 

    int pid = fork();
    if (pid == 0)
    {
        int status = execvp(cmd[0], cmd);
        //perror(scanCmd[0]); /* execvp failed */
        if (status < 0) {
            printf("\x1B[31mCommand Not Found: %s\n\x1B[0m", cmd[0]);
            exit(1);
        } 
     
        exit(0);
    }
    else
    {
        int pid_print = fork();
        if (pid_print == 0) {
            waitpid(pid);
            printf("\x1B[34m ------ EXECUTING : %s \x1B[0m\n",cmd[0]);
            exit(0);
        }
        return pid;
    }
}

void exec_line(char* scan) {
    char*** concur;
    char **cmd;
    int i,len,len2;

    // Remove '\n' 
    if (scan[strlen(scan) - 1] == '\n')
            scan[strlen(scan) - 1] = '\0';

    // Split command by ';'
    concur = malloc(sizeof(scan));
    len = splitcmd(scan, &concur, ';');
    
    // Run each command concurrently
    int pid[len]; 
    if(len>1) printf("\x1B[32m =========== %d commands ===========\x1B[0m\n",len);
    for(i = 0;concur[i];i++) {
        
        cmd = malloc(sizeof(concur[i]));
        len2 = split(concur[i],cmd,100);
        pid[i] = forkExec(cmd);
        
        
    }
    for (i = 0; i<len;i++)
        waitpid(pid[i]);

    if(len>1) printf("\x1B[32m =========== DONE ===========\x1B[0m\n\n");
}

int main(int argc, char **result)
{
    int pid;
    int len,len2;
    

    system("clear");
    printf("\e[1m    😃 Welcome\n");

    // interactive
    if (argc < 2)
    {
        printf("\x1B[35mInteractive mode\n\e[0m");

        while (1)
        {

            char scan[100];
            printf("\e[1m\x1B[32m \nprompt> \e[0m\x1B[0m");
            fflush(stdin);
            gets(scan);

            exec_line(scan);

        }
        //fprintf(stderr, "usage: %s output_file\n", result[0]);
        
        exit(0);
    }

    // batch
    else
    {
        FILE *fp;
        char scan[100];
        
        printf("\x1B[35mEntering batch mode\n \x1B[0m");

        fp = fopen(result[1], "r");

        if (!fp)
        {
            perror(result[1]); /* open failed */
            exit(1);
        }
        else
        {
            printf("\x1B[32m\x1B[32m ✓ Open successful \x1B[0m\e[0m\n");
            int line = 0;
            while (!feof(fp))
            {
                // Read line
                printf("\x1B[32m =========== Line %d:    ===========\x1B[0m\n",++line);
                fgets(scan, 100, fp);
                exec_line(scan);
                
            }

            exit(0);
        }
    }

    exit(1);
}
