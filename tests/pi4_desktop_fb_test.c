#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/pi4_desktop_fb.h"

int main(void) {
    uint32_t *surface=malloc(800u*600u*4u);
    assert(surface);
    for(unsigned y=0;y<600u;y++)for(unsigned x=0;x<800u;x++)
        surface[y*800u+x]=0x10000000u|(y<<12)|x;
    const unsigned modes[][3]={{800,600,3200},{1024,768,4224},
        {1920,1080,7808},{1920,1200,7808},{3840,2160,15488}};
    for(unsigned n=0;n<sizeof(modes)/sizeof(*modes);n++) {
        struct pi4_desktop_fb p;
        unsigned width=modes[n][0],height=modes[n][1],pitch=modes[n][2];
        assert(pi4_desktop_fb_plan(width,height,pitch,32,&p));
        /* These larger firmware modes formerly fail the real compositor's
         * stride/rows guard. Its input must instead stay packed 800x600. */
        if(n)assert(width>800u || height>600u);
        assert(p.scale==(n==3u?2u:n==4u?3u:1u));
        size_t words=(size_t)pitch/4u*height;
        uint32_t *allocation=malloc((words+32u)*4u);
        assert(allocation);
        for(size_t i=0;i<words+32u;i++)allocation[i]=0xdeadbeefu;
        uint32_t *physical=allocation+16u;
        pi4_desktop_fb_present(physical,surface,&p);
        /* Verify every viewport pixel, centering border, row pad and guard.
         * A logical pitch accidentally used for scanout corrupts this case. */
        for(unsigned y=0;y<height;y++)for(unsigned x=0;x<pitch/4u;x++) {
            unsigned inside=x>=p.left && x<p.left+800u*p.scale &&
                y>=p.top && y<p.top+600u*p.scale;
            uint32_t expected=inside ? surface[((y-p.top)/p.scale)*800u+(x-p.left)/p.scale] : 0xdeadbeefu;
            assert(physical[(size_t)y*pitch/4u+x]==expected);
        }
        for(unsigned i=0;i<16u;i++)assert(allocation[i]==0xdeadbeefu &&
            allocation[16u+words+i]==0xdeadbeefu);
        free(allocation);
    }
    struct pi4_desktop_fb p={1,2,3,4,5,6},before=p;
    assert(!pi4_desktop_fb_plan(799,600,3200,32,&p));
    assert(!pi4_desktop_fb_plan(800,599,3200,32,&p));
    assert(!pi4_desktop_fb_plan(800,600,3196,32,&p));
    assert(!pi4_desktop_fb_plan(800,600,3201,32,&p));
    assert(!pi4_desktop_fb_plan(800,600,3200,24,&p));
    assert(!pi4_desktop_fb_plan(UINT64_MAX,600,3200,32,&p));
    assert(!pi4_desktop_fb_plan(800,600,UINT64_MAX,32,&p));
    assert(!pi4_desktop_fb_plan(800,600,3200,32,NULL));
    assert(p.width==before.width && p.height==before.height && p.pitch==before.pitch && p.scale==before.scale);
    free(surface);
    puts("PASS Pi native desktop packed surface, centered integer scaling, firmware pitch/borders/guards and invalid modes");
}
