#include <assert.h>
#include "pi4_audio_policy.h"

int main(void) {
    assert(pi4_hdmi_can_reuse_tone(1, 1, 60, 60, 1));
    assert(!pi4_hdmi_can_reuse_tone(1, 1, 60, 61, 1));
    assert(!pi4_hdmi_can_reuse_tone(1, 1, 60, 0, 1));
    assert(!pi4_hdmi_can_reuse_tone(0, 1, 60, 60, 1));
    assert(!pi4_hdmi_can_reuse_tone(1, 0, 60, 60, 1));
    assert(!pi4_hdmi_can_reuse_tone(1, 1, 60, 60, 0));
    assert(!pi4_hdmi_can_reuse_tone(1, 1, 60, 60, 0x101));
    return 0;
}
