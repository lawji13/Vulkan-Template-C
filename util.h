#ifndef UTIL_H
#define UTIL_H

typedef struct FileInfo
{
  char *content;
  size_t size;

}FileInfo;

FileInfo get_file_info(const char *file_path);
uint32_t clamp_u32(uint32_t value, uint32_t min, uint32_t max);
  
#endif // UTIL_H
