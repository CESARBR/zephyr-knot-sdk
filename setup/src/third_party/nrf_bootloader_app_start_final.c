/*
 * Code based on Nordic's nRF5 SDK.
 * Base version: 15.2.0_9412b96
 *
 * The SDK can be downloaded at:
 * https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/
 *
 * The original file can be found, inside the zipped file, at:
 * components/libraries/bootloader/
 */
/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <flash.h>

// API to disable system clock
extern void sys_clock_disable(void);

#define HANDLER_MODE_EXIT 0xFFFFFFF9 // When this is jumped to, the CPU will exit interrupt context
                                     // (handler mode), and pop values from the stack into registers.
                                     // See ARM's documentation for "Exception entry and return".
#define EXCEPTION_STACK_WORD_COUNT 8 // The number of words popped from the stack when
                                     // HANDLER_MODE_EXIT is branched to.


/**@brief Function that sets the stack pointer and link register, and starts executing a particular address.
 *
 * @param[in]  new_msp  The new value to set in the main stack pointer.
 * @param[in]  new_lr   The new value to set in the link register.
 * @param[in]  addr     The address to execute.
 */
#if defined ( __CC_ARM )
__ASM __STATIC_INLINE void jump_to_addr(uint32_t new_msp, uint32_t new_lr, uint32_t addr)
{
    MSR MSP, R0;
    MOV LR,  R1;
    BX       R2;
}
#else
__STATIC_INLINE void jump_to_addr(uint32_t new_msp, uint32_t new_lr, uint32_t addr)
{
    __ASM volatile ("MSR MSP, %[arg]" : : [arg] "r" (new_msp));
    __ASM volatile ("MOV LR,  %[arg]" : : [arg] "r" (new_lr) : "lr");
    __ASM volatile ("BX       %[arg]" : : [arg] "r" (addr));
}
#endif


/**@brief Function for booting an app as if the chip was reset.
 *
 * @param[in]  vector_table_addr  The address of the app's vector table.
 */
__STATIC_INLINE void app_start(uint32_t vector_table_addr)
{
    const uint32_t current_isr_num = (__get_IPSR() & IPSR_ISR_Msk);
    const uint32_t new_msp         = *((uint32_t *)(vector_table_addr));                    // The app's Stack Pointer is found as the first word of the vector table.
    const uint32_t reset_handler   = *((uint32_t *)(vector_table_addr + sizeof(uint32_t))); // The app's Reset Handler is found as the second word of the vector table.
    const uint32_t new_lr          = 0xFFFFFFFF;

    __set_CONTROL(0x00000000);   // Set CONTROL to its reset value 0.
    __set_PRIMASK(0x00000000);   // Set PRIMASK to its reset value 0.
    __set_BASEPRI(0x00000000);   // Set BASEPRI to its reset value 0.
    __set_FAULTMASK(0x00000000); // Set FAULTMASK to its reset value 0.

    sys_clock_disable();
    if (current_isr_num == 0)
    {
        // The CPU is in Thread mode (main context).
        jump_to_addr(new_msp, new_lr, reset_handler); // Jump directly to the App's Reset Handler.
    }
    else
    {
        // The CPU is in Handler mode (interrupt context).

        const uint32_t exception_stack[EXCEPTION_STACK_WORD_COUNT] = // To be copied onto the stack.
        {
            0x00000000,    // New value of R0. Cleared by setting to 0.
            0x00000000,    // New value of R1. Cleared by setting to 0.
            0x00000000,    // New value of R2. Cleared by setting to 0.
            0x00000000,    // New value of R3. Cleared by setting to 0.
            0x00000000,    // New value of R12. Cleared by setting to 0.
            0xFFFFFFFF,    // New value of LR. Cleared by setting to all 1s.
            reset_handler, // New value of PC. The CPU will continue by executing the App's Reset Handler.
            xPSR_T_Msk,    // New value of xPSR (Thumb mode set).
        };
        const uint32_t exception_sp = new_msp - sizeof(exception_stack);

        memcpy((uint32_t *)exception_sp, exception_stack, sizeof(exception_stack)); // 'Push' exception_stack onto the App's stack.

        jump_to_addr(exception_sp, new_lr, HANDLER_MODE_EXIT); // 'Jump' to the special value to exit handler mode. new_lr is superfluous here.
                                                               // exception_stack will be popped from the stack, so the resulting SP will be the new_msp.
                                                               // Execution will continue from the App's Reset Handler.
    }
}

void nrf_bootloader_app_start_final(uint32_t vector_table_addr)
{
    // Run application
    app_start(vector_table_addr);
}
