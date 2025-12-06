#include "rl_virtual.hpp"
#include "utils/log.hpp"
#include "Arduino.h"

#define PINICORE_TAG_RELAYS_VIRTUAL "pcore_relaysvirtual"


bool RelaysVirtual::isModuleConnected(uint8_t module) {
    return (module < p_modules);
}

void RelaysVirtual::initModules() {
    LOG_I(PINICORE_TAG_RELAYS_VIRTUAL,
        "Virtual relays configured (%d): [modules: %d] [relaysPerModule: %d]",
        (p_modules*p_relaysPerModule), p_modules, p_relaysPerModule
    );
}

bool RelaysVirtual::setHardware(uint8_t module, uint8_t relay, bool state) {
    bool currState = get(module, relay);
    if (currState != state) {
        delay(10);  // Simulate hardware access (10ms)
        return true;
    }
    return false;
}