#ifndef GB_EMU_COMMON_H
#define GB_EMU_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_WIDTH 160
#define WINDOW_HEIGHT 144

#define P1 0xFF00
#define SB 0xFF01
#define SC 0xFF02
#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07
#define IF 0xFF0F
#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23
#define NR50 0xFF24
#define NR51 0xFF25
#define NR52 0xFF26
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY 0xFF42
#define SCX 0xFF43
#define LY 0xFF44
#define LYC 0xFF45
#define DMA 0xFF46
#define BGP 0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define WY 0xFF4A
#define WX 0xFF4B
#define KEY1 0xFF4D
#define VBK 44
#define HDMA1 0xFF51
#define HDMA2 0xFF52
#define HDMA3 0xFF53
#define HDMA4 0xFF54
#define HDMA5 0xFF55
#define RP 0xFF56
#define BCPs 0cFF68
#define BCPD 0xFF69
#define OCPS 0xFF6A
#define OCPD 0xFF6B
#define SVBK 0xFF70

#define IE 0xFFFF
#define UNUSED_VAL 0x00

#define SET_BIT(bit, value) (bit | value)
#define CLEAR_BIT(bit, value) (~bit & value)

enum FETCH_SOURCE {
    BACKGROUND,
    WINDOW,
    OBJECT
};


enum PALETTE {
    OBJ_P0,
    OBJ_P1,
    NOT_OBJ
};


typedef struct PIXEL_DATA {
    uint8_t binary_data;
    enum FETCH_SOURCE source;
    bool palette;
    bool priority;
    uint16_t address;
    bool x_flip;
    uint8_t x_pos;
} PIXEL_DATA;

typedef struct PIXEL_FIFO {
    int8_t front;
    int8_t back;
    uint8_t size;
    PIXEL_DATA** pixel_data;
} PIXEL_FIFO;

typedef struct OAM_STRUCT {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_index;
    uint16_t address;
    bool priority;
    bool y_flip;
    bool x_flip;
    enum PALETTE palette;
} OAM_STRUCT;


#endif //GB_EMU_COMMON_H
