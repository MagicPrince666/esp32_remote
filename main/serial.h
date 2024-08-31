#ifndef __SERIAL_H__
#define __SERIAL_H__

class Serial
{
private:
    int RX_BUF_SIZE;
public:
    Serial();
    ~Serial();

    int sendData(const char* data, const int lenght);

    int RecvData(char* data, const int lenght);
};

#endif
