#ifndef _INTEL_GPU_H
#define _INTEL_GPU_H

#include <stdint.h>
#include <driver/pci.h>
#include <driver/video_probe.h>

typedef struct {
    pci_addr_t addr;
    uint16_t vendor_id;
    uint16_t device_id;

    pci_bar_info_t mmio_bar;   // 候选 MMIO/GTTMMADR
    pci_bar_info_t aper_bar;   // 候选 GMADR/aperture

    void *mmio_virt;
    void *aper_virt;
} intel_gpu_info_t;

bool intel_gpu_probe(pci_display_device_t *disp, intel_gpu_info_t *out);

#endif