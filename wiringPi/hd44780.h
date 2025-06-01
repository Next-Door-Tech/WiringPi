/*
 * hd44780.h:
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Sets up an interface to a hd44780-compatible dot-matrix LCD controller.
 * Sets data bus width to 4 bits if specified, but performs no further communication
 * with the chip.
 *
 * Use `digitalWrite(pinBase)` to write characters to CGRAM/DDRAM and use
 * `digitalWrite(pinBase + 1)` to send control commands.
 * If the read pin is enabled, use `digitalRead(pinBase)` to read from CGRAM/DDRAM and
 * use `digitalRead(pinBase + 1)` to check the busy flag and address counter.
 *
 * @param pinBase Base pseudo pin used to interface with the controller.
 * 
 * @param readEnabled Enables read operations and the RW pin.
 * If disabled, the chip's RW pin should be tied to ground.
 * 
 * @param mode8Enabled Enables 8-bit data bus mode and pins DB3-DB0.
 * If disabled, the chip's pins DB3-DB0 should be tied to ground.
 * 
 * @param fontSize Character font height in pixels.
 * Should be 8, 10, or 11, depending on the LCD itself.
 * 
 * @param pinRS Register Select pin.
 * @param pinRW Read/Write pin. Ignored if readEnabled is false.
 * @param pinE  Enable/Strobe Pin.
 * @param pinDB7,pinDB6,pinDB5,pinDB4,pinDB3,pinDB2,pinDB1,pinDB0
 * Data bus pins.
 * PinDB3-pinDB0 are ignored if mode8Enabled is false.
 * @return 0 on success, nonzero on failure.
 */
extern int hd44780Setup(const int pinBase,
    const bool readEnabled, const bool mode8Enabled,
    const int pinRS, const int pinRW, const int pinE,
    const int pinDB7, const int pinDB6, const int pinDB5, const int pinDB4,
    const int pinDB3, const int pinDB2, const int pinDB1, const int pinDB0);

/**
 * @brief Clears entire display and sets DDRAM address 0 in address counter.
 */
extern int hd44780CMD_Clear(void) __attribute__((__const__));

/**
 * @brief Sets DDRAM address 0 in address counter. Also returns display from being
 * shifted to original position. DDRAM contents remain unchanged.
 */
extern int hd44780CMD_Home(void) __attribute__((__const__));

/**
 * @brief Sets cursor move direction and specifies display shift.
 * These operations are performed during data write and read.
 * @param decrement Cursor decrements rather than increments after write/read.
 * @param shift Entire display shifts instead of the cursor when characters are written.
 * Direction depends on the value of decrement.
 */
extern int hd44780CMD_EntryMode(bool decrement, bool shift) __attribute__((__const__));

/**
 * @brief Sets entire display on/off, cursor on/off, and blinking of cursor position character.
 * @param display Sets entire display on/off
 * @param cursor Sets cursor visibility on/off
 * @param blink Enables cursor blinking
 */
extern int hd44780CMD_OnOff(bool display, bool cursor, bool blink) __attribute__((__const__));

/**
 * @brief Moves cursor and shifts display without changing DDRAM contents.
 * @param cursorDisplay false: shifts cursor; true: shifts entire display
 * @param leftRight false: shift left; true: shift right
 */
extern int hd44780CMD_Shift(bool cursorDisplay, bool leftRight) __attribute__((__const__));

/**
 * @brief Sets interface data length, number of display lines, and character font.
 * @param dataLength false: 4-bit data length; true: 4-bit data length
 * @param displayLines false: 1-line display mode; true: 2-line display mode
 * @param font false: 8-pixel font height; true: 10- or 11-pixel font height;
 */
extern int hd44780CMD_FnSet(bool dataLength, bool displayLines, bool font) __attribute__((__const__));

/**
 * @brief Sets CGRAM address. CGRAM data is sent and received after this setting.
 * @param address CGRAM address to move cursor to. (0x00-0x3F)
 */
extern int hd44780CMD_SetCGRAM(uint8_t address) __attribute__((__const__));

/**
 * @brief Sets DDRAM address. DDRAM data is sent and received after this setting.
 * @param address DDRAM address to move cursor to. (0x00-0x7F)
 */
extern int hd44780CMD_SetDDRAM(uint8_t address) __attribute__((__const__));

#ifdef __cplusplus
}
#endif