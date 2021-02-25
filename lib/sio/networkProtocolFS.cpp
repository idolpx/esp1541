#include "networkProtocolFS.h"

bool networkProtocolFS::open(EdUrlParser *urlParser, cmdFrame_t *cmdFrame, enable_interrupt_t enable_interrupt)
{
    if (cmdFrame->aux1 & 4)
        canRead = true;

    if (cmdFrame->aux1 & 8)
        canWrite = true;

    if (cmdFrame->aux1 == 6)
        dirRead = true;

    return true;
}

bool networkProtocolFS::close(enable_interrupt_t enable_interrupt)
{
    return true;
}

bool networkProtocolFS::read(uint8_t *rx_buf, unsigned short len)
{
    return true;
}

bool networkProtocolFS::write(uint8_t *tx_buf, unsigned short len)
{
    return true;
}

bool networkProtocolFS::status(uint8_t *status_buf)
{
    return true;
}

bool networkProtocolFS::special(uint8_t *sp_buf, unsigned short len, cmdFrame_t *cmdFrame)
{
    return true;
}

bool networkProtocolFS::del(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFS::rename(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFS::mkdir(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFS::rmdir(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFS::note(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}

bool networkProtocolFS::point(EdUrlParser *urlParser, cmdFrame_t *cmdFrame)
{
    return false;
}
