/**
* @file		rl_virtual.hpp
* @brief	Virtual relays implementation, useful to test logic without hardware.
* @author	PiniponSelvagem
*
* Copyright(C) PiniponSelvagem
*
***********************************************************************
* Software that is described here, is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
**********************************************************************/

#pragma once

#ifndef _PINICORE_IO_RELAYS_VIRTUAL_H_
#define _PINICORE_IO_RELAYS_VIRTUAL_H_

#include <stdint.h>
#include "irelays.hpp"

class RelaysVirtual : public IRelays {
    public:
        /**
         * @brief   Check if a module is connected.
         * @param   module Module index.
         * @return  Module connected status.
         */
        bool isModuleConnected(uint8_t module) override;


    private:
        /**
         * @brief   Initializes the relay modules.
         * @note    It is guaranteed that 'm_modules' and 'm_relaysPerModule' are set before this
         *          function is called.
         */
        void initModules() override;

        /**
         * @brief   Sends the command to the hardware
         * @param   module The module id
         * @param   relay The relay id in the module
         * @param   state The state of the relay to be set to
         * @return  True if the relay changed to the new state, false otherwise.
         * @note    It is not necessary for this function to check if 'module' and 'relay' are within valid
         *          range, since \ref 'IRelays' class will do that check before calling it.
         */
        bool setHardware(uint8_t module, uint8_t relay, bool state) override;
};

#endif // _PINICORE_IO_RELAYS_VIRTUAL_H_