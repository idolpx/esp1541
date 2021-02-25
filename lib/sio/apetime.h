#ifndef APETIME_H
#define APETIME_H

#include "sio.h"

class sioApeTime : public sioDevice
{
private:
    void _sio_get_time();

public:
    void sio_process(uint32_t commanddata, uint8_t checksum) override;
    virtual void sio_status() override {};
};

#endif // APETIME_H
