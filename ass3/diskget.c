#include "diskutils.h"

int main(int argc, char** argv)
{
  if(argc < 3)
    {
      printf("Usage: diskget [diskname] [filename]");
    }

  DiskInfo di;
  openDisk(argv[1], &di);

  char* dirp = di.data + 19*PAGE_SIZE;
  int dirIndex;
  int found = 0;
  DirEntry entry;
  for(dirIndex = 0; dirIndex < 16*13; dirIndex++)
    {
       entry = *( (DirEntry*)(dirp) + dirIndex);
       if(entry.filename[0] == 0x00)    // end of directory
         break;
       if(entry.filename[0] == 0xE5 || entry.att & 0x08 || entry.att & 0x10) // not a file
         continue;
       else
         {
           char filename[20];
           getFileName(entry,filename);
           if(strcmp(filename, argv[2]) == 0) // found file
             {
               found = 1;
               break;
             }
         }
    }

  if(!found)
    {
      printf("File not found\n");
      exit(1);
    }

  FILE *fptr;
  fptr = fopen(argv[2], "wb");

  uint16_t FAT_val = entry.first;

  while(FAT_val < 0xFF8)
    {
      if(FAT_val == 0x00 || FAT_val > 0xFF0)
        {
          printf("Hit bad cluster in FAT table. Exiting.\n");
          exit(1);
        }

      void* fdata_ptr = (void*)(di.data + PAGE_SIZE * (33 + FAT_val - 2));

      fwrite( fdata_ptr, PAGE_SIZE, 1, fptr);
 
      FAT_val = readFatVal(di.data, FAT_val);
    }

  fclose(fptr);
}
