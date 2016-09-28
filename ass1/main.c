#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 30


int bg(char** parmList)
{
  if (execvp(parmList[0],parmList) != 0)
    {
      printf("Invalid bg command\n");
      return -1;
    }
  return 0;
}

int main(void)
{
  char* input;
  char* prompt = "PMan:  >";

  int status = 0;
  while(status >= 0)
    {
      input = readline(prompt);

      char command[40];

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
              printf("Please enter a command to run after bg");
            }
          else
            {
              bg(parmList);
            }
        }
      else
        {
          printf("Invalid command: %s\n" , command);
        }
    }

  return status;
}
