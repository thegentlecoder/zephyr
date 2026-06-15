/*
 * Copyright (c) 2026 Renato Mauro
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_glass_lcd

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

/* This driver is for stm32l476g_discovery REVB or REVC only,
 * thus REVA is not supported.
 */
LOG_MODULE_REGISTER(auxdisplay_stm32_glass_lcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* Supported characters

  Numbers:            0..9
  Uppercase letters:  A..Z  a..c  e..l  o..z
  Lowercase letters:  d  m  n
  Symbols:            Space (0x20)  +  -  *  /  °  % (lower degree sign)
  Others:             µ (0xB5)  'full' (0xFF)

  Points (pos 1..4):  . (dot)  : (double dot)  ; (triple dot)

  Bars (pos 6):         (0x00)..(0x0F)
*/

/* Greek letter mu, i.e. micro, is ASCII 0xB5. */
#define ASCII_CHAR_MU                 0xB5  /* µ */
/* Degree Sign or half percent is ASCII 0xB0. */
#define ASCII_CHAR_DEGREE_SIGN        0xB0  /* ° */
/* All the segments in a position, but the dots. */
#define ASCII_CHAR_FULL               0xFF

#define ASCII_DOT                     0x2E  /* . */
#define ASCII_DOUBLE_DOT              0x3A  /* : */
#define ASCII_TRIPLE_DOT              0x3B  /* ; */

/* LCD bars are handled as 4 bits, from 0 to 15.
 * Values from 0 to 9 are based on the '0' character.
 * Values from 10 to 15 are based on the 'A' character. 
 */
enum lcd_bar_value
{
  LCD_BAR_0 = 0x00,
  LCD_BAR_OFF = LCD_BAR_0,
  LCD_BAR_EMPTY = LCD_BAR_0,
  LCD_BAR_0_PERC = LCD_BAR_0,
  LCD_BAR_1 = 0x01,
  LCD_BAR_1_QUARTER = LCD_BAR_1,
  LCD_BAR_25_PERC = LCD_BAR_1,
  LCD_BAR_2 = 0x02,
  LCD_BAR_3 = 0x03,
  LCD_BAR_HALF = LCD_BAR_3,
  LCD_BAR_50_PERC = LCD_BAR_3,
  LCD_BAR_4 = 0x04,
  LCD_BAR_5 = 0x05,
  LCD_BAR_6 = 0x06,
  LCD_BAR_7 = 0x07,
  LCD_BAR_3_QUARTERS = LCD_BAR_7,
  LCD_BAR_75_PERC = LCD_BAR_7,
  LCD_BAR_8 = 0x08,
  LCD_BAR_9 = 0x09,
  LCD_BAR_10 = 0x0A,
  LCD_BAR_11 = 0x0B,
  LCD_BAR_12 = 0x0C,
  LCD_BAR_13 = 0x0D,
  LCD_BAR_14 = 0x0E,
  LCD_BAR_15 = 0x0F,
  LCD_BAR_FULL = LCD_BAR_15,
  LCD_BAR_100_PERC = LCD_BAR_15,
  LCD_BAR_MAX = 0x10
};

/**
  * @brief LCD Glass digit position
  */
enum lcd_digit_position
{
  LCD_DIGIT_POSITION_1 = 0,
  LCD_DIGIT_POSITION_2 = 1,
  LCD_DIGIT_POSITION_3 = 2,
  LCD_DIGIT_POSITION_4 = 3,
  LCD_DIGIT_POSITION_5 = 4,
  LCD_DIGIT_POSITION_6 = 5,
  LCD_DIGIT_MAX_NUMBER = 6,
};

/**
  @verbatim
================================================================================
                              GLASS LCD MAPPING
================================================================================
LCD allows to display informations on six 14-segment digits and 4 bars:

  1       2       3       4       5       6
-----   -----   -----   -----   -----   -----   
|\|/| o |\|/| o |\|/| o |\|/| o |\|/|   |\|/|   BAR3
-- --   -- --   -- --   -- --   -- --   -- --   BAR2
|/|\| o |/|\| o |/|\| o |/|\| o |/|\|   |/|\|   BAR1
----- * ----- * ----- * ----- * -----   -----   BAR0

LCD segment mapping:

  Pos 1..6      Pos 1..4                 Pos 6    

  -----A-----                                     
  |\   |   /|        _                  _______   
  F H  J  K B   COL |_|           BAR3 |_______|  
  |  \ | /  |                           _______   
  --G-- --M--        _            BAR2 |_______|  
  |  / | \  |   COL |_|                 _______   
  E Q  P  N C                     COL  |_______|  
  |/   |   \|        _                  _______   
  -----D-----   DP  |_|           DP   |_______|  

 An LCD character coding is based on the following matrix:
COM           0   1   2     3
SEG(n)      { E , D , P ,   N  }
SEG(n+1)    { M , C , COL , DP }
SEG(23-n-1) { B , A , K ,   J  }
SEG(23-n)   { G , F , Q ,   H  }
with n positive odd number.

 The character 'A' for example is:
  -------------------------------
LSB   { 1 , 0 , 0 , 0 }
      { 1 , 1 , 0 , 0 }
      { 1 , 1 , 0 , 0 }
MSB   { 1 , 1 , 0 , 0 }
      -------------------
 'A' =  F   E   0   0  hexadecimal

BARS

|      Graphic Bar    |   Segment   | Common |   RAM    |
|      (position)     |  (MCU pin)  |   pin  | Register |
|-------------------------------------------------------|
| BAR3 (top)          | SEG9  (PC7) |  COM2  |    4     |
| BAR2 (middle-upper) | SEG9  (PC7) |  COM3  |    6     |
| BAR1 (middle-lower) | SEG11 (PB4) |  COM2  |    4     |
| BAR0 (bottom)       | SEG11 (PB4) |  COM3  |    6     |

  @endverbatim
*/

/* Constant table for cap characters 'A' --> 'Z' */
const uint16_t CapLetterMap[26]=
    {
        /* A       B       C       D       E       F       G       H       I  */
        0xFE00, 0x6714, 0x1D00, 0x4714, 0x9D00, 0x9C00, 0x3F00, 0xFA00, 0x0014,
        /* J       K       L       M       N       O       P       Q       R  */
        0x5300, 0x9841, 0x1900, 0x5A48, 0x5A09, 0x5F00, 0xFC00, 0x5F01, 0xFC01,
        /* S       T       U       V       W       X       Y       Z  */
        0xAF00, 0x0414, 0x5b00, 0x18C0, 0x5A81, 0x00C9, 0x0058, 0x05C0
    };

/* Constant table for number '0' --> '9' */
const uint16_t NumberMap[10]=
    {
        /* 0      1      2      3      4      5      6      7      8      9  */
        0x5FC0,0x4200,0xF500,0x6700,0xEa00,0xAF00,0xBF00,0x04600,0xFF00,0xEF00
    };

/* code for ' ' character */
#define C_SPACE               ((uint16_t) 0x0000)

/* code for '(' character */
#define C_OPENPARMAP          ((uint16_t) 0x0028)

/* code for ')' character */
#define C_CLOSEPARMAP         ((uint16_t) 0x0011)

/* code for 'd' character */
#define C_DMAP                ((uint16_t) 0xf300)

/* code for 'm' character */
#define C_MMAP                ((uint16_t) 0xb210)

/* code for 'n' character */
#define C_NMAP                ((uint16_t) 0x2210)

/* code for 'µ' character */
#define C_UMAP                ((uint16_t) 0x6084)

/* constant code for '*' character */
#define C_STAR                ((uint16_t) 0xA0DD)

/* constant code for '-' character */
#define C_MINUS               ((uint16_t) 0xA000)

/* constant code for '+' character */
#define C_PLUS                ((uint16_t) 0xA014)

/* constant code for '/' */
#define C_SLASH               ((uint16_t) 0x00c0)

/* constant code for ° */
#define C_PERCENT_1           ((uint16_t) 0xec00)

/* constant code for small o */
#define C_PERCENT_2           ((uint16_t) 0xb300)

#define C_FULL                ((uint16_t) 0xffdd)

/**
  * @brief LCD Digit COM & SEG definitions
  */
#define LCD_DIGIT1_COM0               LCD_COM0
#define LCD_DIGIT1_COM0_SEG_MASK      ~(LCD_SEG0 | LCD_SEG1 | LCD_SEG22 | LCD_SEG23)
#define LCD_DIGIT1_COM1               LCD_COM1
#define LCD_DIGIT1_COM1_SEG_MASK      ~(LCD_SEG0 | LCD_SEG1 | LCD_SEG22 | LCD_SEG23)
#define LCD_DIGIT1_COM2               LCD_COM2
#define LCD_DIGIT1_COM2_SEG_MASK      ~(LCD_SEG0 | LCD_SEG1 | LCD_SEG22 | LCD_SEG23)
#define LCD_DIGIT1_COM3               LCD_COM3
#define LCD_DIGIT1_COM3_SEG_MASK      ~(LCD_SEG0 | LCD_SEG1 | LCD_SEG22 | LCD_SEG23)

#define LCD_DIGIT2_COM0               LCD_COM0
#define LCD_DIGIT2_COM0_SEG_MASK      ~(LCD_SEG2 | LCD_SEG3 | LCD_SEG20 | LCD_SEG21)
#define LCD_DIGIT2_COM1               LCD_COM1
#define LCD_DIGIT2_COM1_SEG_MASK      ~(LCD_SEG2 | LCD_SEG3 | LCD_SEG20 | LCD_SEG21)
#define LCD_DIGIT2_COM2               LCD_COM2
#define LCD_DIGIT2_COM2_SEG_MASK      ~(LCD_SEG2 | LCD_SEG3 | LCD_SEG20 | LCD_SEG21)
#define LCD_DIGIT2_COM3               LCD_COM3
#define LCD_DIGIT2_COM3_SEG_MASK      ~(LCD_SEG2 | LCD_SEG3 | LCD_SEG20 | LCD_SEG21)

#define LCD_DIGIT3_COM0               LCD_COM0
#define LCD_DIGIT3_COM0_SEG_MASK      ~(LCD_SEG4 | LCD_SEG5 | LCD_SEG18 | LCD_SEG19)
#define LCD_DIGIT3_COM1               LCD_COM1
#define LCD_DIGIT3_COM1_SEG_MASK      ~(LCD_SEG4 | LCD_SEG5 | LCD_SEG18 | LCD_SEG19)
#define LCD_DIGIT3_COM2               LCD_COM2
#define LCD_DIGIT3_COM2_SEG_MASK      ~(LCD_SEG4 | LCD_SEG5 | LCD_SEG18 | LCD_SEG19)
#define LCD_DIGIT3_COM3               LCD_COM3
#define LCD_DIGIT3_COM3_SEG_MASK      ~(LCD_SEG4 | LCD_SEG5 | LCD_SEG18 | LCD_SEG19)

#define LCD_DIGIT4_COM0               LCD_COM0
#define LCD_DIGIT4_COM0_SEG_MASK      ~(LCD_SEG6 | LCD_SEG17)
#define LCD_DIGIT4_COM0_1             LCD_COM0_1
#define LCD_DIGIT4_COM0_1_SEG_MASK    ~(LCD_SEG7 | LCD_SEG16)
#define LCD_DIGIT4_COM1               LCD_COM1
#define LCD_DIGIT4_COM1_SEG_MASK      ~(LCD_SEG6 |  LCD_SEG17)
#define LCD_DIGIT4_COM1_1             LCD_COM1_1
#define LCD_DIGIT4_COM1_1_SEG_MASK    ~(LCD_SEG7 | LCD_SEG16)
#define LCD_DIGIT4_COM2               LCD_COM2
#define LCD_DIGIT4_COM2_SEG_MASK      ~(LCD_SEG6 | LCD_SEG17)
#define LCD_DIGIT4_COM2_1             LCD_COM2_1
#define LCD_DIGIT4_COM2_1_SEG_MASK    ~(LCD_SEG7 | LCD_SEG16)
#define LCD_DIGIT4_COM3               LCD_COM3
#define LCD_DIGIT4_COM3_SEG_MASK      ~(LCD_SEG6 | LCD_SEG17)
#define LCD_DIGIT4_COM3_1             LCD_COM3_1
#define LCD_DIGIT4_COM3_1_SEG_MASK    ~(LCD_SEG7 | LCD_SEG16)

#define LCD_DIGIT5_COM0               LCD_COM0
#define LCD_DIGIT5_COM0_SEG_MASK      ~(LCD_SEG9 | LCD_SEG14)
#define LCD_DIGIT5_COM0_1             LCD_COM0_1
#define LCD_DIGIT5_COM0_1_SEG_MASK    ~(LCD_SEG8 | LCD_SEG15)
#define LCD_DIGIT5_COM1               LCD_COM1
#define LCD_DIGIT5_COM1_SEG_MASK      ~(LCD_SEG9 | LCD_SEG14)
#define LCD_DIGIT5_COM1_1             LCD_COM1_1
#define LCD_DIGIT5_COM1_1_SEG_MASK    ~(LCD_SEG8 | LCD_SEG15)
#define LCD_DIGIT5_COM2               LCD_COM2
#define LCD_DIGIT5_COM2_SEG_MASK      ~(LCD_SEG9 | LCD_SEG14)
#define LCD_DIGIT5_COM2_1             LCD_COM2_1
#define LCD_DIGIT5_COM2_1_SEG_MASK    ~(LCD_SEG8 | LCD_SEG15)
#define LCD_DIGIT5_COM3               LCD_COM3
#define LCD_DIGIT5_COM3_SEG_MASK      ~(LCD_SEG9 | LCD_SEG14)
#define LCD_DIGIT5_COM3_1             LCD_COM3_1
#define LCD_DIGIT5_COM3_1_SEG_MASK    ~(LCD_SEG8 | LCD_SEG15)

#define LCD_DIGIT6_COM0               LCD_COM0
#define LCD_DIGIT6_COM0_SEG_MASK      ~(LCD_SEG10 | LCD_SEG11 | LCD_SEG12 | LCD_SEG13)
#define LCD_DIGIT6_COM1               LCD_COM1
#define LCD_DIGIT6_COM1_SEG_MASK      ~(LCD_SEG10 | LCD_SEG11 | LCD_SEG12 | LCD_SEG13)
#define LCD_DIGIT6_COM2               LCD_COM2
#define LCD_DIGIT6_COM2_SEG_MASK      ~(LCD_SEG10 | LCD_SEG11 | LCD_SEG12 | LCD_SEG13)
#define LCD_DIGIT6_COM3               LCD_COM3
#define LCD_DIGIT6_COM3_SEG_MASK      ~(LCD_SEG10 | LCD_SEG11 | LCD_SEG12 | LCD_SEG13)

/**
  * @brief LCD Bar segments & coms definitions.
  */
#define LCD_BAR0_2_COM            LCD_COM3
#define LCD_BAR1_3_COM            LCD_COM2
#define LCD_BAR0_SEG              LCD_SEG11
#define LCD_BAR1_SEG              LCD_SEG11
#define LCD_BAR2_SEG              LCD_SEG9
#define LCD_BAR3_SEG              LCD_SEG9
#define LCD_BAR2_SEG_MASK         ~(LCD_BAR2_SEG)
#define LCD_BAR3_SEG_MASK         ~(LCD_BAR3_SEG)

/**
  * @brief LCD segments & coms redefinition.
  * LCD component segments & coms are not necessarily linked to MCU segmnents & coms output.
  */
#define LCD_COM0          MCU_LCD_COM0
#define LCD_COM0_1        MCU_LCD_COM0_1
#define LCD_COM1          MCU_LCD_COM1
#define LCD_COM1_1        MCU_LCD_COM1_1
#define LCD_COM2          MCU_LCD_COM2
#define LCD_COM2_1        MCU_LCD_COM2_1
#define LCD_COM3          MCU_LCD_COM3
#define LCD_COM3_1        MCU_LCD_COM3_1
#define LCD_SEG0          MCU_LCD_SEG4
#define LCD_SEG1          MCU_LCD_SEG23
#define LCD_SEG2          MCU_LCD_SEG6
#define LCD_SEG3          MCU_LCD_SEG13
#define LCD_SEG4          MCU_LCD_SEG15
#define LCD_SEG5          MCU_LCD_SEG29
#define LCD_SEG6          MCU_LCD_SEG31
#define LCD_SEG7          MCU_LCD_SEG33
#define LCD_SEG8          MCU_LCD_SEG35
#define LCD_SEG9          MCU_LCD_SEG25
#define LCD_SEG10         MCU_LCD_SEG17
#define LCD_SEG11         MCU_LCD_SEG8
#define LCD_SEG12         MCU_LCD_SEG9
#define LCD_SEG13         MCU_LCD_SEG26
#define LCD_SEG14         MCU_LCD_SEG24
#define LCD_SEG15         MCU_LCD_SEG34
#define LCD_SEG16         MCU_LCD_SEG32
#define LCD_SEG17         MCU_LCD_SEG30
#define LCD_SEG18         MCU_LCD_SEG28
#define LCD_SEG19         MCU_LCD_SEG14
#define LCD_SEG20         MCU_LCD_SEG12
#define LCD_SEG21         MCU_LCD_SEG5
#define LCD_SEG22         MCU_LCD_SEG22
#define LCD_SEG23         MCU_LCD_SEG3
#define LCD_SEG0_SHIFT          MCU_LCD_SEG4_SHIFT
#define LCD_SEG1_SHIFT          MCU_LCD_SEG23_SHIFT
#define LCD_SEG2_SHIFT          MCU_LCD_SEG6_SHIFT
#define LCD_SEG3_SHIFT          MCU_LCD_SEG13_SHIFT
#define LCD_SEG4_SHIFT          MCU_LCD_SEG15_SHIFT
#define LCD_SEG5_SHIFT          MCU_LCD_SEG29_SHIFT
#define LCD_SEG6_SHIFT          MCU_LCD_SEG31_SHIFT
#define LCD_SEG7_SHIFT          MCU_LCD_SEG33_SHIFT
#define LCD_SEG8_SHIFT          MCU_LCD_SEG35_SHIFT
#define LCD_SEG9_SHIFT          MCU_LCD_SEG25_SHIFT
#define LCD_SEG10_SHIFT         MCU_LCD_SEG17_SHIFT
#define LCD_SEG11_SHIFT         MCU_LCD_SEG8_SHIFT
#define LCD_SEG12_SHIFT         MCU_LCD_SEG9_SHIFT
#define LCD_SEG13_SHIFT         MCU_LCD_SEG26_SHIFT
#define LCD_SEG14_SHIFT         MCU_LCD_SEG24_SHIFT
#define LCD_SEG15_SHIFT         MCU_LCD_SEG34_SHIFT
#define LCD_SEG16_SHIFT         MCU_LCD_SEG32_SHIFT
#define LCD_SEG17_SHIFT         MCU_LCD_SEG30_SHIFT
#define LCD_SEG18_SHIFT         MCU_LCD_SEG28_SHIFT
#define LCD_SEG19_SHIFT         MCU_LCD_SEG14_SHIFT
#define LCD_SEG20_SHIFT         MCU_LCD_SEG12_SHIFT
#define LCD_SEG21_SHIFT         MCU_LCD_SEG5_SHIFT
#define LCD_SEG22_SHIFT         MCU_LCD_SEG22_SHIFT
#define LCD_SEG23_SHIFT         MCU_LCD_SEG3_SHIFT

/**
  * @brief STM32 LCD segments & coms definitions.
  */
#define MCU_LCD_COM0          LCD_RAM_REGISTER0
#define MCU_LCD_COM0_1        LCD_RAM_REGISTER1
#define MCU_LCD_COM1          LCD_RAM_REGISTER2
#define MCU_LCD_COM1_1        LCD_RAM_REGISTER3
#define MCU_LCD_COM2          LCD_RAM_REGISTER4
#define MCU_LCD_COM2_1        LCD_RAM_REGISTER5
#define MCU_LCD_COM3          LCD_RAM_REGISTER6
#define MCU_LCD_COM3_1        LCD_RAM_REGISTER7
#define MCU_LCD_COM4          LCD_RAM_REGISTER8
#define MCU_LCD_COM4_1        LCD_RAM_REGISTER9
#define MCU_LCD_COM5          LCD_RAM_REGISTER10
#define MCU_LCD_COM5_1        LCD_RAM_REGISTER11
#define MCU_LCD_COM6          LCD_RAM_REGISTER12
#define MCU_LCD_COM6_1        LCD_RAM_REGISTER13
#define MCU_LCD_COM7          LCD_RAM_REGISTER14
#define MCU_LCD_COM7_1        LCD_RAM_REGISTER15
#define MCU_LCD_SEG0          (1U << MCU_LCD_SEG0_SHIFT)
#define MCU_LCD_SEG1          (1U << MCU_LCD_SEG1_SHIFT)
#define MCU_LCD_SEG2          (1U << MCU_LCD_SEG2_SHIFT)
#define MCU_LCD_SEG3          (1U << MCU_LCD_SEG3_SHIFT)
#define MCU_LCD_SEG4          (1U << MCU_LCD_SEG4_SHIFT)
#define MCU_LCD_SEG5          (1U << MCU_LCD_SEG5_SHIFT)
#define MCU_LCD_SEG6          (1U << MCU_LCD_SEG6_SHIFT)
#define MCU_LCD_SEG7          (1U << MCU_LCD_SEG7_SHIFT)
#define MCU_LCD_SEG8          (1U << MCU_LCD_SEG8_SHIFT)
#define MCU_LCD_SEG9          (1U << MCU_LCD_SEG9_SHIFT)
#define MCU_LCD_SEG10         (1U << MCU_LCD_SEG10_SHIFT)
#define MCU_LCD_SEG11         (1U << MCU_LCD_SEG11_SHIFT)
#define MCU_LCD_SEG12         (1U << MCU_LCD_SEG12_SHIFT)
#define MCU_LCD_SEG13         (1U << MCU_LCD_SEG13_SHIFT)
#define MCU_LCD_SEG14         (1U << MCU_LCD_SEG14_SHIFT)
#define MCU_LCD_SEG15         (1U << MCU_LCD_SEG15_SHIFT)
#define MCU_LCD_SEG16         (1U << MCU_LCD_SEG16_SHIFT)
#define MCU_LCD_SEG17         (1U << MCU_LCD_SEG17_SHIFT)
#define MCU_LCD_SEG18         (1U << MCU_LCD_SEG18_SHIFT)
#define MCU_LCD_SEG19         (1U << MCU_LCD_SEG19_SHIFT)
#define MCU_LCD_SEG20         (1U << MCU_LCD_SEG20_SHIFT)
#define MCU_LCD_SEG21         (1U << MCU_LCD_SEG21_SHIFT)
#define MCU_LCD_SEG22         (1U << MCU_LCD_SEG22_SHIFT)
#define MCU_LCD_SEG23         (1U << MCU_LCD_SEG23_SHIFT)
#define MCU_LCD_SEG24         (1U << MCU_LCD_SEG24_SHIFT)
#define MCU_LCD_SEG25         (1U << MCU_LCD_SEG25_SHIFT)
#define MCU_LCD_SEG26         (1U << MCU_LCD_SEG26_SHIFT)
#define MCU_LCD_SEG27         (1U << MCU_LCD_SEG27_SHIFT)
#define MCU_LCD_SEG28         (1U << MCU_LCD_SEG28_SHIFT)
#define MCU_LCD_SEG29         (1U << MCU_LCD_SEG29_SHIFT)
#define MCU_LCD_SEG30         (1U << MCU_LCD_SEG30_SHIFT)
#define MCU_LCD_SEG31         (1U << MCU_LCD_SEG31_SHIFT)
#define MCU_LCD_SEG32         (1U << MCU_LCD_SEG32_SHIFT)
#define MCU_LCD_SEG33         (1U << MCU_LCD_SEG33_SHIFT)
#define MCU_LCD_SEG34         (1U << MCU_LCD_SEG34_SHIFT)
#define MCU_LCD_SEG35         (1U << MCU_LCD_SEG35_SHIFT)
#define MCU_LCD_SEG36         (1U << MCU_LCD_SEG36_SHIFT)
#define MCU_LCD_SEG37         (1U << MCU_LCD_SEG37_SHIFT)
#define MCU_LCD_SEG38         (1U << MCU_LCD_SEG38_SHIFT)
#define MCU_LCD_SEG0_SHIFT    0
#define MCU_LCD_SEG1_SHIFT    1
#define MCU_LCD_SEG2_SHIFT    2
#define MCU_LCD_SEG3_SHIFT    3
#define MCU_LCD_SEG4_SHIFT    4
#define MCU_LCD_SEG5_SHIFT    5
#define MCU_LCD_SEG6_SHIFT    6
#define MCU_LCD_SEG7_SHIFT    7
#define MCU_LCD_SEG8_SHIFT    8
#define MCU_LCD_SEG9_SHIFT    9
#define MCU_LCD_SEG10_SHIFT   10
#define MCU_LCD_SEG11_SHIFT   11
#define MCU_LCD_SEG12_SHIFT   12
#define MCU_LCD_SEG13_SHIFT   13
#define MCU_LCD_SEG14_SHIFT   14
#define MCU_LCD_SEG15_SHIFT   15
#define MCU_LCD_SEG16_SHIFT   16
#define MCU_LCD_SEG17_SHIFT   17
#define MCU_LCD_SEG18_SHIFT   18
#define MCU_LCD_SEG19_SHIFT   19
#define MCU_LCD_SEG20_SHIFT   20
#define MCU_LCD_SEG21_SHIFT   21
#define MCU_LCD_SEG22_SHIFT   22
#define MCU_LCD_SEG23_SHIFT   23
#define MCU_LCD_SEG24_SHIFT   24
#define MCU_LCD_SEG25_SHIFT   25
#define MCU_LCD_SEG26_SHIFT   26
#define MCU_LCD_SEG27_SHIFT   27
#define MCU_LCD_SEG28_SHIFT   28
#define MCU_LCD_SEG29_SHIFT   29
#define MCU_LCD_SEG30_SHIFT   30
#define MCU_LCD_SEG31_SHIFT   31
#define MCU_LCD_SEG32_SHIFT   0
#define MCU_LCD_SEG33_SHIFT   1
#define MCU_LCD_SEG34_SHIFT   2
#define MCU_LCD_SEG35_SHIFT   3
#define MCU_LCD_SEG36_SHIFT   4
#define MCU_LCD_SEG37_SHIFT   5
#define MCU_LCD_SEG38_SHIFT   6
#define MCU_LCD_SEG39_SHIFT   7
#define MCU_LCD_SEG40_SHIFT   8
#define MCU_LCD_SEG41_SHIFT   9
#define MCU_LCD_SEG42_SHIFT   10
#define MCU_LCD_SEG43_SHIFT   11

#define LCD_RAM_REGISTER0               (0x00000000U) /*!< LCD RAM Register 0  */
#define LCD_RAM_REGISTER1               (0x00000001U) /*!< LCD RAM Register 1  */
#define LCD_RAM_REGISTER2               (0x00000002U) /*!< LCD RAM Register 2  */
#define LCD_RAM_REGISTER3               (0x00000003U) /*!< LCD RAM Register 3  */
#define LCD_RAM_REGISTER4               (0x00000004U) /*!< LCD RAM Register 4  */
#define LCD_RAM_REGISTER5               (0x00000005U) /*!< LCD RAM Register 5  */
#define LCD_RAM_REGISTER6               (0x00000006U) /*!< LCD RAM Register 6  */
#define LCD_RAM_REGISTER7               (0x00000007U) /*!< LCD RAM Register 7  */
#define LCD_RAM_REGISTER8               (0x00000008U) /*!< LCD RAM Register 8  */
#define LCD_RAM_REGISTER9               (0x00000009U) /*!< LCD RAM Register 9  */
#define LCD_RAM_REGISTER10              (0x0000000AU) /*!< LCD RAM Register 10 */
#define LCD_RAM_REGISTER11              (0x0000000BU) /*!< LCD RAM Register 11 */
#define LCD_RAM_REGISTER12              (0x0000000CU) /*!< LCD RAM Register 12 */
#define LCD_RAM_REGISTER13              (0x0000000DU) /*!< LCD RAM Register 13 */
#define LCD_RAM_REGISTER14              (0x0000000EU) /*!< LCD RAM Register 14 */
#define LCD_RAM_REGISTER15              (0x0000000FU) /*!< LCD RAM Register 15 */

/* Define for scrolling sentences*/
#define SCROLL_SPEED_HIGH     150
#define SCROLL_SPEED_MEDIUM   300
#define SCROLL_SPEED_LOW      450

#define NIBBLE_BUFFER_LEN 5
#define NIBBLE_BUFFER_BAR2_3_INDEX    NIBBLE_BUFFER_LEN-1

#define ASCII_CHAR_0                  0x30  /* 0 */
#define ASCII_CHAR_AT_SYMBOL          0x40  /* @ */
#define ASCII_CHAR_LEFT_OPEN_BRACKET  0x5B  /* [ */
#define ASCII_CHAR_APOSTROPHE         0x60  /* ` */
#define ASCII_CHAR_LEFT_OPEN_BRACE    0x7B  /* ( */

/**
  * @brief LCD Glass point
  * Element values correspond to LCD Glass point, for positions 1 to 4.
  */

enum lcd_point
{
	POINT_OFF = 0,
  POINT_SINGLE,
	POINT_DOUBLE,
	POINT_TRIPLE
};

static void Convert(uint8_t Char, uint8_t position, uint8_t point, uint8_t bar_value, uint32_t* digit);
static void WriteChar(LCD_HandleTypeDef *hlcd, uint8_t ch, uint8_t position, uint8_t point, uint8_t bar_value);

/**
  * @brief  Convert an ascii char to the an LCD digit, handling points and bars too.
  * @param  Char: a char to display.
  * @param  position: the character LCD destination [1:6].
  * @param  point: a point or colon to display right after the character.
  *         Allowed in positions 1 to 4, ignored otherwise.
  *         Valid values: 0x0, ASCII_DOT, ASCII_DOUBLE_DOT, ASCII_TRIPLE_DOT
  * @param  bar_value: the value (enum lcd_bar_value) to display with bars, in position 6 only.
  * @param  digit : output, digit frame buffer (length is NIBBLE_BUFFER_LEN).
  * @retval None
  */
static void Convert(uint8_t Char, uint8_t position, uint8_t point, uint8_t bar_value, uint32_t* digit)
{
  uint16_t ch = 0 ;
  uint8_t loop = 0, index = 0;
  
  switch (Char)
    {
    case ' ' :
      ch = C_SPACE;
      break;

    case '*':
      ch = C_STAR;
      break;

    case '(' :
      ch = C_OPENPARMAP;
      break;

    case ')' :
      ch = C_CLOSEPARMAP;
      break;
      
    case 'd' :
      ch = C_DMAP;
      break;
    
    case 'm' :
      ch = C_MMAP;
      break;
    
    case 'n' :
      ch = C_NMAP;
      break;

    case ASCII_CHAR_MU :
      ch = C_UMAP;
      break;

    case '-' :
      ch = C_MINUS;
      break;

    case '+' :
      ch = C_PLUS;
      break;

    case '/' :
      ch = C_SLASH;
      break;  
      
    case ASCII_CHAR_DEGREE_SIGN :
      ch = C_PERCENT_1;
      break;  
    case '%' : /* truly lower degree sign */
      ch = C_PERCENT_2; 
      break;
    case ASCII_CHAR_FULL :
      ch = C_FULL;
      break ;
    
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':      
      ch = NumberMap[Char - ASCII_CHAR_0];    
      break;
          
    default:
      /* The character Char is one letter in upper case*/
      if ( (Char < ASCII_CHAR_LEFT_OPEN_BRACKET) && (Char > ASCII_CHAR_AT_SYMBOL) )
      {
        ch = CapLetterMap[Char - 'A'];
      }
      /* The character Char is one letter in lower case*/
      if ( (Char < ASCII_CHAR_LEFT_OPEN_BRACE) && ( Char > ASCII_CHAR_APOSTROPHE) )
      {
        ch = CapLetterMap[Char - 'a'];
      }
      break;
  }

  /* add bits for points */
  if (position < LCD_DIGIT_POSITION_5) {
    if (point == POINT_SINGLE || point == POINT_TRIPLE)
    {
      ch |= 0x0002;
    }
    if (point == POINT_DOUBLE || point == POINT_TRIPLE)
    {
      ch |= 0x0020;
    }
  }

  /* add bits for bars */
  digit[NIBBLE_BUFFER_BAR2_3_INDEX] = 0;
  if (position == LCD_DIGIT_POSITION_6 && bar_value) {
      /* bar 0 and 1 are mapped on the same segments as points */
      if (bar_value & 0x01) ch |= 0x0002;
      if (bar_value & 0x02) ch |= 0x0020;
      /* bar 2 and 3 are mapped on segments never used in positions other than 6 */
      if (bar_value & 0x04) digit[NIBBLE_BUFFER_BAR2_3_INDEX] |= 0x0001;
      if (bar_value & 0x08) digit[NIBBLE_BUFFER_BAR2_3_INDEX] |= 0x0002;
  }

  for (loop = 12, index=0 ; index < 4 ; loop -= 4, index++)
  {
    digit[index] = (ch >> loop) & 0x0f; /*To isolate the less significant 4 bits */
  }
}

/**
  * @brief  Write a character in the LCD display, including points and bars.
  * @param  hlcd: the LCD display instance.
  * @param  ch: the character to display.
  * @param  position: the character LCD destination [1:6].
  * @param  point: a point or colon to display right after the character.
  *         Allowed in positions 1 to 4, ignored otherwise.
  *         Valid values: 0x0, ASCII_DOT, ASCII_DOUBLE_DOT, ASCII_TRIPLE_DOT
  * @param  bar_value: the value (enum lcd_bar_value) to display with bars, in position 6 only.
  * @retval None
  */
static void WriteChar(LCD_HandleTypeDef *hlcd, uint8_t ch, uint8_t position, uint8_t point, uint8_t bar_value)
{
  /* To convert displayed character in segment in array digit */
  uint32_t digit[NIBBLE_BUFFER_LEN];
  Convert(ch, position, point, bar_value, digit);

  uint32_t data = 0x00;
  switch (position)
  {
    /* Position 1 on LCD (Digit1)*/
    case LCD_DIGIT_POSITION_1:
      data = ((digit[0] & 0x1) << LCD_SEG0_SHIFT) | (((digit[0] & 0x2) >> 1) << LCD_SEG1_SHIFT)
          | (((digit[0] & 0x4) >> 2) << LCD_SEG22_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG23_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT1_COM0, LCD_DIGIT1_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[1] & 0x1) << LCD_SEG0_SHIFT) | (((digit[1] & 0x2) >> 1) << LCD_SEG1_SHIFT)
          | (((digit[1] & 0x4) >> 2) << LCD_SEG22_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG23_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT1_COM1, LCD_DIGIT1_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = ((digit[2] & 0x1) << LCD_SEG0_SHIFT) | (((digit[2] & 0x2) >> 1) << LCD_SEG1_SHIFT)
          | (((digit[2] & 0x4) >> 2) << LCD_SEG22_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG23_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT1_COM2, LCD_DIGIT1_COM2_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = ((digit[3] & 0x1) << LCD_SEG0_SHIFT) | (((digit[3] & 0x2) >> 1) << LCD_SEG1_SHIFT)
          | (((digit[3] & 0x4) >> 2) << LCD_SEG22_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG23_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT1_COM3, LCD_DIGIT1_COM3_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      break;

    /* Position 2 on LCD (Digit2)*/
    case LCD_DIGIT_POSITION_2:
      data = ((digit[0] & 0x1) << LCD_SEG2_SHIFT) | (((digit[0] & 0x2) >> 1) << LCD_SEG3_SHIFT)
          | (((digit[0] & 0x4) >> 2) << LCD_SEG20_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG21_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT2_COM0, LCD_DIGIT2_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[1] & 0x1) << LCD_SEG2_SHIFT) | (((digit[1] & 0x2) >> 1) << LCD_SEG3_SHIFT)
          | (((digit[1] & 0x4) >> 2) << LCD_SEG20_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG21_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT2_COM1, LCD_DIGIT2_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = ((digit[2] & 0x1) << LCD_SEG2_SHIFT) | (((digit[2] & 0x2) >> 1) << LCD_SEG3_SHIFT)
          | (((digit[2] & 0x4) >> 2) << LCD_SEG20_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG21_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT2_COM2, LCD_DIGIT2_COM2_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = ((digit[3] & 0x1) << LCD_SEG2_SHIFT) | (((digit[3] & 0x2) >> 1) << LCD_SEG3_SHIFT)
          | (((digit[3] & 0x4) >> 2) << LCD_SEG20_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG21_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT2_COM3, LCD_DIGIT2_COM3_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      break;
    
    /* Position 3 on LCD (Digit3)*/
    case LCD_DIGIT_POSITION_3:
      data = ((digit[0] & 0x1) << LCD_SEG4_SHIFT) | (((digit[0] & 0x2) >> 1) << LCD_SEG5_SHIFT)
          | (((digit[0] & 0x4) >> 2) << LCD_SEG18_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG19_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT3_COM0, LCD_DIGIT3_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[1] & 0x1) << LCD_SEG4_SHIFT) | (((digit[1] & 0x2) >> 1) << LCD_SEG5_SHIFT)
          | (((digit[1] & 0x4) >> 2) << LCD_SEG18_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG19_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT3_COM1, LCD_DIGIT3_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = ((digit[2] & 0x1) << LCD_SEG4_SHIFT) | (((digit[2] & 0x2) >> 1) << LCD_SEG5_SHIFT)
          | (((digit[2] & 0x4) >> 2) << LCD_SEG18_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG19_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT3_COM2, LCD_DIGIT3_COM2_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = ((digit[3] & 0x1) << LCD_SEG4_SHIFT) | (((digit[3] & 0x2) >> 1) << LCD_SEG5_SHIFT)
          | (((digit[3] & 0x4) >> 2) << LCD_SEG18_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG19_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT3_COM3, LCD_DIGIT3_COM3_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      break;
    
    /* Position 4 on LCD (Digit4)*/
    case LCD_DIGIT_POSITION_4:
      data = ((digit[0] & 0x1) << LCD_SEG6_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG17_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM0, LCD_DIGIT4_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = (((digit[0] & 0x2) >> 1) << LCD_SEG7_SHIFT) | (((digit[0] & 0x4) >> 2) << LCD_SEG16_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM0_1, LCD_DIGIT4_COM0_1_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[1] & 0x1) << LCD_SEG6_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG17_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM1, LCD_DIGIT4_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = (((digit[1] & 0x2) >> 1) << LCD_SEG7_SHIFT) | (((digit[1] & 0x4) >> 2) << LCD_SEG16_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM1_1, LCD_DIGIT4_COM1_1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = ((digit[2] & 0x1) << LCD_SEG6_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG17_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM2, LCD_DIGIT4_COM2_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = (((digit[2] & 0x2) >> 1) << LCD_SEG7_SHIFT) | (((digit[2] & 0x4) >> 2) << LCD_SEG16_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM2_1, LCD_DIGIT4_COM2_1_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = ((digit[3] & 0x1) << LCD_SEG6_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG17_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM3, LCD_DIGIT4_COM3_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      
      data = (((digit[3] & 0x2) >> 1) << LCD_SEG7_SHIFT) | (((digit[3] & 0x4) >> 2) << LCD_SEG16_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT4_COM3_1, LCD_DIGIT4_COM3_1_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      break;
    
    /* Position 5 on LCD (Digit5)*/
    case LCD_DIGIT_POSITION_5:
       data = (((digit[0] & 0x2) >> 1) << LCD_SEG9_SHIFT) | (((digit[0] & 0x4) >> 2) << LCD_SEG14_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM0, LCD_DIGIT5_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[0] & 0x1) << LCD_SEG8_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG15_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM0_1, LCD_DIGIT5_COM0_1_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = (((digit[1] & 0x2) >> 1) << LCD_SEG9_SHIFT) | (((digit[1] & 0x4) >> 2) << LCD_SEG14_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM1, LCD_DIGIT5_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
       data = ((digit[1] & 0x1) << LCD_SEG8_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG15_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM1_1, LCD_DIGIT5_COM1_1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = (((digit[2] & 0x2) >> 1) << LCD_SEG9_SHIFT) | (((digit[2] & 0x4) >> 2) << LCD_SEG14_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM2, LCD_DIGIT5_COM2_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = ((digit[2] & 0x1) << LCD_SEG8_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG15_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM2_1, LCD_DIGIT5_COM2_1_SEG_MASK, data) ; /* 1Q 1K 1Col 1P  */
      
      data = (((digit[3] & 0x2) >> 1) << LCD_SEG9_SHIFT) | (((digit[3] & 0x4) >> 2) << LCD_SEG14_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM3, LCD_DIGIT5_COM3_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      
      data = ((digit[3] & 0x1) << LCD_SEG8_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG15_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT5_COM3_1, LCD_DIGIT5_COM3_1_SEG_MASK, data) ; /* 1H 1J 1DP 1N  */
      break;
    
    /* Position 6 on LCD (Digit6)*/
    case LCD_DIGIT_POSITION_6:
      data = ((digit[0] & 0x1) << LCD_SEG10_SHIFT) | (((digit[0] & 0x2) >> 1) << LCD_SEG11_SHIFT)
          | (((digit[0] & 0x4) >> 2) << LCD_SEG12_SHIFT) | (((digit[0] & 0x8) >> 3) << LCD_SEG13_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT6_COM0, LCD_DIGIT6_COM0_SEG_MASK, data); /* 1G 1B 1M 1E */
      
      data = ((digit[1] & 0x1) << LCD_SEG10_SHIFT) | (((digit[1] & 0x2) >> 1) << LCD_SEG11_SHIFT)
          | (((digit[1] & 0x4) >> 2) << LCD_SEG12_SHIFT) | (((digit[1] & 0x8) >> 3) << LCD_SEG13_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT6_COM1, LCD_DIGIT6_COM1_SEG_MASK, data) ; /* 1F 1A 1C 1D  */
      
      data = ((digit[2] & 0x1) << LCD_SEG10_SHIFT) | (((digit[2] & 0x2) >> 1) << LCD_SEG11_SHIFT)
          | (((digit[2] & 0x4) >> 2) << LCD_SEG12_SHIFT) | (((digit[2] & 0x8) >> 3) << LCD_SEG13_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT6_COM2, LCD_DIGIT6_COM2_SEG_MASK, data) ; /* 1Q 1K BAR0 1P  */
      
      data = ((digit[3] & 0x1) << LCD_SEG10_SHIFT) | (((digit[3] & 0x2) >> 1) << LCD_SEG11_SHIFT)
          | (((digit[3] & 0x4) >> 2) << LCD_SEG12_SHIFT) | (((digit[3] & 0x8) >> 3) << LCD_SEG13_SHIFT);
      HAL_LCD_Write(hlcd, LCD_DIGIT6_COM3, LCD_DIGIT6_COM3_SEG_MASK, data) ; /* 1H 1J BAR1 1N  */

      /* handle bar 2 and 3 */
      data = ((digit[NIBBLE_BUFFER_BAR2_3_INDEX] & 0x1) << LCD_SEG9_SHIFT);
      HAL_LCD_Write(hlcd, LCD_BAR0_2_COM, LCD_BAR2_SEG_MASK, data) ; /* BAR2 */
      data = (((digit[NIBBLE_BUFFER_BAR2_3_INDEX] & 0x2) >> 1) << LCD_SEG9_SHIFT);
      HAL_LCD_Write(hlcd, LCD_BAR1_3_COM, LCD_BAR3_SEG_MASK, data) ; /* BAR3 */
      break;
    
     default:
      break;
  }
}

/* Immutabile driver configuration structure stored in Flash */
struct stm32_lcd_config {
	LCD_TypeDef *regs;
	const struct stm32_pclken *pclken;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
	uint8_t columns;
	uint8_t rows;
};

/* Mutable driver runtime instance data structure stored in RAM */
struct stm32_lcd_data {
	LCD_HandleTypeDef hlcd;
};

static int stm32_lcd_capabilities_get(const struct device *dev, struct auxdisplay_capabilities *capabilities)
{
    const struct stm32_lcd_config *config = dev->config;
    if (!capabilities) return -EINVAL;

	  capabilities->columns = config->columns;
    capabilities->rows = config->rows;

	  return 0;
}

static int stm32_lcd_clear(const struct device *dev)
{
    struct stm32_lcd_data *data = dev->data;
    HAL_LCD_Clear(&data->hlcd);
    return 0;
}

/* Zephyr Auxdisplay API interface callback: write operation */
static int stm32_lcd_aux_write(const struct device *dev, const uint8_t *data, uint16_t len)
{
	const struct stm32_lcd_config *config = dev->config;
	struct stm32_lcd_data *driver_data = dev->data;

	/* Clear the entire LCD memory structure before rendering the updated text payload */
	//HAL_LCD_Clear(&driver_data->hlcd);

	/* Loop to update segments sequentially up to the physical maximum string length restriction */
	for (int i = 0, position = 0; i < len && position < config->columns; i++, position++) {
		const uint8_t Char = data[i];
		uint8_t point = POINT_OFF;
		uint8_t bar_value = 0;
		if (i+1 < len) {
			const uint8_t symbol_char =  data[i+1];
      /* points in position 1..4, 5 is ignored in Convert() */
			if (position < LCD_DIGIT_POSITION_6) { 
				switch (symbol_char) {
					case ASCII_DOT:
						point = POINT_SINGLE;
						i++;
						break;
					case ASCII_DOUBLE_DOT:
						point = POINT_DOUBLE;
						i++;
						break;
					case ASCII_TRIPLE_DOT:
						point = POINT_TRIPLE;
						i++;
						break;
					default:
						break;
				}
			}
			else if (symbol_char < LCD_BAR_MAX) { /* lcd bars in position 6 */
				bar_value = symbol_char;
				i++;
			}
		}
		WriteChar(&driver_data->hlcd, Char, position, point, bar_value);
	}

	/* Fire a hardware request telling the peripheral controller to push internal RAM data to the glass */
	HAL_LCD_UpdateDisplayRequest(&driver_data->hlcd);

	return 0;
}

/* Map implementation functions to the standard public Zephyr auxdisplay API architecture interface */
static const struct auxdisplay_driver_api stm32_lcd_aux_api = {
	.capabilities_get = stm32_lcd_capabilities_get,
	.clear = stm32_lcd_clear,
	.write = stm32_lcd_aux_write,
};

/* Core device initialization logic executed automatically by the Zephyr kernel boot sequencer */
static int stm32_lcd_aux_init(const struct device *dev)
{
	const struct stm32_lcd_config *config = dev->config;
	struct stm32_lcd_data *driver_data = dev->data;
	int ret;

	/* 1. Request clock gating initialization from the generic Zephyr device sub-system tree */
	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("STM32 Clock Control driver device is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)config->pclken);
	if (ret < 0) {
		LOG_ERR("Failed to enable STM32 LCD peripheral clock gate (err: %d)", ret);
		return ret;
	}

	//k_busy_wait(10);

	/* 2. Enforce the required pin alternate configurations natively via the Pinctrl manager framework */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl default operational states (err: %d)", ret);
		return ret;
	}

	/* 3. Extract the register boundaries configured within Devicetree and pipe them into the ST HAL struct */
	driver_data->hlcd.Instance = config->regs;

	/* Hardware parameters tuning extracted directly from STM32Cube peripheral specifications */
	driver_data->hlcd.Init.Prescaler = LCD_PRESCALER_1;
	driver_data->hlcd.Init.Divider = LCD_DIVIDER_31;
	driver_data->hlcd.Init.Duty = LCD_DUTY_1_4;
	driver_data->hlcd.Init.Bias = LCD_BIAS_1_3;
	driver_data->hlcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;
	driver_data->hlcd.Init.Contrast = LCD_CONTRASTLEVEL_5;
	driver_data->hlcd.Init.DeadTime = LCD_DEADTIME_0;
	driver_data->hlcd.Init.PulseOnDuration = LCD_PULSEONDURATION_4;
	driver_data->hlcd.Init.HighDrive = LCD_HIGHDRIVE_DISABLE;
	driver_data->hlcd.Init.BlinkMode = LCD_BLINKMODE_OFF;
	driver_data->hlcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV32;
	driver_data->hlcd.Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;

	// __HAL_RCC_PWR_CLK_ENABLE();
	// HAL_PWR_EnableBkUpAccess();
	//__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE); 
	// HAL_Delay(2);
	__HAL_RCC_LCD_CLK_ENABLE();

	HAL_StatusTypeDef init_res = HAL_LCD_Init(&driver_data->hlcd);
	if ( init_res != HAL_OK) {
		LOG_ERR("STM32 HAL_LCD_Init execution failure encountered");
		LOG_DBG("DEBUG LCD: RCC_BDCR=0x%08X, LCD_CR=0x%08X, LCD_SR=0x%08X, init_res=%d\n", RCC->BDCR, LCD->CR, LCD->SR, init_res);
		return -EIO;
	}

	/* Trigger hardware core initialization macros to start internal voltage pump generators */
	__HAL_LCD_ENABLE(&driver_data->hlcd);

	LOG_INF("STM32 Segment LCD driver initialized successfully with %d digits", config->columns);
	return 0;
}

/* Advanced Devicetree generation macro mapping compile-time definitions directly to C parameters */
#define STM32_LCD_DEVICE_INIT(inst)                                             \
	PINCTRL_DT_INST_DEFINE(inst);              									\
																				\
	static const struct stm32_pclken stm32_lcd_pclken_##inst[] = {              \
		STM32_DT_INST_CLOCK_INFO_BY_IDX(inst, 0)                                \
	};																			\
																				\
	static const struct stm32_lcd_config stm32_lcd_config_##inst = {            \
		.regs = (LCD_TypeDef *)DT_INST_REG_ADDR(inst),                          \
		.pclken = stm32_lcd_pclken_##inst,                                      \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                           \
		.columns = DT_INST_PROP(inst, columns),                                 \
		.rows = DT_INST_PROP(inst, rows),                                       \
	};                                                                          \
																				\
	static struct stm32_lcd_data stm32_lcd_data_##inst;                         \
																				\
	DEVICE_DT_INST_DEFINE(inst,                                                 \
			      stm32_lcd_aux_init,                                           \
			      NULL,                                                         \
			      &stm32_lcd_data_##inst,                                       \
			      &stm32_lcd_config_##inst,                                     \
			      POST_KERNEL,                                                  \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY,                              \
			      &stm32_lcd_aux_api);

/* Parse the active devicetree layout and execute the instantiation macro for matching okay targets */
DT_INST_FOREACH_STATUS_OKAY(STM32_LCD_DEVICE_INIT)
