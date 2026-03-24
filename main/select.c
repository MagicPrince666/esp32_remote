#include "select.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "select";
bool loop_quit_;
static void select_task(void *arg);

void SelectInit() {
    loop_quit_ = false;
    xTaskCreate(select_task, "select_task", 4 * 1024, NULL, 5, NULL);
}

void select_task(void *arg)
{
    Loop();
}

void AddFd(int fd, callback_t handler)
{
    // callback_map_[fd] = handler;
}

void DeleteFd(int fd)
{
    // if (callback_map_.count(fd)) {
    //     callback_map_.erase(fd);
    // }
}

void Loop() {
    fd_set rfds;
    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0,
    };

    while (!loop_quit_) {
        int maxfd = 0;
        FD_ZERO(&rfds);
        // for (auto fds : callback_map_) {
        //     FD_SET(fds.first, &rfds);
        //     if (fds.first > maxfd) { /* maxfd 为最大值  */
        //         maxfd = fds.first;
        //     }
        // }

        int s = select(maxfd + 1, &rfds, NULL, NULL, &tv);

        if (s < 0) {
            ESP_LOGE(TAG, "Select failed: errno %d", errno);
            break;
        } else if (s == 0) {
            ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
        } else {
            // for (auto fds : callback_map_) {
            //     if (FD_ISSET(fds.first, &rfds)) {
            //         fds.second();
            //     }
            // }
        }
    }
}