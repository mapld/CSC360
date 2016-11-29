#include "diskutils.h"

int main(int argc, char** argv)
{

  if(argc < 2)
    {
      printf("Usage: diskinfo [filename] \n");
      exit(1);
    }

  DiskInfo di;
  openDisk(argv[1], &di);

  readStr(di.data,3,di.os_name,8);

  readStr(di.data,43,di.disk_label,11);

  di.disk_free = di.disk_size - getDiskUsed(di.data);

  di.fat_copies = (int)di.data[16];

  di.sectors_per_fat = *((int16_t*)(di.data + 22));

  int root_file_count = 0;
  char* dirp = di.data + 19*PAGE_SIZE;
  int dirIndex;
  for(dirIndex = 0; dirIndex < 16*13; dirIndex++)
    {
      DirEntry entry = *( (DirEntry*)(dirp) + dirIndex);
      if(entry.filename[0] == 0x00)
        {
          break;
        }
      if(entry.filename[0] == 0xE5)
        {
          continue;
        }

      if(entry.att & 0x08)
        {
        // check if descriptor
          if(entry.att == 0x08)
            {
              getFileName(entry,di.disk_label);
            }
          continue;
        }

      if(!(entry.att & 0x10))
        {
          // not a subdir
          root_file_count++;
        }
    }

// print out info
  if(!(*di.os_name))
    strcpy(di.os_name, "N/A");
  printf("OS Name: %s\n", di.os_name);

  if(!(*di.disk_label))
    strcpy(di.disk_label, "N/A");
  printf("Label of the Disk: %s\n", di.disk_label);

  printf("Total size of the disk: %jd\n", di.disk_size);

  printf("Free size of the disk: %jd\n", di.disk_free);

  printf("===================\n");

  printf("The number of files in the root directory (not including subdirectories): %d\n", root_file_count);

  printf("===================\n");

  printf("Number of FAT copies: %d\n", di.fat_copies);

  printf("Sectors per FAT: %d\n", di.sectors_per_fat);

  // cleanup
  closeDisk(&di);
}
