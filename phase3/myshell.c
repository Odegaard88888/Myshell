#include "csapp.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#define Max_input 1024
char H[Max_input][Max_input];
int indx = 0;
int ex = 1;
int cz = 0;
struct k{
    int listnumber;
    pid_t pid;
    int RorS;
    int killed;
    char cmd[10000];
};
struct k job_index[100];
int jobs_count = 0;
int main(int argc, char** argv)
{
    FILE* HL = fopen("history3.txt", "r");
    if (HL != NULL)
    {
        char his[Max_input];
        while (fgets(his, Max_input, HL) != NULL)
            strcpy(H[indx++], his);
    }
    fclose(HL);
    char input[Max_input]; /* Command line */
    while (1) {
        printf("CSE4100-MP-P1> ");
        
        fgets(input, Max_input, stdin);
        if (feof(stdin))
            exit(0);
    
        /* Evaluate */
        ex = 1;
        evaluate(input);
    }
}
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
        unix_error("Dup2 error");
    return rc;
}
DIR *Opendir(const char *name)
{
    DIR *dirp = opendir(name);

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}
struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;

    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}
int Closedir(DIR *dirp)
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}
void RemoveQuote(char* input)
{
    char* start = NULL;
    char* end = NULL;
    int length = strlen(input);
    int x, y, z = 0;
    for (int i = 0; i < length; i++)
    {
        if (input[i] == '\'')
        {
            for (int j = i+1; j < length; j++)
            {
                if (input[j] == '\'')
                {
                    x = i;
                    y = j;
                    z = 1;
                    break;
                }
            }
        }
    }
    if (z == 0)
    {
        for (int i = 0; i < length; i++)
        {
            if (input[i] == '\"')
            {
                for (int j = i+1; j < length; j++)
                {
                    if (input[j] == '\"')
                    {
                        x = i;
                        y = j;
                        z = 1;
                        break;
                    }
                }

            }
        } 
    }
    if (z == 1)
    {
        for (int p = x; p < y-1; p++)
        {
            char r = input[p+1];
            input[p] = r;
        }
        for (int q = y-1; q < length-2; q++)
        {
            char s = input[q+2];
            input[q] = s;
        }
        input[length-2] = '\0';
    }
}
int parse(char* buf, char** args)
{
    char A[10000], B[10000], C[10000], D[10000];
    strcpy(A, buf);
    RemoveQuote(A);
    strcpy(B, A);
    RemoveQuote(B);
    strcpy(C, B);
    RemoveQuote(C);
    strcpy(D, C);
    RemoveQuote(D);
    int n = 0;
    char* a = strtok(D, " \n\t");
    while (a != NULL)
    {
        args[n] = a;
        n++;
        a = strtok(NULL, " \n\t");
    }
    args[n] = NULL;
    return n;
}

handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}
void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0)
        unix_error("Waitpid error");
    return(retpid);
}
void PipeExecute(char** args, int n)
{
    int pipecount = 0;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(args[i], "|") == 0)
            pipecount++;
    }
    if (pipecount == 1)
    {
        int a;
        char *com1[1000];
        char *com2[1000];
        
        for (int i = 0; i < n; i++)
        {
            if (strcmp(args[i], "|") == 0)
                a = i;
        }
        int p;
        for (p = 0; p < a; p++)
        {
           com1[p] = args[p];
        }
        com1[p] = '\0';
        p = 0;
        for (p = 0; p < n-a-1; p++)
        {
            com2[p] = args[p+a+1];
        }
        com2[p] = '\0';
        int fd1[2];
        if (pipe(fd1) == -1)
        {
            perror("pipe");
            exit(1);
        }
        pid_t pid;
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            Dup2(fd1[1], 1);
            close(fd1[1]);
            char Temp1[Max_input];
            strcpy(Temp1, "/bin/");
            strcat(Temp1, com1[0]);
            com1[0] = Temp1;
            Execve(Temp1, com1, NULL);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[1]);
            Dup2(fd1[0], 0);
            close(fd1[0]);
            char Temp2[Max_input];
            strcpy(Temp2, "/bin/");
            strcat(Temp2, com2[0]);
            com2[0] = Temp2;
            Execve(Temp2, com2, NULL);
            exit(0);
        }
        close(fd1[0]);
        close(fd1[1]);
        wait(NULL);
        wait(NULL);
        
    }
    else if (pipecount == 2)
    {
        int a = -1, b = -1;
        char* com1[1000];
        char* com2[1000];
        char* com3[1000];
        for (int i = 0; i < n; i++)
        {
            if (strcmp(args[i], "|") == 0) {
                if (a == -1)
                    a = i;
                else
                    b = i;
            }
        }
        int p = 0;
        
        for (p = 0; p < a; p++)
        {
            com1[p] = args[p];
        }
        com1[p] = '\0';
        int q = 0;
        for (q = 0; q < b-a-1; q++)
        {
            com2[q] = args[q+a+1];
        }
        com2[q] = '\0';
       
        int r = 0;
        for (r = 0; r < n-b-1; r++)
        {
            com3[r] = args[r+b+1];
        }
        com3[r] = '\0';
        int fd1[2], fd2[2];
        if (pipe(fd1) == -1)
        {
            perror("pipe");
            exit(1);
        }
        if (pipe(fd2) == -1)
        {
            perror("pipe");
            exit(1);
        }
        pid_t pid;
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            close(fd2[0]);
            close(fd2[1]);
            Dup2(fd1[1], 1);
            close(fd1[1]);
            execvp(com1[0], com1);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[1]);
            close(fd2[0]);
            Dup2(fd1[0], 0);
            Dup2(fd2[1], 1);
            close(fd1[0]);
            close(fd2[1]);
            execvp(com2[0], com2);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            close(fd1[1]);
            close(fd2[1]);
            Dup2(fd2[0], 0);
            close(fd2[0]);
            execvp(com3[0], com3);
            exit(0);
        }
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        wait(NULL);
        wait(NULL);
        wait(NULL);
    }
    
    else if (pipecount == 3)
    {
        int a = -1, b = -1, c = -1;
        char* com1[1000];
        char* com2[1000];
        char* com3[1000];
        char* com4[1000];
        for (int i = 0; i < n; i++)
        {
            if (strcmp(args[i], "|") == 0) {
                if (a == -1)
                    a = i;
                else if (b == -1)
                    b = i;
                else
                    c = i;
            }
        }
        int p = 0;

        for (p = 0; p < a; p++)
        {
            com1[p] = args[p];
        }
        com1[p] = '\0';
        int q = 0;
        for (q = 0; q < b-a-1; q++)
        {
            com2[q] = args[q+a+1];
        }
        com2[q] = '\0';

        int r = 0;
        for (r = 0; r < c-b-1; r++)
        {
            com3[r] = args[r+b+1];
        }
        com3[r] = '\0';
        int t = 0;
        for (t = 0; t < n-c-1; t++)
        {
            com4[t] = args[t+c+1];
        }
        com4[t] = '\0';
        int fd1[2], fd2[2], fd3[2];
        if (pipe(fd1) == -1)
        {
            perror("pipe");
            exit(1);
        }
        if (pipe(fd2) == -1)
        {
            perror("pipe");
            exit(1);
        }
        if (pipe(fd3) == -1)
        {
            perror("pipe");
            exit(1);
        }
        pid_t pid;
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            close(fd2[0]);
            close(fd2[1]);
            close(fd3[0]);
            close(fd3[1]);
            Dup2(fd1[1], 1);
            close(fd1[1]);
            execvp(com1[0], com1);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[1]);
            close(fd2[0]);
            close(fd3[0]);
            close(fd3[1]);
            Dup2(fd1[0], 0);
            Dup2(fd2[1], 1);
            close(fd1[0]);
            close(fd2[1]);
            execvp(com2[0], com2);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            close(fd1[1]);
            close(fd2[1]);
            close(fd3[0]);
            Dup2(fd2[0], 0);
            Dup2(fd3[1], 1);
            close(fd2[0]);
            close(fd3[1]);
            execvp(com3[0], com3);
            exit(0);
        }
        if ((pid = Fork()) == 0)
        {
            close(fd1[0]);
            close(fd1[1]);
            close(fd2[0]);
            close(fd2[1]);
            close(fd3[1]);
            Dup2(fd3[0], 0);
            close(fd3[0]);
            execvp(com4[0], com4);
            exit(0);
        }
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        close(fd3[0]);
        close(fd3[1]);
        wait(NULL);
        wait(NULL);
        wait(NULL);
        wait(NULL);
    }
}
void addjob(char** args, int n)
{
    job_index[jobs_count].listnumber = jobs_count+1;
    strcpy(job_index[jobs_count].cmd, args[0]);
    for (int i = 1; i < n; i++)
    {
        strcat(job_index[jobs_count].cmd, " ");
        strcat(job_index[jobs_count].cmd, args[i]);
    }
    job_index[jobs_count].killed = 0;
    jobs_count++;
}
void sigtstp_handler(int sig)
{
    if (job_index[jobs_count].pid > 0) {	
        kill(job_index[jobs_count].pid, SIGTSTP);
        job_index[jobs_count].RorS = 0;
        cz = 1;
    }
}
void evaluate(char* cmd)
{
  while (ex == 1) {
    ex = 0;
    char* args[10000];
    char buf[Max_input];
    pid_t pid;
    int status;
    strcpy(buf, cmd);
    int bg = 0;
    if (buf[strlen(buf) - 2] == '&')
    {
        bg = 1;
        buf[strlen(buf)-2] = NULL;
    }
    int n = parse(buf, args);
    if (n == 0)
        return;
    if (indx == 0)
    {
        strncpy(H[indx], cmd, strlen(cmd));
        indx++;
    }
    else if (indx > 0 && strcmp(cmd, H[indx-1])!= 0 && strncmp(cmd, "!!", 2) != 0 && !(strncmp(cmd, "!", 1) == 0 && isdigit(cmd[1]) != 0))
    {
        strncpy(H[indx], cmd, strlen(cmd));
        indx++;
    }
    int pipeexist = 0;
    for (int t = 0; t < n; t++) {
        if (strcmp(args[t], "|") == 0) {
            pipeexist = 1;
            break;
        }
    }
    if (pipeexist == 1 && strncmp(cmd, "!", 1) != 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            PipeExecute(args, n);
            exit(0);
        }
        else
        {
	    job_index[jobs_count].pid = pid;
	    signal(SIGTSTP, sigtstp_handler);
	    if (cz == 1)
	    {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
	    signal(SIGTSTP, SIG_DFL);
	    job_index[jobs_count].pid = 0;
        }
      }
      else
      {
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            PipeExecute(args, n);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }
      }
    }   
  else { 
    if ((strcmp(args[0], "jobs")) == 0)
    {
        for (int i = 0; i < jobs_count; i++)
        {
            if (job_index[i].killed != 1) {
                if (job_index[i].RorS == 0)
                    printf("[%d] running %s\n", job_index[i].listnumber, job_index[i].cmd);
                else if (job_index[i].RorS == 1)
                    printf("[%d] suspended %s\n", job_index[i].listnumber, job_index[i].cmd);
            }
            else
                continue;
        }
    }
    else if (strcmp(args[0], "bg") == 0)
    {
        int a = atoi(args[1] + 1);
        if (a > jobs_count || job_index[a-1].killed == 1)
            printf("No Such Job\n");
        else {
            printf("[%d] running %s\n", job_index[a-1].listnumber, job_index[a-1].cmd);
        }
    }
    else if (strcmp(args[0], "fg") == 0)
    {
        int a = atoi(args[1] + 1);
        if (a > jobs_count || job_index[a-1].killed == 1)
            printf("No Such Job\n");
        else {
             printf("[%d] running %s\n", job_index[a-1].listnumber, job_index[a-1].cmd);
	     job_index[a-1].killed = 1;
             ex = 1;
             strcpy(cmd, job_index[a-1].cmd);
        }	
    }
    else if (strcmp(args[0], "kill") == 0)
    {
        int a = atoi(args[1] + 1);
        if (a > jobs_count || job_index[a-1].killed == 1)
            printf("No Such Job\n");
        else {
            job_index[a-1].killed = 1;
        }
    }
    else if ((strcmp(args[0], "cd")) == 0)
    {	    
        if (n == 1)
            chdir(getenv("HOME"));
        else if (strcmp(args[1], "..") == 0)
        {
            if (chdir("..") == -1)
                perror("cd");
        }
        else
        {
            if (chdir(args[1]) == -1)
                perror("cd");
        }

    }
    else if (strcmp(args[0], "ls") == 0)
    { 
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/ls";
            args[0] = LS;
            Execve(LS, args, NULL);
            exit(0);
        }
        else
        {
	    job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
	}
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/ls";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }
                
      }
        
    }
    else if (strcmp(args[0], "mkdir") == 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* MK = "/bin/mkdir";
            args[0] = MK;
            Execve(MK, args, NULL);
            exit(0);
        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
	}
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/mkdir";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }
      }
    }
    else if (strcmp(args[0], "rmdir") == 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* RM = "/bin/rmdir";
            args[0] = RM;
            Execve(RM, args, NULL);
            exit(0);

        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
        }
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/rmdir";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }

      }
    }
    else if (strcmp(args[0], "touch") == 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* TO = "/bin/touch";
            args[0] = TO;
            Execve(TO, args, NULL);
            exit(0);
        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
        }
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/touch";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }
      }
    }
    else if (strcmp(args[0], "cat") == 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* CAT = "/bin/cat";
            args[0] = CAT;
            Execve(CAT, args, NULL);
            exit(0);
        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
        }
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/cat";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }

      }
    }
    else if (strcmp(args[0], "echo") == 0)
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char* EC = "/bin/echo";
            args[0] = EC;
            Execve(EC, args, NULL);
            exit(0);
        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
        }
      }
      else
      {
	job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/echo";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
        }
      }
    }
    else if (strcmp(args[0], "history") == 0)
    {
        for (int j = 0; j < indx ; j++)
        {
            printf("%d   %s", j+1, H[j]);
        }
    }
    else if (strncmp(args[0], "!!", 2) == 0)
    {
        if (strcmp(args[0], "!!") == 0)
	{
            if (indx <= 1)
            {
                printf("No latest command\n");
            }
            else
            {
                printf("%s", H[indx-1]);
                char* ARGS[Max_input];
                char X[10000];
                strncpy(X, H[indx-1], strlen(H[indx-1]));
                X[strlen(H[indx-1])] = '\0';
                int h = parse(X, ARGS);
                
		if((strcmp(ARGS[0], "cd")) == 0)
                {
                    if (h == 1)
                        chdir(getenv("HOME"));
                    else if (strcmp(ARGS[1], "..") == 0)
                    {
                        if (chdir("..") == -1)
                            perror("cd");
                    }
                    else
                    {
                        if (chdir(ARGS[1]) == -1)
                            perror("cd");
                    }

                }
                
                else if (strcmp(ARGS[0], "history") == 0)
                {
                    for (int j = 0; j < indx ; j++)
                    {
                        printf("%d   %s", j+1, H[j]);
                    }
                }
                else if (strcmp(ARGS[0], "exit") == 0)
                {
                    FILE* HL = fopen("history3.txt", "w");
                    if (HL != NULL)
                    {
                        for (int j = 0; j < indx; j++)
                            fprintf(HL, "%s", H[j]);
                    }
                    fclose(HL);
                    exit(0);
                }
                else if (strcmp(ARGS[0], "ls") == 0 || strcmp(ARGS[0], "mkdir") == 0 || strcmp(ARGS[0], "rmdir") == 0 || strcmp(ARGS[0], "touch") == 0 || strcmp(ARGS[0], "cat") == 0 || strcmp(ARGS[0], "echo") == 0)
                {
                    if ((pid = Fork()) == 0)
                    {
                        char Temp[Max_input];
                        strcpy(Temp, "/bin/");
                        strcat(Temp, ARGS[0]);
                        ARGS[0] = Temp;
                        Execve(Temp, ARGS, NULL);
                        exit(0);
                    }
                    else
                    {
                        job_index[jobs_count].pid = pid;
                        signal(SIGTSTP, sigtstp_handler);
                        if (cz == 1)
                        {
                            addjob(args, n);
                        }
                        Waitpid(pid, &status, 0);
                        signal(SIGTSTP, SIG_DFL);
                        job_index[jobs_count].pid = 0;
                    }
                }
                else
                     printf("%s: command not found\n", ARGS[0]);	
             
	    }
        }
        else
        {
            char* ARGS[Max_input];
            char X[10000], Y[10000];
            strncpy(X, H[indx-1], strlen(H[indx-1]));
            X[strlen(H[indx-1])-1] = '\0';
            strcat(X, buf+2);
            printf("%s\n", X);
            strcpy(Y, X);
            int h = parse(X, ARGS);
	    int PE = 0;
            for (int t = 0; t < n; t++) {
                if (strcmp(ARGS[t], "|") == 0) {
                    PE = 1;
                    break;
                }
            }
          if (PE == 1) {
              if ((pid = Fork()) == 0)
            {
                PipeExecute(ARGS, h);
                exit(0);
            }
            else
            {
                job_index[jobs_count].pid = pid;
                signal(SIGTSTP, sigtstp_handler);
                if (cz == 1)
                {
                    addjob(args, n);
                }
                Waitpid(pid, &status, 0);
                signal(SIGTSTP, SIG_DFL);
                job_index[jobs_count].pid = 0;
            }
	    strncpy(H[indx], X, strlen(X));
            indx++;
          }
          else {
            if (ARGS[1] == NULL)
            { 
                printf("%s: command not found\n", X);
                strncpy(H[indx], X, strlen(X));
                strcat(H[indx], "\n");
                indx++;

            }
            else {
                if ((strcmp(ARGS[0], "cd")) == 0)
                {
                    if (h == 1)
                        chdir(getenv("HOME"));
                    else if (strcmp(ARGS[1], "..") == 0)
                    {
                        if (chdir("..") == -1)
                            perror("cd");
                    }
                    else
                    {
                        if (chdir(ARGS[1]) == -1)
                            perror("cd");
                    }
                    strncpy(H[indx], X, strlen(X));
                    strcat(H[indx], "\n");
                    indx++;
                }
                else if (strcmp(ARGS[0], "history") == 0)
                {
                    for (int j = 0; j < indx ; j++)
                    {
                        printf("%d   %s", j+1, H[j]);
                    }
                    strncpy(H[indx], X, strlen(X));
                    strcat(H[indx], "\n");
                    indx++;
                }
                else if (strcmp(ARGS[0], "exit") == 0)
                {
                    FILE* HL = fopen("history3.txt", "w");
                    if (HL != NULL)
                    {
                        for (int j = 0; j < indx; j++)
                            fprintf(HL, "%s", H[j]);
                    }
                    fclose(HL);
                    strncpy(H[indx], X, strlen(X));
                    strcat(H[indx], "\n");
                    indx++;
                    exit(0);
                }

                else
                {
                    if ((pid = Fork()) == 0)
                    {
                        char Temp[Max_input];
                        strcpy(Temp, "/bin/");
                        strcat(Temp, ARGS[0]);
                        ARGS[0] = Temp;
                        Execve(Temp, ARGS, NULL);
                        exit(0);
                    }
                    else
                    {
                        job_index[jobs_count].pid = pid;
                        signal(SIGTSTP, sigtstp_handler);
                        if (cz == 1)
                        {
                            addjob(args, n);
                        }
                        Waitpid(pid, &status, 0);
                        signal(SIGTSTP, SIG_DFL);
                        job_index[jobs_count].pid = 0;
                    }
                    strncpy(H[indx], X, strlen(X));
                    strcat(H[indx], "\n");
                    indx++;
                }
	      }
            }
        }
    }
    else if (strncmp(args[0], "!", 1) == 0 && isdigit(args[0][1]) != 0)
    {
        int a = atoi(args[0] + 1);
        if (a > indx - 1)
            printf("!%d: event not found\n", a);
        else
        {
            printf("%s", H[a-1]);
            char* ARGS[Max_input];
            char X[10000], Y[10000];
            strncpy(X, H[a-1], strlen(H[a-1]));
            X[strlen(H[a-1])] = '\0';
            strcpy(Y, X);
            int h = parse(X, ARGS);
	    int PE = 0;
            for (int t = 0; t < n; t++) {
                if (strcmp(ARGS[t], "|") == 0) {
                    PE = 1;
                    break;
                }
            }
          if (PE == 1) {
              if ((pid = Fork()) == 0)
            {
                PipeExecute(ARGS, h);
                exit(0);
            }
            else
            {
                job_index[jobs_count].pid = pid;
                signal(SIGTSTP, sigtstp_handler);
                if (cz == 1)
                {
                    addjob(args, n);
                }
                Waitpid(pid, &status, 0);
                signal(SIGTSTP, SIG_DFL);
                job_index[jobs_count].pid = 0;
            }
            strncpy(H[indx], X, strlen(X));
            indx++;
          }
          else {
            if ((strcmp(ARGS[0], "cd")) == 0)
            {
                if (h == 1)
                    chdir(getenv("HOME"));
                else if (strcmp(ARGS[1], "..") == 0)
                {
                    if (chdir("..") == -1)
                        perror("cd");
                }
                else
                {
                    if (chdir(ARGS[1]) == -1)
                        perror("cd");
                }
                strncpy(H[indx], Y, strlen(Y));
                indx++;
            }
            else if (strcmp(ARGS[0], "history") == 0)
            {
                for (int j = 0; j < indx ; j++)
                {
                    printf("%d   %s", j+1, H[j]);
                }
                strncpy(H[indx], Y, strlen(Y));
                indx++;
            }
            else if (strcmp(ARGS[0], "exit") == 0)
            {
                strncpy(H[indx], Y, strlen(Y));
                indx++;
                FILE* HL = fopen("history3.txt", "w");
                if (HL != NULL)
                {
                    for (int j = 0; j < indx; j++)
                        fprintf(HL, "%s", H[j]);
                }
                fclose(HL);
                
                exit(0);
            }
            else if (strcmp(ARGS[0], "ls") == 0 || strcmp(ARGS[0], "mkdir") == 0 || strcmp(ARGS[0], "rmdir") == 0 || strcmp(ARGS[0], "touch") == 0 || strcmp(ARGS[0], "cat") == 0 || strcmp(ARGS[0], "echo") == 0)
            {
                if ((pid = Fork()) == 0)
                {
                    char Temp[Max_input];
                    strcpy(Temp, "/bin/");
                    strcat(Temp, ARGS[0]);
                    ARGS[0] = Temp;
                    Execve(Temp, ARGS, NULL);
                    exit(0);
                }
                else
                {
                    job_index[jobs_count].pid = pid;
                    signal(SIGTSTP, sigtstp_handler);
                    if (cz == 1)
                    {
                        addjob(args, n);
                    }
                    Waitpid(pid, &status, 0);
                    signal(SIGTSTP, SIG_DFL);
                    job_index[jobs_count].pid = 0;
                }
                strncpy(H[indx], Y, strlen(Y));
                indx++;
            }
            else {
                printf("%s: command not found\n", ARGS[0]);
                strncpy(H[indx], Y, strlen(Y));
                strcat(H[indx], "\n");
                indx++;
            }
	  }
        }
        
    }

    else if (strcmp(args[0], "exit") == 0)
    {
        FILE* HL = fopen("history3.txt", "w");
        if (HL != NULL)
        {
            for (int j = 0; j < indx; j++)
                fprintf(HL, "%s", H[j]);
        }
        fclose(HL);
        exit(0);
    }
    else
    {
      if (!bg) {
        if ((pid = Fork()) == 0)
        {
            char Temp[Max_input];
            strcpy(Temp, "/bin/");
            strcat(Temp, args[0]);
            args[0] = Temp;
            Execve(Temp, args, NULL);
            exit(0);
        }
        else
        {
            job_index[jobs_count].pid = pid;
            signal(SIGTSTP, sigtstp_handler);
            if (cz == 1)
            {
                addjob(args, n);
            }
            Waitpid(pid, &status, 0);
            signal(SIGTSTP, SIG_DFL);
            job_index[jobs_count].pid = 0;
        }
      }
      else {
        job_index[jobs_count].pid = pid;
        addjob(args, n);
        if ((pid = Fork()) == 0)
        {
            char* LS = "/bin/ls";
            args[0] = LS;
            int fd = open("/dev/null", O_WRONLY);
            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDERR_FILENO);
            close(fd);
            Execve(LS, args, NULL);
            Dup2(STDOUT_FILENO, fileno(stdout));
            exit(0);
	}
      }
    }
  }
  }
}   
