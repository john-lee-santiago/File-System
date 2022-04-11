//include any other header files you need

#include "mfs.h"
#include "fsInit.h"


int fs_mkdir(const char *pathname, mode_t mode)
  {
  //copy from root directory stuff
  }

int fs_rmdir(const char *pathname)
  {
  //muhammed
  }

fdDir * fs_opendir(const char *name)
  {
  //john
  }

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
  {
  //john
  }

int fs_closedir(fdDir *dirp)
  {
  //john
  }

char * fs_getcwd(char *buf, size_t size)
  {
  //janelle
  }

int fs_setcwd(char *buf)
  {
  //janelle
  }

int fs_isFile(char * path)
  {
  //madina
  }

int fs_isDir(char * path)
  {
  //madina
  }

int fs_delete(char* filename)
  {
  //muhammed
  }