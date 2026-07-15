.. Copyright 2026 Renato Mauro
..
.. SPDX-License-Identifier: Apache-2.0

.. _st_gh08172t_shield:

ST GH08172T SLCD Panel Shield
==========================

Overview
--------

The GH08172T is a 6-digit 14-segment LCD panel with 9 indicators.
Hardware Configuration:
- 6 Display Positions (digits and letters 1-6, left to right in datasheet)
- 14 Segments per Position (A, B, C, D, E, F, G, H, J, K, M, N, P, Q)
- 4 physical COM lines (COM0-COM3), 8 logical COM lines (COM0-COM3, COM0_1-COM3_1)
  handled via 8 RAM registers
- 24 SEG lines (SEG0-SEG23)
- 28 physical pins (Pin 1-28), no VDD and GND, the LCD can be mounted upside-down,
  in that case the mapping MCU-PIN to LCD-PIN changes, but the panel logic is the same
- 8 dot indicators (COL1-COL4, DP11-DP4): dot and double dot indicators between digit
  positions 1-5, not available between positions 5 and 6
- 1 four-bars indicator at the right side of position 6


Supported characters

Numbers:            0..9
Uppercase letters:  A..Z
Unit prefixes:      d  c  m  u  n
Operators:          +  -  *  /
Symbols:            Space (' ': 0x20)  %  _  (  )  Degree ('^': 0x5E)  Full ('#': 0x23)

Points (pos 1..4):  . (dot)  : (double dot)  ; (triple dot)

Bars (pos 6):         (0x00)..(0x0F)

The percentage symbol is translated into three display positions
It is made by a degree sign, a slash and a low ring

The degree sign is passed as the ^ ASCII character

All the segments in a position, but the dots are passed as the # ASCII character

Dots can be single '.', double ':' and triple ';'

LCD bars are handled as 4 bits, from ASCII 0 to 15


 GLASS LCD MAPPING
 The LCD has six 14-segment digits with point/colon and 4 bars:

   1       2       3       4       5       6
 -----   -----   -----   -----   -----   -----
 |\|/| o |\|/| o |\|/| o |\|/| o |\|/|   |\|/|   BAR3
 -- --   -- --   -- --   -- --   -- --   -- --   BAR2
 |/|\| o |/|\| o |/|\| o |/|\| o |/|\|   |/|\|   BAR1
 ----- * ----- * ----- * ----- * -----   -----   BAR0

 LCD segment mapping:

  Pos 1..6     Pos 1..4                 Pos 6

 -----A-----
 |\   |   /|        _                  _______
 F H  J  K B   COL |_|           BAR3 |_______|  (same as pos 5 COL)
 |  \ | /  |                           _______
 --G-- --M--        _            BAR2 |_______|  (some as pos 5 DP)
 |  / | \  |   COL |_|                 _______
 E Q  P  N C                     BAR1 |_______|  (same as pos 6 COL)
 |/   |   \|        _                  _______
 -----D-----   DP  |_|           BAR0 |_______|  (some as pos 6 DP)

 An LCD character coding is based on the following matrix:

 COM         |   0     1     2     3   |
 ---------------------------------------
 SEG(n)      {   E ,   D ,   P ,   N   }
 SEG(n+1)    {   M ,   C ,   COL , DP  }
 SEG(23-n-1) {   B ,   A ,   K ,   J   }
 SEG(23-n)   {   G ,   F ,   Q ,   H   }

 The character 'A' for example is:
 -----------------------
 LSB   { 1 , 0 , 0 , 0 }
       { 1 , 1 , 0 , 0 }
       { 1 , 1 , 0 , 0 }
 MSB   { 1 , 1 , 0 , 0 }
 -----------------------
  'A' =  F   E   0   0  hexadecimal

 n is a positive even number, whose value is n = 2 * (Pos - 1):

 POS          |  1 |  2 |  3 |  4 |  5 |  6 |
 --------------------------------------------
 n  2*(Pos-1) |  0 |  2 |  4 |  6 |  8 | 10 |
 --------------------------------------------
 SEG (n)      |  0 |  2 |  4 |  6 |  8 | 10 |
 SEG (n+1)    |  1 |  3 |  5 |  7 |  9 | 11 |
 SEG (23-n-1) | 22 | 20 | 18 | 16 | 14 | 12 |
 SEG (23-n)   | 23 | 21 | 19 | 17 | 15 | 13 |


 BARS

 |      Graphic Bar    |   Segment   | Common |
 |      (position)     |  (MCU pin)  |  pin   |
 ----------------------------------------------
 | BAR3 (top)          | SEG9  (PC7) |  COM2  |
 | BAR2 (middle-upper) | SEG9  (PC7) |  COM3  |
 | BAR1 (middle-lower) | SEG11 (PB4) |  COM2  |
 | BAR0 (bottom)       | SEG11 (PB4) |  COM3  |

 Position 0

 COM              |   0     1     2     3   |
 --------------------------------------------
 SEG (n)        0 {   E ,   D ,   P ,   N   }
 SEG (n+1)      1 {   M ,   C ,   COL , DP  }
 SEG (23-n-1)  22 {   B ,   A ,   K ,   J   }
 SEG (23-n)    23 {   G ,   F ,   Q ,   H   }

      ---------- A S22_C1 ---------
     |\             |             /|
     |  \           |           /  |
     |    \      J S22_C3     /    |
 F S23_C1   \       |       /   B S23_C0
     |    H S23_C3  |  K S23_C2    |               _
     |          \   |   /          |    COL S1_C2 |_|
     |            \ | /            |
      -- G S23_C0 -- -- M S1_C0  --
     |            / | \            |
     |          /   |   \          |               _
     |    Q S23_C2  | N S0_C3      |    COL S1_C2 |_|
 E S0_C0    /       |       \   C S1_C1
     |    /      P S0_C2      \    |
     |  /           |           \  |
     |/             |             \|               _
      ---------- D S0_C1 ----------     DP  S1_C3 |_|


 Summary for all positions:
 LCD pin to LCD crystal, sorted by LCD pin

 | LCD   | LCD | COM3 | COM2 | COM1 | COM0 |
 | Pin   | Pin |      |      |      |      |
 | name  |     |      |      |      |      |
 |       |     |      |      |      |      |
 -------------------------------------------
 | SEG0  |   1 |  1N  |  1P  |  1D  |  1E  |
 | SEG1  |   2 | 1DP  | 1COL |  1C  |  1M  |
 | SEG2  |   3 |  2N  |  2P  |  2D  |  2E  |
 | SEG3  |   4 | 2DP  | 2COL |  2C  |  2M  |
 | SEG4  |   5 |  3N  |  3P  |  3D  |  3E  |
 | SEG5  |   6 | 3DP  | 3COL |  3C  |  3M  |
 | SEG6  |   7 |  4N  |  4P  |  4D  |  4E  |
 | SEG7  |   8 | 4DP  | 4COL |  4C  |  4M  |
 | SEG8  |   9 |  5N  |  5P  |  5D  |  5E  |
 | SEG9  |  10 | BAR2 | BAR3 |  5C  |  5M  |
 | SEG10 |  11 |  6N  |  6P  |  6D  |  6E  |
 | SEG11 |  12 | BAR0 | BAR1 |  6C  |  6M  |
 | COM3  |  13 | COM3 |      |      |      |
 | COM2  |  14 |      | COM2 |      |      |
 | COM1  |  15 |      |      | COM1 |      |
 | COM0  |  16 |      |      |      | COM0 |
 | SEG12 |  17 |  6J  |  6K  |  6A  |  6B  |
 | SEG13 |  18 |  6H  |  6Q  |  6F  |  6G  |
 | SEG14 |  19 |  5J  |  5K  |  5A  |  5B  |
 | SEG15 |  20 |  5H  |  5Q  |  5F  |  5G  |
 | SEG16 |  21 |  4J  |  4K  |  4A  |  4B  |
 | SEG17 |  22 |  4H  |  4Q  |  4F  |  4G  |
 | SEG18 |  23 |  3J  |  3K  |  3A  |  3B  |
 | SEG19 |  24 |  3H  |  3Q  |  3F  |  3G  |
 | SEG20 |  25 |  2J  |  2K  |  2A  |  2B  |
 | SEG21 |  26 |  2H  |  2Q  |  2F  |  2G  |
 | SEG22 |  27 |  1J  |  1K  |  1A  |  1B  |
 | SEG23 |  28 |  1H  |  1Q  |  1F  |  1G  |

Truly it's more complicated, because each COM is able to drive
more than 32 bits; so, if bits 0-31 are driven by COM0, bits
32-63 (actually, in ST code, 38 for segments and 43 for shifts)
are driven by a second register, named COM0_1. This means that
a logical 63 bit set is handled via two 32 bit sets driven by
two registers. This happens for positions 4 and 5 only.
