# 一款esp32自制遥控器

## 编译

```zsh
source /Volumes/unix/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
idf.py -p /dev/tty.usbmodem57340051551 flash
```