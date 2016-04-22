#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "snappy/snappy.h"
using namespace std;

char *get_config(const char *file)
{
  FILE *fd = fopen(file, "rb+");
  if (fd == NULL)
  {
    fprintf(stderr, "open conf file %s failed: %s", file, strerror(errno));
    return NULL;
  }
  fseek(fd, 0L, SEEK_END);
  int64_t len = ftell(fd);
  rewind(fd);// fseek(fd, 0L, SEEK_SET);
  char *buf = new char[len];
  assert(fread(buf, sizeof(char), len, fd) == (size_t)len);
  size_t output_length = 0;
  if(!snappy::GetUncompressedLength(buf, len, &output_length))
  {
    fprintf(stderr, "snappy decompress err while GetUncompressedLength");
    fclose(fd);
    return NULL;
  }
  char *output = new char[output_length];
  if(!snappy::RawUncompress(buf, len, output))
  {
    fprintf(stderr, "snappy decompress err while RawUncompress");
    delete []buf;
    delete []output;
    fclose(fd);
    return NULL;
  }
  fclose(fd);
  return output;
}

void write_conf(const char *file, const char *content)
{
  FILE *fd = fopen(file, "w+");
  fprintf(fd, content);
  fclose(fd);
}

int main(int argc, char *argv[])//argv[0]:main, argv[1]:input, argv[2]:output
{
  if (argc < 3)
  {
    fprintf(stderr, "Usage:\n./parse_admin_conf input.txt out.txt\n\n");
    return -1;
  }
  char *output = get_config(argv[1]);
  cout << "admin conf:" << output;
  if (output == NULL)
    return -1;
  write_conf(argv[2], output); 
  delete output;
  return 0;
}
