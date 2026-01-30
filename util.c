#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"

FileInfo get_file_info(const char *file_path)
{
  FileInfo file_info = {0};
  FILE* file;
  if ((file = fopen(file_path, "r")) == NULL) {
    fprintf(stderr, "ERROR: Could not open file %s", file_path);
    return file_info;
  }

  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file); 
  fseek(file, 0, SEEK_SET);

  char* file_contents = malloc(file_size);

  if (file_contents == NULL) {
    fprintf(stderr, "ERROR: Could not allocate buffer contents of file %s", file_path);
    fclose(file);
    return file_info;
  }

  fread(file_contents, 1, file_size, file);
  
  fclose(file);
  file_info.content = file_contents;
  file_info.size = file_size;
  return file_info;
}

uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
