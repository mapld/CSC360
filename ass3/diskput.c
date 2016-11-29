#include "diskutils.h"

int main(int argc, char** argv)
{
  if(argc < 3)
    {
      printf("Usage: diskput [diskname] [filename]");
    }

  DiskInfo di;
  openDisk(argv[1], &di);

  char* dirp = di.data + 19*PAGE_SIZE;
  int dirIndex;
  int found = 0;
  DirEntry* entry;
  for(dirIndex = 0; dirIndex < 16*13; dirIndex++)
    {
      entry = ((DirEntry*)(dirp) + dirIndex);
      if(entry->filename[0] == 0x00)    // end of directory
        {
          found = 1;
          break;
        }
    }

  if(!found)
    {
      printf("Not enough free space on the disk image.");
      exit(1);
    }

  FILE *fptr;
  if(!(fptr = fopen(argv[2], "rb")))
    {
      printf("File not found.\n");
      exit(1);
    }

  uint16_t first_empty_cluster = seekEmptyFAT(di.data, 2);
  uint16_t FAT_val = first_empty_cluster;
  int32_t total_read_size = 0;
  while(FAT_val < MAX_FAT_VAL)
    {
      void* fdata_ptr = (void*)(di.data + PAGE_SIZE * (33 + FAT_val - 2));
      int32_t read_size = fread(fdata_ptr, 1, PAGE_SIZE, fptr);
      total_read_size += read_size;
      if(read_size < PAGE_SIZE)
        {
          writeFatVal(di.data, FAT_val, 0xFFF);
          break;
        }
      uint16_t next_FAT_val = seekEmptyFAT(di.data, FAT_val + 1);
      writeFatVal(di.data, FAT_val, next_FAT_val);
      FAT_val = next_FAT_val;
    }

  if(!FAT_val)
    printf("Not enough free space on the disk image.");

  // write dir entry
  writeFileName(entry, argv[2]);
  entry->att = 0x00;
  entry->first = first_empty_cluster;
  entry->filesize = total_read_size;

  fclose(fptr);
  closeDisk(&di);
}
