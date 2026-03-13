#ifndef _VIDEO_PROBE_H
#define _VIDEO_PROBE_H

#include <stdint.h>
#include <driver/pci.h>
#include <graphics.h>

bool video_probe_primary(video_device_t *dev);
bool intel_gpu_init(video_device_t *dev, pci_display_device_t *disp, uint16_t device_id);

#endif