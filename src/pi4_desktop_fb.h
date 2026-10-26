/* Fixed native desktop surface -> firmware scanout. No mode-set or MMIO. */
#pragma once
#include <stdint.h>

#define PI4_DESKTOP_WIDTH 800u
#define PI4_DESKTOP_HEIGHT 600u
struct pi4_desktop_fb {
    uint32_t width, height, pitch, scale, left, top;
};
static int pi4_desktop_fb_plan(uint64_t width,uint64_t height,uint64_t pitch,
        unsigned bpp,struct pi4_desktop_fb *out) {
    if(!out || bpp!=32u || width<PI4_DESKTOP_WIDTH || height<PI4_DESKTOP_HEIGHT ||
        width>16384u || height>16384u || (pitch&3u) || pitch<width*4u ||
        pitch>16384u*4u)return 0;
    unsigned scale=(unsigned)(width/PI4_DESKTOP_WIDTH);
    if(scale>height/PI4_DESKTOP_HEIGHT)scale=(unsigned)(height/PI4_DESKTOP_HEIGHT);
    *out=(struct pi4_desktop_fb){(uint32_t)width,(uint32_t)height,(uint32_t)pitch,scale,
        ((uint32_t)width-PI4_DESKTOP_WIDTH*scale)/2u,
        ((uint32_t)height-PI4_DESKTOP_HEIGHT*scale)/2u};
    return 1;
}
/* The caller owns a validated plan and an exactly 800x600 packed surface.
 * Preserve physical row padding and borders; clear the borders once at setup. */
static void pi4_desktop_fb_present(volatile uint32_t *destination,
        const uint32_t *surface,const struct pi4_desktop_fb *p) {
    for(unsigned y=0;y<PI4_DESKTOP_HEIGHT;y++) {
        const uint32_t *src=surface+y*PI4_DESKTOP_WIDTH;
        for(unsigned dy=0;dy<p->scale;dy++) {
            volatile uint32_t *dst=destination+
                (uint64_t)(p->top+y*p->scale+dy)*(p->pitch/4u)+p->left;
            for(unsigned x=0;x<PI4_DESKTOP_WIDTH;x++)
                for(unsigned dx=0;dx<p->scale;dx++)dst[x*p->scale+dx]=src[x];
        }
    }
}
