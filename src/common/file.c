#include <stdbool.h>
#include <stdio.h>
#include <common/logging.h>

bool file_save(const char* fileName, int size, const void* data)
{
  FILE *dest;
  int written;
  dest = fopen(fileName, "wb");
  if (dest == NULL)
  {
    LOG("Failed to open %s for save.", fileName);
    return false;
  }

  written = fwrite(data, 1, size, dest);
  if(written != size)
  {
    LOG("Failed to write object to %s", fileName);
    fclose(dest);
    return false;
  }
  fclose(dest);
  return true;
}

bool file_load(const char* fileName, int* size, void* data, int data_size)
{
  FILE *src;
  int read;
  src = fopen(fileName, "rb");
  if (src == NULL)
  {
    LOG("Failed to open %s for load.", fileName);
    return false;
  }
  fseek(src, 0L, SEEK_END);
  *size = ftell(src);
  fseek(src, 0L, SEEK_SET);
  if(*size > data_size)
  {
    LOG("Not enough size in data buffer, %d available, %d required.", data_size, *size);
    return false;
  }
  read = fread( data, 1, *size, src );
  if(read != *size)
  {
    LOG("Failed to read object from %s", fileName);
    fclose(src);
    return false;
  }
  fclose(src);
  return true;
}

bool file_delete(const char* fileName)
{
  int result = remove(fileName);
  if(result != 0)
  {
    LOG("Failed to delete %s", fileName);
    return false;
  }
  return true;
}