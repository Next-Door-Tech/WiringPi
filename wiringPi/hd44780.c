/*
 * hd44780.c:
 *  Extend wiringPi with the HD44780 Dot-Matrix LCD Controller
 *  Copyright (c) 2025 Grazer Computer Club and contributors
 ***********************************************************************
 * This file is part of wiringPi:
 *      https://github.com/WiringPi/WiringPi
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include "wiringPi.h"

#include "hd44780.h"

#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdbit.h>
#include <string.h>
#include <time.h>

#define LIKELY(cond) __builtin_expect((cond), true)
#define UNLIKELY(cond) __builtin_expect((cond), false)

const int enable_duration = 300;    // 300 ns

const struct timespec standard_busy_delay = {0, 37000};  // 37 us
const struct timespec extended_busy_delay = {0, 41000};  // 37 us + 4 us for address counter index
const struct timespec reset_busy_delay = {0, 1520000};   // 1.52 ms

[[gnu::nonnull(1,2)]] static inline struct timespec timespec_add(const struct timespec * lhs, const struct timespec * rhs, struct timespec * sum) {
    if (sum == NULL) {
        struct timespec retval;
        sum = &retval;
    }

    sum->tv_sec = lhs->tv_sec + rhs->tv_sec + (lhs->tv_nsec + rhs->tv_nsec) / 1000000000l;
    sum->tv_nsec = (lhs->tv_nsec + rhs->tv_nsec) % 1000000000l;

    return *sum;
};

[[gnu::nonnull]] static inline bool timespec_gt(const struct timespec * lhs, const struct timespec * rhs) {
    return (lhs->tv_sec == rhs->tv_sec ? lhs->tv_nsec > rhs->tv_nsec : lhs->tv_sec > rhs->tv_sec);
};

struct hd44780DataStruct {
    // Initialization flags
    const bool readEnabled;     // indicates whether read pin is enabled
    const bool mode8Enabled;    // indicates whether 8-bit data bus is enabled
                                // otherwise, only 4-bit mode is valid

    // Internal state flags
    bool mode8;     // indicates 8-bit mode is active (if enabled)

    // Pins
    const int pinRS;     // Register select pin (required)
    const int pinRW;     // Read/Write pin (valid only if readEnabled == true)
    const int pinE;      // Strobe/E pin (required)
    const int pinDB[8];  // Data pins DB0-DB7 (DB0-DB3 valid only if mode8Enabled == true)

    struct timespec operation_end;
};

static inline void setStatusPins(struct hd44780DataStruct * nodeData, bool RS, bool RW);

static inline void writeCycleStart(const int pinE);
static inline void writeCycleEnd(const int pinE);
static inline void writeCycleEndShort(const int pinE);

static inline void readCycleStart(const int pinE);
static inline void readCycleEnd(const int pinE);
static inline void readCycleEndShort(const int pinE);

static inline void waitWhileBusy_8(struct hd44780DataStruct * nodeData);
static inline void waitWhileBusy_4(struct hd44780DataStruct * nodeData);
static inline void waitWhileBusy_ReadDisabled(struct hd44780DataStruct * nodeData);

static void hd44780DigitalWrite_8(struct wiringPiNodeStruct *node, int pin, int data);
static void hd44780DigitalWrite_4(struct wiringPiNodeStruct *node, int pin, int data);

static int hd44780DigitalRead_8(struct wiringPiNodeStruct *node, int pin);
static int hd44780DigitalRead_4(struct wiringPiNodeStruct *node, int pin);

static inline void setStatusPins(struct hd44780DataStruct * nodeData, bool RS, bool RW) {

    digitalWrite(nodeData->pinRS, RS);
    if (nodeData->readEnabled) {
        digitalWrite(nodeData->pinRW, RW);
    }

    delayNanoseconds(50);

}


static inline void writeCycleStart(const int pinE) {
    digitalWrite(pinE, HIGH);
}


static inline void writeCycleEnd(const int pinE) {
    delayNanoseconds(enable_duration);
    digitalWrite(pinE, LOW);
    delayNanoseconds(enable_duration);
}


static inline void writeCycleEndShort(const int pinE) {
    delayNanoseconds(enable_duration);
    digitalWrite(pinE, LOW);
}


static inline void readCycleStart(const int pinE) {
    digitalWrite(pinE, HIGH);
    delayNanoseconds(enable_duration);
}


static inline void readCycleEnd(const int pinE) {
    digitalWrite(pinE, LOW);
    delayNanoseconds(enable_duration);
}


static inline void readCycleEndShort(const int pinE) {
    digitalWrite(pinE, LOW);
}


static inline void waitWhileBusy_8(struct hd44780DataStruct * nodeData) {

    if (nodeData->readEnabled) {

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        if (UNLIKELY(((nodeData->operation_end.tv_sec - now.tv_sec) * 1000000000l + (nodeData->operation_end.tv_nsec - now.tv_nsec)) > 100000l)) {
            // if expected operation end is very far away (>100us, should only happen after sending return home instruction)
            clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &(nodeData->operation_end), NULL);
        }

        pinMode(nodeData->pinDB[7], INPUT);

        setStatusPins(nodeData, LOW, HIGH);
        readCycleStart(nodeData->pinE);

        // check busy flag until it has cleared
        while (digitalRead(nodeData->pinDB[7]) != LOW) {
            readCycleEnd(nodeData->pinE);
            readCycleStart(nodeData->pinE);
        }

        pinMode(nodeData->pinDB[7], OUTPUT);
        readCycleEnd(nodeData->pinE);
    } else {
        waitWhileBusy_ReadDisabled(nodeData);
    }

}


static inline void waitWhileBusy_4(struct hd44780DataStruct * nodeData) {

    if (nodeData->readEnabled) {

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        if (UNLIKELY(((nodeData->operation_end.tv_sec - now.tv_sec) * 1000000000l + (nodeData->operation_end.tv_nsec - now.tv_nsec)) > 100000l)) {
            // if expected operation end is very far away (>100us, should only happen after sending return home instruction)
            clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &(nodeData->operation_end), NULL);
        }

        pinMode(nodeData->pinDB[7], INPUT);

        setStatusPins(nodeData, LOW, HIGH);
        readCycleStart(nodeData->pinE);

        // check busy flag until it has cleared
        while (digitalRead(nodeData->pinDB[7]) != LOW) {
            readCycleEnd(nodeData->pinE);

            // full cycle, ignoring second data nibble
            readCycleStart(nodeData->pinE);
            readCycleEnd(nodeData->pinE);

            readCycleStart(nodeData->pinE);
        }

        pinMode(nodeData->pinDB[7], OUTPUT);
        readCycleEnd(nodeData->pinE);

        // full cycle, ignoring second data nibble
        readCycleStart(nodeData->pinE);
        readCycleEnd(nodeData->pinE);
    } else {
        waitWhileBusy_ReadDisabled(nodeData);
    }

}


static inline void waitWhileBusy_ReadDisabled(struct hd44780DataStruct * nodeData) {
    struct timespec tsNow;

    clock_gettime(CLOCK_MONOTONIC_RAW, &tsNow);

    if (timespec_gt(&(nodeData->operation_end), &tsNow)) {
        while (clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &(nodeData->operation_end), NULL)) {
            // do nothing until sleep duration has passed.
        }
    }
}


static void hd44780DigitalWrite_8(struct wiringPiNodeStruct *node, int pin, int data) {

    struct hd44780DataStruct * const nodeData = node->dataStruct;
    const uint8_t data8 = data;

    const bool RS = (pin == node->pinBase);

    if (UNLIKELY(!(nodeData->mode8))) {
        // we are in mode4
        if (UNLIKELY(!RS && (data8 & 0xF0) == 0x30)) {
            // we are in mode4, and sending an instruction to switch to mode8
            nodeData->mode8 = true;
        }

        hd44780DigitalWrite_4(node, pin, data8);
        return;
    }

    if (UNLIKELY(!RS && (data8 & 0xF0) == 0x20)) {
        // we are in mode8, and sending an instruction to switch to mode4
        nodeData->mode8 = false;
    }

    waitWhileBusy_8(nodeData);

    setStatusPins(nodeData, RS, false);
    writeCycleStart(nodeData->pinE);

    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(nodeData->pinDB[i], (data8 >> i) & 0b1);
    }

    writeCycleEnd(nodeData->pinE);

    // set expected end time
    struct timespec now;
    const struct timespec * delay;

    if (RS) {
        // writing to CGRAM/DDRAM; add 4 us to allow address counter to index
        delay = &extended_busy_delay;
    } else if ((data8 & 0xFF) > 0x03) {
        // standard control instruction
        delay = &standard_busy_delay;
    } else {
        // reset control instruction
        delay = &reset_busy_delay;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec_add(&now, delay, &(nodeData->operation_end));

}


static void hd44780DigitalWrite_4(struct wiringPiNodeStruct *node, int pin, int data) {

    struct hd44780DataStruct * const nodeData = node->dataStruct;
    const uint8_t data8 = data;

    const bool RS = (pin == node->pinBase);

    waitWhileBusy_4(nodeData);

    setStatusPins(nodeData, RS, false);
    writeCycleStart(nodeData->pinE);

    for (uint8_t i = 4; i < 8; ++i) {
        digitalWrite(nodeData->pinDB[i], (data8 >> i) & 0b1);
    }

    writeCycleEnd(nodeData->pinE);
    writeCycleStart(nodeData->pinE);

    for (uint8_t i = 4; i < 8; ++i) {
        digitalWrite(nodeData->pinDB[i], (data8 >> (i - 4)) & 0b1);
    }

    writeCycleEnd(nodeData->pinE);

    // set expected end time
    struct timespec now;
    const struct timespec * delay;

    if (RS) {
        // writing to CGRAM/DDRAM; add 4 us to allow address counter to index
        delay = &extended_busy_delay;
    } else if ((data8 & 0xFF) > 0x03) {
        // standard control instruction
        delay = &standard_busy_delay;
    } else {
        // reset control instruction
        delay = &reset_busy_delay;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec_add(&now, delay, &(nodeData->operation_end));

}



static int hd44780DigitalRead_8(struct wiringPiNodeStruct *node, int pin) {

    uint8_t data = 0;

    struct hd44780DataStruct * const nodeData = node->dataStruct;

    if (UNLIKELY(!(nodeData->mode8))) {
        return hd44780DigitalRead_4(node, pin);
    }

    if (pin == node->pinBase) {
        // read from CGRAM/DDRAM
        [[gnu::assume(nodeData->readEnabled == true)]];
        waitWhileBusy_8(nodeData);
        setStatusPins(nodeData, true, true);
    } else {
        // read busy flag & address counter
        setStatusPins(nodeData, false, true);
    }

    for (unsigned int i=0; i<8; ++i) {
        pinMode(nodeData->pinDB[i], INPUT);
    }

    readCycleStart(nodeData->pinE);

    for (int i=7; i>=0; --i) {
        data = (data << 1) | digitalRead(nodeData->pinDB[i]);
    }
    readCycleEnd(nodeData->pinE);

    for (unsigned int i=0; i<8; ++i) {
        pinMode(nodeData->pinDB[i], OUTPUT);
    }

    return data;

}

static int hd44780DigitalRead_4(struct wiringPiNodeStruct *node, int pin) {

    struct hd44780DataStruct * const nodeData = node->dataStruct;

    uint8_t data = 0;

    for (unsigned int i=4; i<8; ++i) {
        pinMode(nodeData->pinDB[i], INPUT);
    }

    if (pin == node->pinBase) { // read from CGRAM/DDRAM

        [[gnu::assume(nodeData->readEnabled == true)]];
        waitWhileBusy_4(nodeData);
        setStatusPins(nodeData, true, true);

    } else { // read busy flag & address counter

        setStatusPins(nodeData, false, true);

    }

    readCycleStart(nodeData->pinE);

    for (unsigned int i=7; i>=4; --i) {
        data = (data << 1) | (bool)digitalRead(nodeData->pinDB[i]);
    }

    readCycleEnd(nodeData->pinE);
    readCycleStart(nodeData->pinE);

    for (unsigned int i=7; i>=4; --i) {
        data = (data << 1) | (bool)digitalRead(nodeData->pinDB[i]);
    }

    readCycleEnd(nodeData->pinE);

    for (unsigned int i=4; i<8; ++i) {
        pinMode(nodeData->pinDB[i], OUTPUT);
    }

    return data;

}

// Helper function to set pins to default values.
static inline void setupPin(int pin) {
    pullUpDnControl(pin, PUD_UP);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

extern int hd44780Setup(const int pinBase,
                        const bool readEnabled, const bool mode8Enabled,
                        const int pinRS, const int pinRW, const int pinE,
                        const int pinDB7, const int pinDB6, const int pinDB5, const int pinDB4,
                        const int pinDB3, const int pinDB2, const int pinDB1, const int pinDB0) {

    {  // Check preconditions
        bool fail_preconditions = false;

        // macro required to pass name of pin to fprintf
        #define CHECK_PIN(id) do {                                      \
            if (id < 0 || (id >= 64 && !wiringPiFindNode(id))) {        \
                fprintf(stderr, "%s%i\n",                               \
                     "Error: invalid pin number for " #id ": ", id);    \
                fail_preconditions = true;                              \
            }                                                           \
        } while (0);

        CHECK_PIN(pinRS);
        if (readEnabled) {
            CHECK_PIN(pinRW);
        }
        CHECK_PIN(pinE);

        CHECK_PIN(pinDB7);
        CHECK_PIN(pinDB6);
        CHECK_PIN(pinDB5);
        CHECK_PIN(pinDB4);
 
        if (mode8Enabled) {
            CHECK_PIN(pinDB3);
            CHECK_PIN(pinDB2);
            CHECK_PIN(pinDB1);
            CHECK_PIN(pinDB0);
        }

        if (fail_preconditions) {
            fputs("hd44780Setup() failed.\n", stderr);
            return EXIT_FAILURE;
        }

        #undef CHECK_PIN
    }

    struct wiringPiNodeStruct *node;
    if (!(node = wiringPiNewNode(pinBase, 2))) {
        fputs("Error: unable to allocate WiringPi node.\n", stderr);
    }

    if ((node->dataStruct = malloc(sizeof(struct hd44780DataStruct)))) {
        // memcpy required as a struct with const members cannot be
        // reassigned after allocation (including via indirection)
        struct hd44780DataStruct tmpStruct = {
            .readEnabled = readEnabled, .mode8Enabled = mode8Enabled, .mode8 = mode8Enabled,
            .pinRS = pinRS, .pinRW = pinRW, .pinE = pinE,
            .pinDB = {pinDB0,  pinDB1,  pinDB2,  pinDB3, pinDB4,  pinDB5,  pinDB6,  pinDB7},
            .operation_end = {0}
        };
        memcpy(node->dataStruct, &tmpStruct, sizeof(struct hd44780DataStruct));
    } else {
        fputs("Error: unable to allocate node data structure.\n", stderr);
        return EXIT_FAILURE;
    }

    // setup pins
    setupPin(pinRS);
    if (readEnabled) {
        setupPin(pinRW);
    }
    setupPin(pinE);

    setupPin(pinDB7);
    setupPin(pinDB6);
    setupPin(pinDB5);
    setupPin(pinDB4);

    if (mode8Enabled) {
        setupPin(pinDB3);
        setupPin(pinDB2);
        setupPin(pinDB1);
        setupPin(pinDB0);

        node->digitalWrite = &hd44780DigitalWrite_8;
        if (readEnabled) {
            node->digitalRead = &hd44780DigitalRead_8;
        }
    } else {
        // set data bus to 4-bit

        writeCycleStart(pinE);
        digitalWrite(pinDB5, HIGH);
        writeCycleEnd(pinE);
        digitalWrite(pinDB5, LOW);
        delayMicroseconds(37);

        node->digitalWrite = &hd44780DigitalWrite_4;
        if (readEnabled) {
            node->digitalRead = &hd44780DigitalRead_4;
        }
    }

    return EXIT_SUCCESS;
}


/*******************************
 * Command Instruction Helpers *
 *******************************/

extern inline int hd44780CMD_Clear(void) {
    return 0x01;
}

extern inline int hd44780CMD_Home(void) {
    return 0x02;
}

extern inline int hd44780CMD_EntryMode(bool decrement, bool shift) {
    return 0x04 | decrement * 0x02 | shift * 0x01;
}

extern inline int hd44780CMD_OnOff(bool display, bool cursor, bool blink) {
    return 0x08 | display * 0x04 | cursor * 0x02 | blink * 0x01;
}

extern inline int hd44780CMD_Shift(bool cursorDisplay, bool leftRight) {
    return 0x10 | cursorDisplay * 0x08 | leftRight * 0x04;
}

extern inline int hd44780CMD_FnSet(bool dataLength, bool displayLines, bool font) {
    return 0x20 | dataLength * 0x10 | displayLines * 0x08 | font * 0x04;
}

extern inline int hd44780CMD_SetCGRAM(uint8_t address) {
    return 0x40 | (address & 0x3F);
}

extern inline int hd44780CMD_SetDDRAM(uint8_t address) {
    return 0x80 | (address & 0x7F);
}