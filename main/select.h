#ifndef __SELECT_H__
#define __SELECT_H__

#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
typedef void (*callback_t)(void);

void SelectInit();
void AddFd(int fd, callback_t handler);
void DeleteFd(int fd);
void Loop();

#endif
