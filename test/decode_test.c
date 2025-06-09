// Dummy test program template

#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

int main() {
    twig_dev_t *dev = twig_open();

    if (dev)
        twig_close(dev);
    return 0;
}