#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define MAX_CHAR 100
#define SHELL_NAME "shell"

int split(char *str, char *result[], int max_size)
{
    if (str[0] == 0)
    {
        result[0] = 0;
        return 0;
    }

    char *p, *start_of_word;
    int c;
    enum states
    {
        DULL,
        IN_STRING,
        IN_WORD
    } state = DULL;
    int argc = 0;
    char quotetype;

    for (p = str; argc < max_size && *p != '\0'; p++)
    {
        c = (char)*p;
        //printf("[%d]%c\n",state,c);
        switch (state)
        {
        case DULL:
            if (isspace(c))
            {
                continue;
            }

            if (c == '\"' || c == '\'')
            {
                state = IN_STRING;
                start_of_word = p + 1;
                quotetype = c;
                continue;
            }
            state = IN_WORD;
            start_of_word = p;
            continue;

        case IN_STRING:
            if (c == quotetype)
            {
                *p = 0;
                result[argc++] = start_of_word;
                state = DULL;
            }
            else if (isspace(c))
            {
                *p = '+';
            }
            continue;

        case IN_WORD:
            if (isspace(c))
            {
                *p = 0;
                result[argc++] = start_of_word;
                state = DULL;
            }
            if (c == '\"' || c == '\'')
            {
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

int splitcmd(char *str, char ***result, char splitter)
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

    *result = (char **)malloc(sizeof(char *) * count);
    if (*result == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == splitter)
        {
            (*result)[i] = (char *)malloc(sizeof(char) * token_len);
            if ((*result)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*result)[i] = (char *)malloc(sizeof(char) * token_len);
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

int forkExec(char **cmd, char len)
{
    int i, j, pid;

    if (len == 0)
    {
        return 0;
    }
    else if (strcmp(cmd[0], "quit") == 0)
    {
        fprintf(stderr, "\x1B[31mExit shell.. \n\x1B[0m");
        exit(0);
    }
    else if (strcmp(cmd[0], "cd") == 0)
    {
        chdir(cmd[1]);
        return 0;
    }
    else if (strcmp(cmd[0], SHELL_NAME) == 0)
    {
        fileScanner(cmd[1]);
        return 0;
    }

    int newfd, savefd, isRedirected = 0;
    for (i = 0; i < len - 1; i++)
    {
        if(cmd[i][0] == '>') {
            isRedirected = 1;

            char *filename = cmd[i + 1];
            // Open file for redirection
            if (strcmp(cmd[i], ">") == 0) {
                if ((newfd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0)
                {
                    perror(filename); /* open failed */
                    return 1;
                }
                fprintf(stderr, "\x1B[36m Output written in %s\x1B[0m\n", filename);
            } else if (strcmp(cmd[i], ">>") == 0) {
                if ((newfd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644)) < 0)
                {
                    perror(filename); /* open failed */
                    return 1;
                }
                fprintf(stderr, "\x1B[35m Output appended to %s\x1B[0m\n", filename);
            }
            
            

            // Remove " > .... " arguments
            len -= 2;
            for (j = i; j < len; j++)
            {
                memmove(cmd + j, cmd + j + 2, sizeof(cmd[0]));
            }
            savefd = dup(1);
            dup2(newfd, 1);
         }
    }

    cmd[len] = 0;

    pid = fork();
    if (pid == 0)
    {
        int status = execvp(cmd[0], cmd);
        perror(cmd[0]); /* execvp failed */

        if (status < 0)
        {
            fprintf(stderr, "\x1B[31m[Command Not Found]: %s\n\x1B[0m", cmd[0]);
            exit(1);
        }
        exit(0);
    }
    else
    {
        // if child comes from dup2, dont let it be another parent
        if (isRedirected)
        {
            dup2(savefd, 1);
            close(newfd);
            close(savefd);
            return 0;
        }

        return pid;
    }
}

void exec_line(char *scan)
{
    char ***concur;
    char **cmd;
    int i, len, len2;

    // Remove '\n'
    if (scan[strlen(scan) - 1] == '\n')
        scan[strlen(scan) - 1] = '\0';

    // Split command by ';'

    concur = malloc(sizeof(scan));
    len = splitcmd(scan, &concur, ';');

    // Run each command concurrently
    int pid[len];
    if (len > 1)
        fprintf(stderr, "\x1B[32m =========== %d commands ===========\x1B[0m\n", len);
    for (i = 0; i < len; i++)
    {

        cmd = malloc(sizeof(char *) * MAX_CHAR);
        len2 = split(concur[i], cmd, MAX_CHAR);

        pid[i] = forkExec(cmd, len2);
    }

    for (i = 0; i < len; i++)
        waitpid(pid[i]);

}

int fileScanner(char *filename)
{
    FILE *fp;
    char scan[MAX_CHAR];

    fprintf(stderr, "\x1B[35mEntering batch mode\n \x1B[0m");

    fp = fopen(filename, "r");

    if (!fp)
    {
        perror(filename); /* open failed */
        return 1;
    }
    else
    {
        fprintf(stderr, "\x1B[32m\x1B[32m ✓ Open successful \x1B[0m\e[0m\n");
        int line = 0;
        while (!feof(fp))
        {
            // Read line
            fprintf(stderr, "\x1B[32m =========== Line %d:    ===========\x1B[0m\n", ++line);
            fscanf(fp, "%[^\n]\n", scan);
            //fgets(scan, MAX_CHAR, fp);
            exec_line(scan);
        }
        fclose(fp);

        return 0;
    }
}

void sigint_handler(int sig)
{
    //system("clear");
    fprintf(stderr, "\e[1mGood Bye 😃\n");
    exit(0);
}

int main(int argc, char **argv)
{
    int pid;
    int len, len2;

    signal(SIGINT, sigint_handler);
    //signal(SIGSEGV,sigsegv_handler);

    system("clear");
    fprintf(stderr, "\e[1m    😃 Welcome\n");

    // interactive
    if (argc < 2)
    {
        fprintf(stderr, "\x1B[35mInteractive mode\n\e[0m");

        while (1)
        {

            char scan[MAX_CHAR];
            fprintf(stderr, "\e[1m\x1B[32m \nprompt> \e[0m\x1B[0m");
            fflush(stdin);
            gets(scan);

            exec_line(scan);
        }

        exit(0);
    }

    // batch
    else
    {
        fileScanner(argv[1]);
    }

    exit(1);
}
