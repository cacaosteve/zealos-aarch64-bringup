/* Optional macOS test runner for the QEMU framework bundled with UTM.
 * Build with build-utm-qemu-runner.sh; it does not control UTM's VMs. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *framework = getenv("UTM_QEMU_FRAMEWORK");
    if (!framework)
        framework = "/Applications/UTM.app/Contents/Frameworks/"
                    "qemu-aarch64-softmmu.framework/qemu-aarch64-softmmu";
    void *handle = dlopen(framework, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "Cannot load UTM QEMU: %s\n", dlerror());
        return 1;
    }
    void (*init)(int, char **) = dlsym(handle, "qemu_init");
    int (*loop)(void) = dlsym(handle, "qemu_main_loop");
    void (*cleanup)(void) = dlsym(handle, "qemu_cleanup");
    if (!init || !loop || !cleanup) {
        fprintf(stderr, "UTM QEMU entry point missing: %s\n", dlerror());
        return 1;
    }
    init(argc, argv);
    int result = loop();
    cleanup();
    return result;
}
