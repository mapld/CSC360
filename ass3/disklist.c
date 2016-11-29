#include "diskutils.h"

int main(int argc, char** argv)
{
  if(argc < 2)
    {
      printf("Usage: disklist [filename] \n");
      exit(1);
    }

  DiskInfo di;
  openDisk(argv[1], &di);

  char* dirp = di.data + 19*PAGE_SIZE;
  int dirIndex;
  for(dirIndex = 0; dirIndex < 16*13; dirIndex++)
    {
      DirEntry entry = *( (DirEntry*)(dirp) + dirIndex);
      if(entry.filename[0] == 0x00)    // end of directory
          break;
      if(entry.filename[0] == 0xE5)    // free entry
          continue;
      if(entry.att & 0x08)     // volume label flag, ie not directory or file
          continue;

      char descriptor;
      if(entry.att & 0x10) // subdirectory
        descriptor = 'D';
      else
        descriptor = 'F';

      int filesize = entry.filesize;

      char filename[20];
      getFileName(entry, filename);

      uint16_t date = readVal16((char*)&entry.ctime.d);
      uint16_t time = readVal16((char*)&entry.ctime.t);

      printf("%c %d %s %d-%d-%d %02d:%02d\n", descriptor, filesize, filename, (date & 0x007F) + 1980, (date & 0x0780) >> 7, (date & 0xF800) >> 11, (time & 0x001F), (time & 0x07E0) >> 5);
    }

  closeDisk(&di);
}
