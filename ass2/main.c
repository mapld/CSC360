#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct Flow
{

}

int main(int argc, char** argv)
{
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t line_len;

  if(argc <= 1)
    {
      printf("Usage: MFS [filename]\n");
      return 1;
    }
  fp = fopen(argv[1], "r");
  if (fp == NULL)
    {
      printf("Failed to open file\n");
      return 2;
    }

  int num_flows;
  fscanf(f, "%d\n", &num_flows);


  fclose(fp);
  free(line);
  return 0;
}

