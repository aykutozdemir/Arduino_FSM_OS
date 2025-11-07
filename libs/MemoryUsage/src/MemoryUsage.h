/**
 * @file MemoryUsage.h
 * @brief Memory usage monitoring utilities for Arduino.
 *
 * This file provides functions to monitor and analyze SRAM memory usage on AVR-based
 * Arduino boards. It helps in understanding memory allocation patterns and detecting
 * memory issues.
 *
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#ifndef __MemoryUsage_h__
#define __MemoryUsage_h__

#include "Arduino.h"
#include <stdint.h>

/**
 * @brief SRAM memory layout description
 *
 * Memory layout of an AVR-based Arduino:
 * \verbatim
 * +---------------+------------------+---------------------------------------------+-----------------+
 * |               |                  |                                             |                 |
 * |               |                  |                                             |                 |
 * |    static     |                  |                                             |                 |
 * |     data      |       heap       |                   free ram                  |      stack      |
 * |               |                  |                                             |                 |
 * |               |                  |                                             |                 |
 * |               |                  |                                             |                 |
 * +---------------+------------------+---------------------------------------------+-----------------+
 *        _end or __heap_start     __brkval                                         SP             RAMEND
 * \endverbatim
 *
 * Source: http://www.nongnu.org/avr-libc/user-manual/malloc.html
 */

/** @brief End of the data section, start of the heap */
extern uint8_t _end;

/** @brief Top of the stack */
extern uint8_t __stack;

/** @brief Current heap break value, indicating the top of the heap */
extern uint8_t *__brkval;

/** @brief Start of the data section */
extern uint8_t *__data_start;

/** @brief End of the data section */
extern uint8_t *__data_end;

/** @brief Start of the heap section */
extern uint8_t *__heap_start;

/** @brief End of the heap section */
extern uint8_t *__heap_end;

/** @brief Start of the BSS section */
extern uint8_t *__bss_start;

/** @brief End of the BSS section */
extern uint8_t *__bss_end;

/**
 * @brief Utility class for memory usage analysis
 *
 * Provides functions to monitor and analyze SRAM memory usage on AVR-based Arduino boards,
 * helping to identify memory issues and optimize memory consumption.
 */
class MemoryUsage
{
public:
    /**
     * @brief Paint the stack with a canary pattern
     *
     * Fills unused memory (between heap and stack) with a canary value (0xc5)
     * to help detect maximum stack usage.
     */
    static void stackPaint(void);

    /**
     * @brief Calculate the current free RAM
     *
     * Calculates the amount of free memory between heap and stack pointer.
     *
     * @return Number of free bytes
     */
    static int freeRam(void);

    /**
     * @brief Calculate the minimum free RAM that has occurred
     *
     * Uses the painted canary pattern to determine the minimum amount of free RAM
     * that has been available since stackPaint() was called.
     *
     * @return Minimum number of free bytes observed
     */
    static int minimumFreeRam(void);

    /**
     * @brief Display detailed RAM usage information
     *
     * Outputs detailed information about memory usage, including data, BSS,
     * heap, free RAM, and stack segments with their sizes and addresses.
     *
     * @param out Print stream to output information to (e.g., Serial)
     */
    static void ramDisplay(Print &out);
};

// Memory printing macros
#define MEMORY_PRINT_START { Serial.print(F("Data start:")); Serial.println((int) &__data_start); }
#define MEMORY_PRINT_HEAPSTART { Serial.print(F("Heap start:")); Serial.println((int)&__heap_start); }
#define MEMORY_PRINT_HEAPEND { Serial.print(F("Heap end:")); Serial.println(__brkval == 0 ? (int)&__heap_start : (int)__brkval); }
#define MEMORY_PRINT_STACKSTART { Serial.print(F("Stack start:")); Serial.println((int) SP); }
#define MEMORY_PRINT_END { Serial.print(F("Stack end:")); Serial.println((int) RAMEND); }
#define MEMORY_PRINT_HEAPSIZE { Serial.print(F("Heap size:")); Serial.println((int) (__brkval == 0 ? (int)&__heap_start : (int)__brkval) - (int)&__heap_start); }
#define MEMORY_PRINT_STACKSIZE { Serial.print(F("Stack size:")); Serial.println((int) RAMEND - (int)SP); }
#define MEMORY_PRINT_FREERAM { Serial.print(F("Free ram:")); Serial.println((int) SP - (int) (__brkval == 0 ? (int)&__heap_start : (int)__brkval)); }
#define MEMORY_PRINT_TOTALSIZE { Serial.print(F("SRAM size:")); Serial.println((int) RAMEND - (int) &__data_start); }

// Stack count macros
#define STACK_DECLARE unsigned int mu_stack_size = (RAMEND - SP);
#define STACK_COMPUTE { mu_stack_size = (RAMEND - SP) > mu_stack_size ? (RAMEND - SP) : mu_stack_size;}
#define STACK_PRINT_TEXT(text) { STACK_COMPUTE; Serial.print(text); Serial.println(mu_stack_size); }
#define STACK_PRINT STACK_PRINT_TEXT(F("Stack Maximum Size (Instrumentation method): "));

// Free RAM macros
#define FREERAM_PRINT_TEXT(text) Serial.print(text); Serial.println(MemoryUsage::freeRam());
#define FREERAM_PRINT FREERAM_PRINT_TEXT(F("Free Ram Size: "));

// Stack paint macros
#define STACKPAINT_PRINT_TEXT(text) { Serial.print(text); Serial.println(MemoryUsage::minimumFreeRam()); }
#define STACKPAINT_PRINT STACKPAINT_PRINT_TEXT(F("Stack Maximum Size (Painting method): "));

#endif
