#include "uci.h"
#include "tt.h"
int main(void) {
    tt_init();
    uci_loop();
    return 0;
}
