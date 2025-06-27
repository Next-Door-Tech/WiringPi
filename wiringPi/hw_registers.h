/*
 * wiringPi:
 *  Arduino look-a-like Wiring library for the Raspberry Pi
 *  Copyright (c) 2012-2025 Gordon Henderson and contributors
 ***********************************************************************
 * This file is part of wiringPi:
 *    https://github.com/WiringPi/WiringPi
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdint.h>

union [[gnu::packed]] BCM_PWM_registers {

  // Note: The channel numbers used differ from the peripherals documentation for the BCM283X / BCM2711 chips.
  //    WiringPi uses channel numbers 0 and 1, whereas the hardware documentation uses channel numbers 1 and 2.
  //    Structure members have been named using the WiringPi convention, but the Broadcom names are listed
  //    in parentheses in the comments.

  struct { // Individually named registers

    union { // CTL : PWM Control
      volatile uint32_t CTRL;  // Full register
      struct BCM_PWM_CTRL_FIELDS {
        volatile uint32_t CHAN0_ENABLE    : 1;  // Channel 0 Enable (PWEN1)
        volatile uint32_t CHAN0_MODE      : 1;  // Channel 0 Mode (MODE1)
        volatile uint32_t CHAN0_REPEAT    : 1;  // Channel 0 Repeat Last Data (RPTL1)
        volatile uint32_t CHAN0_SILENCE   : 1;  // Channel 0 Silence Bit (SBIT1)
        volatile uint32_t CHAN0_POLARITY  : 1;  // Channel 0 Polarity (POLA1)
        volatile uint32_t CHAN0_FIFO      : 1;  // Channel 0 Use FIFO (USEF1)
        volatile uint32_t CLEAR_FIFO      : 1;  // Clear FIFO (CLRF)
        volatile uint32_t CHAN0_MS_ENABLE : 1;  // Channel 0 M/S Enable (MSEN1)
        volatile uint32_t CHAN1_ENABLE    : 1;  // Channel 0 Enable (PWEN2)
        volatile uint32_t CHAN1_MODE      : 1;  // Channel 0 Mode (MODE2)
        volatile uint32_t CHAN1_REPEAT    : 1;  // Channel 0 Repeat Last Data (RPTL2)
        volatile uint32_t CHAN1_SILENCE   : 1;  // Channel 0 Silence Bit (SBIT2)
        volatile uint32_t CHAN1_POLARITY  : 1;  // Channel 0 Polarity (POLA2)
        volatile uint32_t CHAN1_FIFO      : 1;  // Channel 0 Use FIFO (USEF2)
        volatile uint32_t                 : 1;  // Reserved
        volatile uint32_t CHAN1_MS_ENABLE : 1;  // Channel 0 M/S Enable (MSEN2)
        volatile uint32_t                 : 16; // Reserved
      } CTRL_FIELD;
      struct BCM_PWM_CTRL_CHAN_FIELDS {
        volatile uint8_t ENABLE     : 1;  // Enable (PWEN1/2)
        volatile uint8_t MODE       : 1;  // Mode (MODE1/2)
        volatile uint8_t REPEAT     : 1;  // Repeat Last Data (RPTL1/2)
        volatile uint8_t SILENCE    : 1;  // Silence Bit (SBIT1/2)
        volatile uint8_t POLARITY   : 1;  // Polarity (POLA1/2)
        volatile uint8_t USE_FIFO   : 1;  // Use FIFO (USEF1/2)
        volatile uint8_t            : 1;  // Reserved (really CLRF for CHAN[0], reserved for CHAN[1])
        volatile uint8_t MS_ENABLE  : 1;  // M/S Enable (MSEN1/2)
      } CTRL_FIELD_CHAN[2];
    };

    union { // PWM Status (STA)
      volatile uint32_t STATUS; // Full register
      struct BCM_PWM_STATUS_FIELDS {
        const volatile uint32_t ERR_FULL    : 1;  // FIFO Full Flag (FULL1)
        const volatile uint32_t ERR_EMPTY   : 1;  // FIFO Empty Flag (EMPT1)
        volatile uint32_t       ERR_WRITE   : 1;  // FIFO Write Error Flag (WERR1)
        volatile uint32_t       ERR_READ    : 1;  // FIFO Read Error Flag (RERR1)
        volatile uint32_t       CHAN0_GAP   : 1;  // Channel 0 Gap Occurred Flag (GAP1)
        volatile uint32_t       CHAN1_GAP   : 1;  // Channel 1 Gap Occurred Flag (GAP2)
        volatile uint32_t                   : 2;  // Reserved
        volatile uint32_t       ERR_BUS     : 1;  // Bus Error Flag
        const volatile uint32_t CHAN0_STATE : 1;  // Channel 0 State (STA1)
        const volatile uint32_t CHAN1_STATE : 1;  // Channel 1 State (STA2)
      } STATUS_FIELD;
    };

    union { // PWM DMA Configuration (DMAC)
      volatile uint32_t DMA_CONF; // Full Register
      struct BCM_PWM_DMAC_FIELDS {
        volatile uint32_t DREQ    : 8;  // DMA Threshold for DREQ signal
        volatile uint32_t PANIC   : 8;  // DMA Threshold for PANIC signal
        volatile uint32_t         : 15; // Reserved
        volatile uint32_t ENABLE  : 1;  // DMA Enable
      } DMA_CONF_FIELD;
    };

    volatile uint32_t CHAN0_RANGE;  // Channel 0 Range (RNG1)
    volatile uint32_t CHAN0_DATA;   // Channel 0 Data (DAT1)
    volatile uint32_t FIFO_IN;      // Channel FIFO Input (FIF1)
    volatile uint32_t CHAN1_RANGE;  // Channel 1 Range (RNG2)
    volatile uint32_t CHAN1_DATA;   // Channel 1 Data (DAT2)

  };

  struct { // Indexed Channel Registers

    volatile uint32_t : 32; // Padding (Overlaps with CTRL)
    volatile uint32_t : 32; // Padding (Overlaps with STATUS)
    struct CM_PWM_CHAN {
      volatile uint32_t : 32;   // Padding (Overlaps with DMA_CONF / FIFO_IN)
      volatile uint32_t RANGE;  // Channel Range (RNG1/2)
      volatile uint32_t DATA;   // Channel Data (DAT1/2)
    } CHAN[2];

  };

};

struct [[gnu::packed]] RP1_PWM_registers {

  union { // GLOBAL_CTRL
    volatile uint32_t GLOBAL_CTRL;
    struct RP1_PWM_GLOBAL_CTRL_FIELDS {
      volatile uint32_t CHAN0_EN    : 1;  // Channel 0 Enable
      volatile uint32_t CHAN1_EN    : 1;  // Channel 1 Enable
      volatile uint32_t CHAN2_EN    : 1;  // Channel 2 Enable
      volatile uint32_t CHAN3_EN    : 1;  // Channel 3 Enable
      volatile uint32_t             : 27; // Reserved
      volatile uint32_t SET_UPDATE  : 1;  // Settings Update Trigger
    } GLOBAL_CTRL_FIELD;
  };

  union { // FIFO_CTRL
    volatile uint32_t FIFO_CTRL;
    struct RP1_PWM_FIFO_CTRL_FIELDS {
      volatile const uint32_t LEVEL       : 5;
      volatile uint32_t       FLUSH       : 1;
      volatile const uint32_t FLUSH_DONE  : 1;
      volatile uint32_t                   : 4;
      volatile uint32_t       THRESHOLD   : 5;
      volatile uint32_t       DWELL_TIME  : 5;
      volatile uint32_t                   : 10;
      volatile uint32_t       DREQ_EN     : 1;
    } FIFO_CTRL_FIELD;
  };

  volatile uint32_t COMMON_RANGE;
  volatile uint32_t COMMON_DUTY;
  volatile uint32_t DUTY_FIFO;

  struct RP1_PWM_CHAN {
    union {
      volatile uint32_t CTRL;
      struct RP1_PWM_CHAN_CTRL_FIELDS {
        volatile uint32_t MODE            : 3;  // PWM generation mode
        volatile uint32_t INVERT          : 1;  //
        volatile uint32_t BIND            : 1;  // Bind Channel to the common_range and common_duty/duty_fifo registers
        volatile uint32_t USEFIFO         : 1;  //
        volatile uint32_t SDM             : 1;  //
        volatile uint32_t DITHER          : 1;  //
        volatile uint32_t FIFO_POP_MASK   : 1;  //
        volatile uint32_t                 : 3;  //
        volatile uint32_t SDM_BANDWIDTH   : 4;  //
        volatile uint32_t SDM_BIAS        : 16; //
      } CTRL_FIELD;
    };
    volatile uint32_t RANGE;
    volatile uint32_t PHASE;
    volatile uint32_t DUTY;
  } CHAN[4];

  union { // INTR : Raw Interrupts
    volatile uint32_t INTR;
    struct {
      volatile uint32_t FIFO_UNDERFLOW    : 1;
      volatile uint32_t FIFO_OVERFLOW     : 1;
      const volatile uint32_t FIFO_EMPTY  : 1;
      const volatile uint32_t FIFO_FULL   : 1;
      const volatile uint32_t DREQ_ACTIVE : 1;
      volatile uint32_t CHAN0_RELOAD      : 1;
      volatile uint32_t CHAN1_RELOAD      : 1;
      volatile uint32_t CHAN2_RELOAD      : 1;
      volatile uint32_t CHAN3_RELOAD      : 1;
    } INTR_FIELD;
  };

  union { // INTE : Interrupt Enable
    volatile uint32_t INTE;
    struct {
      volatile uint32_t FIFO_UNDERFLOW  : 1;
      volatile uint32_t FIFO_OVERFLOW   : 1;
      volatile uint32_t FIFO_EMPTY      : 1;
      volatile uint32_t FIFO_FULL       : 1;
      volatile uint32_t DREQ_ACTIVE     : 1;
      volatile uint32_t CHAN0_RELOAD    : 1;
      volatile uint32_t CHAN1_RELOAD    : 1;
      volatile uint32_t CHAN2_RELOAD    : 1;
      volatile uint32_t CHAN3_RELOAD    : 1;
    } INTE_FIELD;
  };

  union { // INTF : Interrupt Force
    volatile uint32_t INTF;
    struct {
      volatile uint32_t FIFO_UNDERFLOW  : 1;
      volatile uint32_t FIFO_OVERFLOW   : 1;
      volatile uint32_t FIFO_EMPTY      : 1;
      volatile uint32_t FIFO_FULL       : 1;
      volatile uint32_t DREQ_ACTIVE     : 1;
      volatile uint32_t CHAN0_RELOAD    : 1;
      volatile uint32_t CHAN1_RELOAD    : 1;
      volatile uint32_t CHAN2_RELOAD    : 1;
      volatile uint32_t CHAN3_RELOAD    : 1;
    } INTF_FIELD;
  };

  union { // INTS : Interrupt status after masking & forcing
    const volatile uint32_t INTS;
    struct {
      const volatile uint32_t FIFO_UNDERFLOW  : 1;
      const volatile uint32_t FIFO_OVERFLOW   : 1;
      const volatile uint32_t FIFO_EMPTY      : 1;
      const volatile uint32_t FIFO_FULL       : 1;
      const volatile uint32_t DREQ_ACTIVE     : 1;
      const volatile uint32_t CHAN0_RELOAD    : 1;
      const volatile uint32_t CHAN1_RELOAD    : 1;
      const volatile uint32_t CHAN2_RELOAD    : 1;
      const volatile uint32_t CHAN3_RELOAD    : 1;
    } INTS_FIELD;
  };

};

union PWM_registers {
  union BCM_PWM_registers BCM;  // BCM2835, BCM2836, BCM2837, BCM2711, and RP3A0-based models
  struct RP1_PWM_registers RP1; // RP1-based models
  volatile uint32_t REG[];      // Individual register access by offset
};
