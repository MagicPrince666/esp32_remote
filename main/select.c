#include "select.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdlib.h>

static const char* TAG = "select";
static bool loop_quit_ = false;
static void select_task(void *arg);

// 回调链表头指针
callback_node_t *callback_list_ = NULL;

// 初始化链表
static void list_init(void) {
    callback_node_t *node = callback_list_;
    while (node) {
        callback_node_t *next = node->next;
        free(node);
        node = next;
    }
    callback_list_ = NULL;
}

// 添加节点到链表（头插法）
int AddFd(int fd, select_callback_t handler) {
    // 查找是否已存在该 fd
    callback_node_t *curr = callback_list_;
    while (curr) {
        if (curr->fd == fd) {
            // 已存在则更新回调
            curr->callback = handler;
            return 0;
        }
        curr = curr->next;
    }

    // 创建新节点
    callback_node_t *node = (callback_node_t *)malloc(sizeof(callback_node_t));
    if (!node) {
        return -1;
    }
    node->fd = fd;
    node->callback = handler;
    node->next = callback_list_;
    callback_list_ = node;

    return 0;
}

// 删除链表中的指定 fd 节点
void DeleteFd(int fd) {
    callback_node_t *curr = callback_list_;
    callback_node_t *prev = NULL;

    while (curr) {
        if (curr->fd == fd) {
            if (prev) {
                prev->next = curr->next;
            } else {
                callback_list_ = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

// 遍历接口：获取第 index 个节点
callback_node_t *GetCallbackNode(int index) {
    callback_node_t *curr = callback_list_;
    int count = 0;

    while (curr) {
        if (count == index) {
            return curr;
        }
        count++;
        curr = curr->next;
    }
    return NULL;
}

// 获取链表节点数量
int GetCallbackCount(void) {
    callback_node_t *curr = callback_list_;
    int count = 0;

    while (curr) {
        count++;
        curr = curr->next;
    }
    return count;
}

void SelectInit() {
    list_init();
    loop_quit_ = false;
    xTaskCreate(select_task, "select_task", 4 * 1024, NULL, 5, NULL);
}

void select_task(void *arg)
{
    fd_set rfds;
    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0,
    };

    while (!loop_quit_) {
        int maxfd = 0;
        FD_ZERO(&rfds);

        // 遍历链表设置 fd
        callback_node_t *node = callback_list_;
        while (node) {
            FD_SET(node->fd, &rfds);
            if (node->fd > maxfd) {
                maxfd = node->fd;
            }
            node = node->next;
        }

        int s = select(maxfd + 1, &rfds, NULL, NULL, &tv);

        if (s < 0) {
            ESP_LOGE(TAG, "Select failed: errno %d", errno);
            break;
        } else if (s == 0) {
            // ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
        } else {
            // 遍历链表检查并调用回调
            node = callback_list_;
            while (node) {
                if (FD_ISSET(node->fd, &rfds) && node->callback) {
                    node->callback();
                }
                node = node->next;
            }
        }
    }
}
