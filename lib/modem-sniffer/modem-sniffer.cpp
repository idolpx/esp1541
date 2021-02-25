/**
 * modem sniffer library for esp1541
 * logs character streams from MODEM.
 */

#include "modem-sniffer.h"
#include "../../include/debug.h"

ModemSniffer::ModemSniffer(FileSystem *_fs, bool _enable)
{
    Debug_printf("ModemSniffer::ModemSniffer(%s)\n", _fs->typestring());

    if (_fs == nullptr)
        Debug_printf("_fs is NULL!\n");

    activeFS = _fs;
    direction = INIT;
    enable = _enable;
}

ModemSniffer::~ModemSniffer()
{
    Debug_printf("ModemSniffer::~ModemSniffer()\n");

    if (_file != nullptr)
    {
        Debug_printf("Closing" SNIFFER_OUTPUT_FILE "\n");
        fclose(_file);
        _file = nullptr;
    }
}

size_t ModemSniffer::getOutputSize()
{
    if (_file != nullptr)
        return FileSystem::filesize(_file);

    long result = FileSystem::filesize(SNIFFER_OUTPUT_FILE);

    return result == -1 ? 0 : result;
}

void ModemSniffer::closeOutput()
{
    Debug_print("ModemSniffer::closeOutput\n");

    if (_file == nullptr)
    {
        _file = activeFS->file_open(SNIFFER_OUTPUT_FILE, "r+"); // Seeks don't work right if we use "append" mode - use "r+"
        
        if (_file == nullptr)
        {
            Debug_printf("Error opening sniffer output: %d\n", errno);
            return;
        }

        fseek(_file, 0, SEEK_END);
    }

    // Close the file
    fflush(_file);
    fclose(_file);
    _file = nullptr;
}

FILE *ModemSniffer::closeOutputAndProvideReadHandle()
{
    Debug_print("ModemSniffer::closeOutputAndProvideReadHandle()\n");

    closeOutput();
    FILE *result = activeFS->file_open(SNIFFER_OUTPUT_FILE); // read-only.
    if (result == nullptr)
        Debug_printf("Error opening sniffer output: %d\n", errno);

    return result;
}

void ModemSniffer::restartOutput()
{
    if (_file != nullptr)
        fclose(_file);

    _file = activeFS->file_open(SNIFFER_OUTPUT_FILE, "w"); // This should create/truncate the file

    Debug_printf("ModemSniffer::restartOutput(%p)\n", _file);
}

void ModemSniffer::dumpInput(uint8_t *buf, unsigned short len)
{
    if (enable == false)
        return;

    if (_file == nullptr)
    {
        restartOutput();
    }

    if (direction != INPUT)
    {
        fprintf(_file, "\n\nINCOMING: ");
        Debug_printf("\n\nINCOMING: ");
    }

    direction = INPUT;

    for (int i = 0; i < len; i++)
    {
        if (buf[i] > 0x20 && buf[i] < 0x7F)
        {
            // Printable ASCII character.
            fprintf(_file, "'%c' ", buf[i]);
            Debug_printf("'%c' ", buf[i]);
        }
        else
        {
            // non-printable ASCII character.
            fprintf(_file, "%02x ", buf[i]);
            Debug_printf("%02x ", buf[i]);
        }
    }
    fflush(_file);
}

void ModemSniffer::dumpOutput(uint8_t *buf, unsigned short len)
{
    if (enable == false)
        return;

    if (_file == nullptr)
    {
        restartOutput();
    }

    if (direction != OUTPUT)
    {
        fprintf(_file, "\n\nOUTGOING: ");
        Debug_printf("\n\nOUTGOING: ");
    }

    direction = OUTPUT;

    for (int i = 0; i < len; i++)
    {
        if (buf[i] > 0x20 && buf[i] < 0x7F)
        {
            // Printable ASCII character.
            fprintf(_file, "'%c' ", buf[i]);
            Debug_printf("'%c' ", buf[i]);
        }
        else
        {
            // non-printable ASCII character.
            fprintf(_file, "%02X ", buf[i]);
            Debug_printf("%02X ", buf[i]);
        }
    }
    fflush(_file);
}