/*
 * notes.h
 * Notas A0..C8 como número MIDI (A4 = 69 = 440 Hz). CS4 = C#4, DB4 = CS4.
 */
#ifndef INC_NOTES_H_
#define INC_NOTES_H_

#define REST 0

#define A0    21   /*    27.500 Hz */
#define AS0    22   /*    29.135 Hz */
#define BB0    22
#define B0    23   /*    30.868 Hz */
#define C1    24   /*    32.703 Hz */
#define CS1    25   /*    34.648 Hz */
#define DB1    25
#define D1    26   /*    36.708 Hz */
#define DS1    27   /*    38.891 Hz */
#define EB1    27
#define E1    28   /*    41.203 Hz */
#define F1    29   /*    43.654 Hz */
#define FS1    30   /*    46.249 Hz */
#define GB1    30
#define G1    31   /*    48.999 Hz */
#define GS1    32   /*    51.913 Hz */
#define AB1    32
#define A1    33   /*    55.000 Hz */
#define AS1    34   /*    58.270 Hz */
#define BB1    34
#define B1    35   /*    61.735 Hz */
#define C2    36   /*    65.406 Hz */
#define CS2    37   /*    69.296 Hz */
#define DB2    37
#define D2    38   /*    73.416 Hz */
#define DS2    39   /*    77.782 Hz */
#define EB2    39
#define E2    40   /*    82.407 Hz */
#define F2    41   /*    87.307 Hz */
#define FS2    42   /*    92.499 Hz */
#define GB2    42
#define G2    43   /*    97.999 Hz */
#define GS2    44   /*   103.826 Hz */
#define AB2    44
#define A2    45   /*   110.000 Hz */
#define AS2    46   /*   116.541 Hz */
#define BB2    46
#define B2    47   /*   123.471 Hz */
#define C3    48   /*   130.813 Hz */
#define CS3    49   /*   138.591 Hz */
#define DB3    49
#define D3    50   /*   146.832 Hz */
#define DS3    51   /*   155.563 Hz */
#define EB3    51
#define E3    52   /*   164.814 Hz */
#define F3    53   /*   174.614 Hz */
#define FS3    54   /*   184.997 Hz */
#define GB3    54
#define G3    55   /*   195.998 Hz */
#define GS3    56   /*   207.652 Hz */
#define AB3    56
#define A3    57   /*   220.000 Hz */
#define AS3    58   /*   233.082 Hz */
#define BB3    58
#define B3    59   /*   246.942 Hz */
#define C4    60   /*   261.626 Hz */
#define CS4    61   /*   277.183 Hz */
#define DB4    61
#define D4    62   /*   293.665 Hz */
#define DS4    63   /*   311.127 Hz */
#define EB4    63
#define E4    64   /*   329.628 Hz */
#define F4    65   /*   349.228 Hz */
#define FS4    66   /*   369.994 Hz */
#define GB4    66
#define G4    67   /*   391.995 Hz */
#define GS4    68   /*   415.305 Hz */
#define AB4    68
#define A4    69   /*   440.000 Hz */
#define AS4    70   /*   466.164 Hz */
#define BB4    70
#define B4    71   /*   493.883 Hz */
#define C5    72   /*   523.251 Hz */
#define CS5    73   /*   554.365 Hz */
#define DB5    73
#define D5    74   /*   587.330 Hz */
#define DS5    75   /*   622.254 Hz */
#define EB5    75
#define E5    76   /*   659.255 Hz */
#define F5    77   /*   698.456 Hz */
#define FS5    78   /*   739.989 Hz */
#define GB5    78
#define G5    79   /*   783.991 Hz */
#define GS5    80   /*   830.609 Hz */
#define AB5    80
#define A5    81   /*   880.000 Hz */
#define AS5    82   /*   932.328 Hz */
#define BB5    82
#define B5    83   /*   987.767 Hz */
#define C6    84   /*  1046.502 Hz */
#define CS6    85   /*  1108.731 Hz */
#define DB6    85
#define D6    86   /*  1174.659 Hz */
#define DS6    87   /*  1244.508 Hz */
#define EB6    87
#define E6    88   /*  1318.510 Hz */
#define F6    89   /*  1396.913 Hz */
#define FS6    90   /*  1479.978 Hz */
#define GB6    90
#define G6    91   /*  1567.982 Hz */
#define GS6    92   /*  1661.219 Hz */
#define AB6    92
#define A6    93   /*  1760.000 Hz */
#define AS6    94   /*  1864.655 Hz */
#define BB6    94
#define B6    95   /*  1975.533 Hz */
#define C7    96   /*  2093.005 Hz */
#define CS7    97   /*  2217.461 Hz */
#define DB7    97
#define D7    98   /*  2349.318 Hz */
#define DS7    99   /*  2489.016 Hz */
#define EB7    99
#define E7   100   /*  2637.020 Hz */
#define F7   101   /*  2793.826 Hz */
#define FS7   102   /*  2959.955 Hz */
#define GB7   102
#define G7   103   /*  3135.963 Hz */
#define GS7   104   /*  3322.438 Hz */
#define AB7   104
#define A7   105   /*  3520.000 Hz */
#define AS7   106   /*  3729.310 Hz */
#define BB7   106
#define B7   107   /*  3951.066 Hz */
#define C8   108   /*  4186.009 Hz */

#define NOTE_MIN A0
#define NOTE_MAX C8

#endif /* INC_NOTES_H_ */
