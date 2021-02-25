#ifndef DISK_H
#define DISK_H

#include "sio.h"
#include "diskType.h"

class sioDisk : public sioDevice
{
private:
    DiskType *_disk = nullptr;

    void sio_read();
    void sio_write(bool verify);
    void sio_format();
    void sio_status() override;
    void sio_process(uint32_t commanddata, uint8_t checksum) override;

    void derive_percom_block(uint16_t numSectors);
    void sio_read_percom_block();
    void sio_write_percom_block();
    void dump_percom_block();

public:
    disktype_t mount(FILE *f, const char *filename, uint32_t disksize, disktype_t disk_type = DISKTYPE_UNKNOWN);
    void unmount();
    bool write_blank(FILE *f, uint16_t sectorSize, uint16_t numSectors);

    disktype_t disktype() { return _disk == nullptr ? DISKTYPE_UNKNOWN : _disk->_disktype; };

    ~sioDisk();
};

#endif
