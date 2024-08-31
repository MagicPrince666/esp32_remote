#include "select.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "select";

Select::Select() {
    loop_quit_ = false;
    xTaskCreate(Select::select_task, "select_task", 4 * 1024, this, 5, NULL);
}

Select::~Select() {}

void Select::select_task(void *arg)
{
    Select *select = (Select *)arg;
    select->Loop();
}

void Select::AddFd(int fd, std::function<void()> handler)
{
    callback_map_[fd] = handler;
}

void Select::Loop() {
    fd_set rfds;
    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0,
    };

    while (!loop_quit_) {
        int maxfd = 0;
        FD_ZERO(&rfds);
        for (auto fds : callback_map_) {
            FD_SET(fds.first, &rfds);
            if (fds.first > maxfd) { /* maxfd 为最大值  */
                maxfd = fds.first;
            }
        }

        int s = select(maxfd + 1, &rfds, NULL, NULL, &tv);

        if (s < 0) {
            ESP_LOGE(TAG, "Select failed: errno %d", errno);
            break;
        } else if (s == 0) {
            ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
        } else {
            for (auto fds : callback_map_) {
                if (FD_ISSET(fds.first, &rfds)) {
                    fds.second();
                }
            }
        }
    }
}