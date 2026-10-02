#ifndef _LW_AHCI_H
#define _LW_AHCI_H

#include "io.h"
#include "ata.h"

BYTE ahci_detect(PATA_DEV devs, BYTE max_devices);
int ahci_rw(
    PCATA_DEV d,    
    QWORD lba,
    DWORD sectors,
    PVOID buf,
    int write
);

#endif