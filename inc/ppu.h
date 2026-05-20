#ifndef GB_EMU_PPU_H
#define GB_EMU_PPU_H

enum PPU_STATE {
    OAM_SEARCH,
    PIXEL_TRANSFER,
    H_BLANK,
    V_BLANK
};

enum PIXEL_TRANSFER_STATE {
    FETCH_TILE,
    GET_DATA_LOW,
    GET_DATA_HIGH,
    PUSH
};

typedef struct PPU_STRUCT {
    enum PPU_STATE STATE;
    enum PIXEL_TRANSFER_STATE PIXEL_TRANSFER_STATE;
    //RENDER DATA
    uint16_t RENDER_LINE_CYCLE;
    uint8_t RENDER_X;   //incremented per pixel pushed
    bool FIRST_TILE_DONE;
    PIXEL_FIFO* BACKGROUND_FIFO;
    PIXEL_FIFO* SPRITE_FIFO;
    enum FETCH_SOURCE FETCH_TYPE;
    uint8_t NUM_SCROLL_PIXELS;
    uint8_t PENALTY;
    bool POP_ENABLE;
    // WINDOW DATA
    uint8_t WINDOW_LINE_COUNTER;
    bool Y_CONDITION;
    bool INC_WINDOW_COUNTER;
    //FETCHER DATA
    uint8_t FETCHER_X;  //incremented per 8 pixels fetched
    uint8_t TILE_INDEX;
    uint16_t TILE_ADDRESS;
    uint8_t DATA_LOW;
    uint8_t DATA_HIGH;
    PIXEL_DATA* PIXEL_DATA; //8 pixels
    //OBJECT DATA
    struct OAM_STRUCT* CURRENT_OBJ;
    bool VALID_OAM;
} PPU_STRUCT;


PPU_STRUCT* PPU;

void ppu_init();
void ppu_free();
void execute_next_PPU_cycle();

#endif //GB_EMU_PPU_H
