#ifndef UTIL_H
#define UTIL_H

int initRootDir();
const char* getRootDir();
int ensureDirectoryExist(const char* path);
bool isDaemonRunning();


#endif // UTIL_H
