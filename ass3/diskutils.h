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
#define MAX_FAT_VAL PAGE_SIZE*9 

typedef unsigned int uint;

typedef struct DiskInfo
{
  char* data;
  int fd;
  int fat_copies;
  int sectors_per_fat;
  off_t disk_size;
  off_t disk_free;
  char os_name[9];
  char disk_label[12];
} DiskInfo;


typedef int16_t timeofday;
typedef int16_t date;
typedef struct timestamp
{
timeofday t;
date d;
} timestamp;


typedef struct DirEntry
{
char filename[FILENAME_SIZE];
char extension[EXTENSION_SIZE];
char att;
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

int getFileName(DirEntry entry, char* buf)
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
  buf[i] = '\0';
  return i;
}

void writeFileName(DirEntry* entry, char* fname)
{
  memset(&entry->filename,' ',FILENAME_SIZE);
  memset(&entry->extension,' ',EXTENSION_SIZE);

  int i;
  int j;
  for(i = 0; i <= FILENAME_SIZE; i++)
    {
      char cur = fname[i];
      if(cur == '.')
        {
          i++;
          break;
        }
      else if (cur == '\0')
        {
          return;
        }
      else
        {
          entry->filename[i] = cur;
        }
    }

  for(j = 0; j < EXTENSION_SIZE; j++, i++)
    {
      char cur = fname[i];
      if(cur == '\0')
        return;
      entry->extension[j] = cur;
    }
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

// reads 16 bit value as is instead of inverted
uint16_t readVal16(char* pt)
{
  return (int)*(pt+1) + ((int)*pt << 8);
}

uint16_t readFatVal(char* data, uint16_t index)
{
  uint16_t high;
  uint16_t low;

  if(index % 2) // odd case
    {
      high = *(data + 512 + 1 + (3*index/2)) << 4;
      low = (*(data + 512 + (3*index/2)) & 0xF0) >> 4;
    }
  else         // even case
    {
      high = (*(data + 512 + 1 + (3*index/2)) & 0x0F) << 8;
      low = (*(data + 512 + (3*index/2)) & 0x00FF);
    }

  return high + low;
}

void writeFatVal(char* data, uint16_t index, uint16_t val)
{
  uint16_t high;
  uint16_t low;
  char* hptr;
  char* lptr;


  if(index % 2) // odd case
    {
      high = (val & 0x0FF0) >> 4;
      low = (val & 0x000F);

      hptr = (data + 512 + 1 + 3*index/2);
      lptr = (data + 512 + (3*index/2));

      *hptr = high;
      *lptr = (low << 4) + (*lptr & 0x0F);
    }
  else         // even case
    {
      high = (val & 0x0F00) >> 8;
      low = (val & 0x00FF);

      hptr = (data + 512 + 1 + (3*index/2));
      lptr = (data + 512 + 3*index/2);

      *lptr = (char)low;
      *hptr = (high) + (*hptr & 0xF0);
    }
}

uint16_t seekEmptyFAT(char* data, uint16_t index)
{
  uint16_t i;
  for(i = index; i < PAGE_SIZE * 9; i++)
    {
      if(!readFatVal(data, i))
        return i;
    }
  return 0;
}

void openDisk(char* fn, DiskInfo* di)
{
  di->fd = open(fn, O_RDWR);
  if ( di->fd < 0)
    {
      printf("Failed to open file with error: %s\n", strerror(errno));
      exit(1);
    }
  di->disk_size = getFileSize(fn);

  di->data = mmap(0, di->disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, di->fd, 0);
  if ( di->data == MAP_FAILED)
    {
      printf("Memory map failed with error: %s\n", strerror(errno));
      exit(1);
    }
}

void closeDisk(DiskInfo* di)
{
  munmap(di->data, di->disk_size);
  close(di->fd);
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

