#ifndef __SELECT_H__
#define __SELECT_H__

#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include <functional>
#include <string>
#include <unordered_map>

#define MY_SELECT Select::Instance()

class Select {
public:
    static Select& Instance() {
        static Select instance;
        return instance;
    }
    ~Select();

    void AddFd(int fd, std::function<void()> handler);

    void DeleteFd(int fd);

private:
    bool loop_quit_;
    // 回调列表
    std::unordered_map<int, std::function<void()>> callback_map_;

    Select();
    void Loop();
    static void select_task(void *arg);
};

#endif
