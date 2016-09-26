#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(void)
{
  char* input = NULL;
  char* prompt = "PMan:  >";

  // if status < 0, exit
  int status = 0;
  while(status >= 0)
    {
      input = readline(prompt);
      printf("Thanks for enting the command %s\n",input);
    }

  return status;
}
