#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 30

// Data take up process struct size * MAX_PROCESS * MAX_WORD_LENGTH
#define MAX_PROCESS 100
#define MAX_WORD_LENGTH 60

typedef struct Process
{
  pid_t pid;
  char name[MAX_WORD_LENGTH];
} Process;

typedef struct ProcessList
{
  int n;
  Process p[MAX_PROCESS];
} ProcessList;

int bg(char** parmList, ProcessList* pList)
{
  pid_t pid;
  pid = fork();
  if ( pid == 0) // in child
    {
      if (execvp(parmList[0],parmList) != 0)
        {
          printf("Invalid bg command\n");
          return -1;
        }
    }
  else if( pid < 0) // error
    {
      printf("Error creating process\n");
      return -2;
    }
  else // in parent
    {
      pList->p[pList->n].pid = pid;
      strcpy(pList->p[pList->n].name, parmList[0]);
      pList->n++;
    }
  return 0;
}

int bglist(ProcessList* pList)
{
  updateProcessStatus(pList);
  int i;
  for(i = 0; i < pList->n;i++)
    {
      printf("%ld: %s\n", (long)pList->p[i].pid, pList->p[i].name);
    }
  printf("Total background jobs: %d\n" , pList->n);
  return 0;
}


int updateProcessStatus(ProcessList* pList)
{
  int i;
  for(i = 0;i < pList->n;i++)
    {
      int status;
      if(waitpid(pList->p[i].pid,&status, WNOHANG) > 0)
        {
          // alert the user to the end of this process
          printf("Child with PID %ld terminated\n", (long)pList->p[i].pid);

          // delete the element from process storage by "swapping" with last element
          pList->p[i] = pList->p[pList->n - 1];
          pList->n--;
        }
    }
  return 0;
}

int killAll(ProcessList* pList)
{
  int i;
  for(i = 0; i < pList->n; i++)
    {
      kill(pList->p[i].pid, SIGKILL);
      printf("Child with PID %ld terminated\n", (long)pList->p[i].pid);
    }
  pList->n = 0;
  return 0;
}

int pstatReadStat(char* pid)
{
  char filename[1000];
  sprintf(filename, "/proc/%s/stat" , pid );
  FILE *f = fopen(filename, "r");
  if (f == NULL)
  {
    return -1;
  }

  size_t len = 0;
  char* line = NULL;

  if( getline(&line,&len,f) == -1)
    {
      printf("Failed to read proc file\n");
      fclose(f);
      return -1;
    }

  char* token;
  const int statLen = 20;

  int i = 0;
  token = strtok(line, " ");
  while(token != NULL)
    {
      // R
      switch(i)
        {
        case 1:
          printf("comm: %s\n", token);
          break;
        case 2:
          printf("state: %s\n", token);
          break;
        case 13:
          printf("utime: %s\n", token);
          break;
        case 14:
          printf("stime: %s\n", token);
          break;
        case 23:
          printf("rss: %s\n", token);
          break;
        }

      token = strtok(NULL, " ");
      i++;
    }

  fclose(f);
  free(line);

  return 0;
}

int pstatReadStatus(char* pid)
{
  char filename[1000];
  sprintf(filename, "/proc/%s/status", pid);
  FILE *f = fopen(filename, "r");
  if (f == NULL)
  {
    return -1;
  }

  size_t len = 0;
  char* line = NULL;
  char* token;

  while( getline(&line, &len, f) != -1)
    {
      token = strtok(line, " :");
      if(token != NULL)
        {
          char* str1 = "voluntary_ctxt_switches";
          char* str2 = "nonvoluntary_ctxt_switches";
          if(strcmp(token, str1) == 0)
            {
              token = strtok(NULL, " :\n");
              printf("%s: %s\n", str1, token);
            }
          if(strcmp(token, str2) == 0)
            {
              token = strtok(NULL, " :\n");
              printf("%s: %s\n", str2, token);
            }
        }
    }


  fclose(f);
  free(line);
  return 0;
}

int pstat(char* pid)
{
  printf("Printing stats for process with PID %ld\n" , (long)pid);

  if( pstatReadStat(pid) >= 0 &&
      pstatReadStatus(pid) >= 0)
    {
      return 0;
    }
  else
    {
      printf("Failed to find stats for a process with that PID\n");
      return -1;
    }
}

int main(void)
{
  char* input;
  char* prompt = "PMan:  >";

  ProcessList* pList = malloc(sizeof(ProcessList));

  int status = 0;
  while(status >= 0)
    {
      updateProcessStatus(pList);
      input = readline(prompt);

      char command[MAX_WORD_LENGTH];

      char* parmList[MAX_ARGS] = {};

      char* token;
      token = strtok(input, " ");

      if(token != NULL)
        {
          strcpy(command,token);
          token = strtok(NULL, " ");
        }
      else
        {
          continue;
        }

      int i = 0;
      while(token != NULL)
        {
          parmList[i] = malloc(strlen(token) + 1);
          strcpy(parmList[i], token);
          token = strtok(NULL, " ");
          i++;
          if(i >= MAX_ARGS)
            {
              printf("WARNING: Too many arguments\n");
              break;
            }
        }

      if (strcmp(command,"bg") == 0)
        {
          if(parmList[0] == NULL)
            {
              printf("Please enter a command to run after bg\n");
            }
          else
            {
              bg(parmList, pList);
            }
        }
      else if (strcmp(command, "bglist") == 0)
        {
          bglist(pList);
        }
      else if (strcmp(command, "bgkill") == 0)
        {
          if(parmList[0] == NULL)
            {
              printf("Please enter a PID\n");
            }
          else
            {
              printf("Killing process with pid %s\n" , parmList[0]);
              kill(strtol(parmList[0], NULL ,0), SIGTERM);
            }
        }
      else if (strcmp(command, "bgstop") == 0)
        {
          if(parmList[0] == NULL)
            {
              printf("Please enter a PID\n");
            }
          else
            {
              printf("Stopping process with pid %s\n" , parmList[0]);
              kill(strtol(parmList[0],NULL,0), SIGSTOP);
            }
        }
      else if (strcmp(command, "bgstart") == 0)
        {
          if(parmList[0] == NULL)
            {
              printf("Please enter a PID\n");
            }
          else
            {
              printf("Restarting process with pid %s" , parmList[0]);
              kill(strtol(parmList[0],NULL,0), SIGCONT);
            }
        }
      else if (strcmp(command, "exit") == 0)
        {
          printf("Killing all background processes\n");
          killAll(pList);

          printf("Exiting\n");
          break;
        }
      else if (strcmp(command, "pstat") == 0)
        {
          if(parmList[0] == NULL)
            {
              printf("Please enter a PID\n");
            }
          else
            {
              pstat(parmList[0]);
            }
        }
      else
        {
          printf("Invalid command: %s\n" , command);
        }

      int j;
      for(j = 0;j < i;j++)
        {
          free(parmList[j]);
        }
    }

  free(pList);
  return status;
}
