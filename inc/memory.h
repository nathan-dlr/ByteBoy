#ifndef GB_EMU_MEMORY_H
#define GB_EMU_MEMORY_H

enum CARTRIDGES {
    MBC0,
    MBC1
};

typedef struct CARTRIDGE_STRUCT {
    enum CARTRIDGES CART_TYPE;
    uint8_t* ROM;
    uint8_t* RAM;
    uint32_t ROM_SIZE;
    uint32_t RAM_SIZE;
    uint8_t CART_ROM_BANK;
    uint8_t RAM_UPPER_ROM;
    bool BANK_MODE;
    bool RAM_ENABLE;
    uint8_t NUM_ROM_BANKS;
    uint8_t NUM_RAM_BANKS
} CARTRIDGE_STRUCT;

uint8_t* MEMORY;
CARTRIDGE_STRUCT* CARTRIDGE;
void read_memory(uint8_t UNUSED);
void write_memory(uint8_t UNUSED);
#endif //GB_EMU_MEMORY_H
