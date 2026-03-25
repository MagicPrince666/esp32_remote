#ifndef __SELECT_H__
#define __SELECT_H__

#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

typedef void (*select_callback_t)(void);

// 回调链表节点
typedef struct callback_node {
    int fd;
    select_callback_t callback;
    struct callback_node *next;
} callback_node_t;

// 链表头指针
extern callback_node_t *callback_list_;

void SelectInit();
int AddFd(int fd, select_callback_t handler);
void DeleteFd(int fd);

// 遍历接口：返回第 index 个节点，不存在则返回 NULL
callback_node_t *GetCallbackNode(int index);

// 获取链表长度
int GetCallbackCount();

#endif
