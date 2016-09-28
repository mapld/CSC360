#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>


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
      char* parmList[30] = {};

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
        }
      /* parmList[i] = (char*) NULL; */

      if (strcmp(command,"bg"))
        {
          bg(parmList);
        }
      else
        {
          printf("Invalid command: %s" , command);
        }
    }

  return status;
}
