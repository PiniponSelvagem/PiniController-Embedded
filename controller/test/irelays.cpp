#include "irelays.hpp"
#include "utils/log.hpp"

#define PINICORE_TAG_IRELAYS "pcore_irelays"

#define RELAYS_ON_COLOR  "\e[1;42m"
#define RELAYS_OFF_COLOR "\e[1;41m"
#define RELAYS_RST_COLOR "\e[0m"
#define RELAYS_ON_TXT    RELAYS_ON_COLOR  " ON" RELAYS_RST_COLOR
#define RELAYS_OFF_TXT   RELAYS_OFF_COLOR "OFF" RELAYS_RST_COLOR

void IRelays::init(uint8_t modules, uint8_t relaysPerModule) {
    p_modules = modules;
    p_relaysPerModule = relaysPerModule;
    initModules();
}

bool IRelays::set(uint8_t module, uint8_t relay, bool state) {
    if (module >= p_modules || relay >= p_relaysPerModule)
        return false;

    if (!isModuleConnected(module)) {
        LOG_W(PINICORE_TAG_IRELAYS, "Unable to update module %2d relay %2d to %s, module not found",
            module, relay, state ? RELAYS_ON_TXT : RELAYS_OFF_TXT
        );
        return false;
    }

    bool changedState = setHardware(module, relay, state);
    if (changedState) {
        updateActiveCount();
        LOG_I(PINICORE_TAG_IRELAYS, "Updated module %2d relay %2d to %s",
            module, relay, state ? RELAYS_ON_TXT : RELAYS_OFF_TXT
        );
    }
    return changedState;
}

bool IRelays::get(uint8_t module, uint8_t relay) {
    if (module >= p_modules || relay >= p_relaysPerModule)
        return false;

    uint16_t globalIndex = module * p_relaysPerModule + relay;

    if (globalIndex >= RELAYS_MAX)
        return false;

    uint16_t wordIndex = globalIndex / RELAYS_STATE_SIZE_MAX;
    uint8_t  bitIndex  = globalIndex % RELAYS_STATE_SIZE_MAX;

    if (wordIndex >= RELAYS_STATE_SIZE_MAX)
        return false;

    return (m_relaysState[wordIndex] >> bitIndex) & 0x1;
}

const int IRelays::getActiveCount() {
    for (int i=0; i<RELAYS_STATE_SIZE_MAX; ++i) {
        LOG_D(PINICORE_TAG_IRELAYS,
            "0x%4x 0x%4x 0x%4x 0x%4x 0x%4x 0x%4x 0x%4x 0x%4x",
            m_relaysState[0], m_relaysState[1], m_relaysState[2], m_relaysState[3],
            m_relaysState[4], m_relaysState[5], m_relaysState[6], m_relaysState[7]
        );
    }
    return m_relaysActiveCount;
}

void IRelays::onRelay(RelaysOnRelayCallback callback) {
    m_onRelayCallback = callback;
}
void IRelays::onModule(RelaysOnModuleCallback callback) {
    m_onModuleCallback = callback;
}



void IRelays::updateActiveCount() {
    m_relaysActiveCount = 0;
    for (int i=0; i<RELAYS_STATE_SIZE_MAX; ++i) {
        // Counts how many bits are '1' in the passed value
        m_relaysActiveCount += __builtin_popcount(m_relaysState[i]);
    }
}

void IRelays::_onRelay(uint8_t module, uint8_t relay, bool state) {
    if (m_onRelayCallback != NULL)
        m_onRelayCallback(module, relay, state);
}
void IRelays::_onModule(uint8_t module, bool state) {
    if (m_onModuleCallback != NULL)
        m_onModuleCallback(module, state);
}