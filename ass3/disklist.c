#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

// Memory map
#include <sys/mman.h>

// For file size
#include <sys/stat.h>

// Error handling
#include <errno.h>
#include <string.h>

#include <stdint.h>

#define FILENAME_SIZE 8
#define EXTENSION_SIZE 3
#define PAGE_SIZE 512

typedef unsigned int uint;

typedef struct DiskInfo
{
  int fat_copies;
  int sectors_per_fat;
  off_t disk_size;
  off_t disk_free;
  char os_name[9];
  char disk_label[12];
} DiskInfo;


typedef int16_t timeofday;
typedef int16_t date;
typedef struct Timestamp
{
timeofday t;
date d;
} timestamp;

typedef struct DirEntry
{
char filename[FILENAME_SIZE];
char extension[EXTENSION_SIZE];
int8_t att;
int16_t res;
timestamp ctime; // creation time
date adate; // access date
int16_t ignore;
timestamp wtime; // last write time
int16_t first; // first logical cluster
int32_t filesize;
} DirEntry;

off_t getFileSize(char* fn)
{
  struct stat st;

  stat(fn, &st);
  return st.st_size;
}

void getFileName(DirEntry entry, char* buf)
{
  int i;
  for(i = 0; i < FILENAME_SIZE;i++)
    {
      if(isspace(entry.filename[i]))
        {
          break;
        }
      buf[i] = entry.filename[i];
    }
  if(!isspace(entry.extension[0]))
    {
      buf[i++] = '.';
      int j;
      for(j = 0; j < EXTENSION_SIZE; j++,i++)
        {
          buf[i] = entry.extension[j];
        }
    }
  buf[i++] = '\0';
}

// Gets a fixed series of characters from disk and adds null terminator
void readStr(char* src, off_t loc, char* dst, int len)
{
  int flag = 0;
  int i;
  for(i = 0 ; i < len; i++)
    {
      *(dst + i) = *(src + loc + i);
      if (!isspace(*(dst + i)))
        {
          flag = 1;
        }
    }
  dst[len] = 0;

  // If the string is only whitespace, set to empty
  if(!flag)
      *dst = 0;
}

uint readFatVal(char* data, int index)
{
  uint high;
  uint low;

  if(index % 2) // odd case
    {
      high = *(data + 512 + 1 + (3*index/2)) << 4;
      low = (*(data + 512 + (3*index/2)) & 0xF0) >> 4;
    }
  else         // even case
    {
      high = (*(data + 512 + 1 + (3*index/2)) & 0x0F) << 8;
      low = *(data + 512 + (3*index/2));
    }

  /* printf("Index: %d\n", index); */
  /* printf("high: %u\n", high); */
  /* printf("low: %u\n", low); */
  /* printf("high+low: %u\n", high+low); */
  return high + low;
}

off_t getDiskUsed(char* data)
{
  int fat_offset = 512;
  int i = 0;
  uint fat_val;
  while(fat_val = readFatVal(data, i + 2))
    {
      i++;
    }
  return 512 * (i + 2);
}

int main(int argc, char** argv)
{
  DiskInfo di;

  if(argc < 2)
    {
      printf("Usage: disklist [filename] \n");
      exit(1);
    }

  int fd;
  char* data;

  fd = open(argv[1], O_RDONLY);
  if ( fd < 0)
    {
      printf("Failed to open file with error: %s\n", strerror(errno));
      exit(1);
    }
  di.disk_size = getFileSize(argv[1]);

  data = mmap(0, di.disk_size, PROT_READ, MAP_SHARED, fd, 0);
  if ( data == MAP_FAILED)
    {
      printf("Memory map failed with error: %s\n", strerror(errno));
      exit(1);
    }

  readStr(data,3,di.os_name,8);

  readStr(data,43,di.disk_label,11);

  di.disk_free = di.disk_size - getDiskUsed(data);

  di.fat_copies = (int)data[16];

  di.sectors_per_fat = ((int)data[22]) + ((int)data[23] << 8);

  char* dirp = data + 19*PAGE_SIZE;
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

      // check if descriptor
      // check if directory
      // otherwise file

      char full_file_name[FILENAME_SIZE+EXTENSION_SIZE+2];
      getFileName(entry,full_file_name);
      printf("filename: %s\n", full_file_name);
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

  printf("Number of FAT copies: %d\n", di.fat_copies);

  printf("Sectors per FAT: %d\n", di.sectors_per_fat);
}
