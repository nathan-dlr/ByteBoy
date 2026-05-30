#include <common.h>
#include <gb.h>
#include <min_heap.h>
#include <lcd.h>
#include <queue.h>
#include <memory.h>
#include <ppu.h>

#define BITS_PER_TILE 16
#define CYCLES_PER_LINE 456
#define PIXELS_PER_TILE 8
#define OAM_BASE_ADDRESS 0xFE00


static void pop_pixel();

void ppu_init() {
    PPU = (PPU_STRUCT*)malloc(sizeof(PPU_STRUCT));
    PPU->CURRENT_OBJ = (OAM_STRUCT*)malloc(sizeof(OAM_STRUCT));
    PPU->PIXEL_DATA = (PIXEL_DATA*)calloc(PIXELS_PER_TILE, sizeof(PIXEL_DATA));
    PPU->STATE = V_BLANK;
    PPU->RENDER_LINE_CYCLE = 1;
    PPU->FETCH_TYPE = BACKGROUND;
    PPU->FETCHER_X = 0;
    PPU->RENDER_X = 0;
    PPU->PENALTY = 0;
    PPU->POP_ENABLE = true;
    PPU->FIRST_TILE_DONE = false;
    PPU->WINDOW_LINE_COUNTER = 0;
    PPU->Y_CONDITION = false;
    PPU->INC_WINDOW_COUNTER = false;
}

void ppu_free() {
    free(PPU->PIXEL_DATA);
    free(PPU->CURRENT_OBJ);
    free(PPU);
}

static void oam_search_validate() {
    uint16_t oam_address = OAM_BASE_ADDRESS + (PPU->RENDER_LINE_CYCLE - 1) * 2;
    uint8_t obj_y_pos = MEMORY[oam_address];
    uint8_t obj_x_pos = MEMORY[oam_address + 1];
    uint8_t lcd_y_position = MEMORY[LY];
    uint8_t obj_height = MEMORY[LCDC] & 0x04 ? 16 : 8;

    // An object's y-position is equal to their vertical position on screen + 16
    if ((lcd_y_position >= obj_y_pos - 16) && (lcd_y_position < obj_y_pos - 16 + obj_height )) {
        PPU->VALID_OAM = true;
        PPU->CURRENT_OBJ->y_pos = obj_y_pos;
        PPU->CURRENT_OBJ->x_pos = obj_x_pos;
        PPU->CURRENT_OBJ->address = oam_address;
    }
    else {
        PPU->VALID_OAM = false;
    }
}

static void oam_search_store() {
    if (PPU->VALID_OAM) {
        uint16_t oam_address = PPU->CURRENT_OBJ->address;
        uint8_t oam_attributes = MEMORY[oam_address + 3];
        PPU->CURRENT_OBJ->tile_index = MEMORY[oam_address + 2];
        PPU->CURRENT_OBJ->priority = oam_attributes & 0x80;
        PPU->CURRENT_OBJ->y_flip = oam_attributes & 0x40;
        PPU->CURRENT_OBJ->x_flip = oam_attributes & 0x20;
        PPU->CURRENT_OBJ->palette = oam_attributes & 0x10;
        heap_insert(PPU->CURRENT_OBJ);
    }
    if (PPU->RENDER_LINE_CYCLE == 80) {
        PPU->STATE = PIXEL_TRANSFER;
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
        PPU->FETCH_TYPE = BACKGROUND;
        MEMORY[STAT] = (MEMORY[STAT] & 0xFC) | 0x03;
        PPU->NUM_SCROLL_PIXELS = MEMORY[SCX] % 8;
    }
}

static void construct_pixel_data() {
    uint8_t data_low = PPU->DATA_LOW;
    uint8_t data_high = PPU->DATA_HIGH;
    uint8_t bit_0;
    uint8_t bit_1;
    uint8_t pixel;
    enum FETCH_SOURCE source = PPU->FETCH_TYPE;
    bool is_obj = source == OBJECT;
    const OAM_STRUCT* obj = heap_peek();
    for (int8_t i = 7, j = 0; i >= 0; i--, j++) {
        bit_0 = (data_low >> i) & 0x01;
        bit_1 = (data_high >> i) & 0x01;
        pixel = (bit_1 << 1) | bit_0;
        if (is_obj) {
            uint8_t index = obj->x_flip ? i : j;
            PPU->PIXEL_DATA[index].binary_data = pixel;
            PPU->PIXEL_DATA[index].source = source;
            PPU->PIXEL_DATA[index].address = obj->address;
            PPU->PIXEL_DATA[index].palette = obj->palette;

            PPU->PIXEL_DATA[index].priority = obj->priority;
            PPU->PIXEL_DATA[index].x_pos = obj->x_pos + index;
        }
        else {
            PPU->PIXEL_DATA[j].binary_data = pixel;
            PPU->PIXEL_DATA[j].source = source;
        }
    }
}


static void fetch_tile() {
    uint8_t tile_x;
    uint8_t tile_y;
    uint16_t base_address;
    uint16_t tile_map_address;
    if (PPU->FETCH_TYPE == WINDOW) {
        base_address = MEMORY[LCDC] & 0x40 ? 0x9C00 : 0x9800;
        tile_x = PPU->FETCHER_X;
        tile_y = PPU->WINDOW_LINE_COUNTER;
        tile_map_address = base_address + tile_x + ((tile_y / 8) * 32);
        PPU->TILE_INDEX = MEMORY[tile_map_address];
    }
    else if (PPU->FETCH_TYPE == BACKGROUND) {
        base_address = MEMORY[LCDC] & 0x08 ? 0x9C00 : 0x9800;
        tile_x = ((MEMORY[SCX] / 8) + PPU->FETCHER_X) & 0x1F;
        tile_y = (MEMORY[LY] + MEMORY[SCY]) & 0xFF;
        tile_map_address = (base_address + tile_x + ((tile_y / 8) * 32));
        //TODO IF VRAM IS BLOCKED THAN TILE NUMBER WILL BE READ AS 0xFF
        PPU->TILE_INDEX = MEMORY[tile_map_address];
    }
    else {
        PPU->TILE_INDEX = heap_peek()->tile_index;
        if (MEMORY[LCDC] & 0x04) {
            PPU->TILE_INDEX &= 0xFE;
        }
    }
    PPU->PIXEL_TRANSFER_STATE = GET_DATA_LOW;
}

static void get_tile_data_low() {
    //get base address
    if ((MEMORY[LCDC] & 0x10) || (PPU->FETCH_TYPE == OBJECT)) {
        PPU->TILE_ADDRESS = 0x8000 + (PPU->TILE_INDEX * BITS_PER_TILE);
    }
    else {
        PPU->TILE_ADDRESS = 0x9000 + ((int8_t)PPU->TILE_INDEX * BITS_PER_TILE);
    }
    //add offset from pixel's y_position in the tile
    if (PPU->FETCH_TYPE == BACKGROUND) {
        PPU->TILE_ADDRESS += 2 * ((MEMORY[LY] + MEMORY[SCY]) % 8);
    }
    else if (PPU->FETCH_TYPE == WINDOW) {
        PPU->TILE_ADDRESS += 2 * ((PPU->WINDOW_LINE_COUNTER) % 8);
    }
    else {
        uint8_t y_offset = MEMORY[LY] - (heap_peek()->y_pos - 16);
        if (heap_peek()->y_flip) {
            uint8_t sprite_height = (MEMORY[LCDC] & 0x04) ? 16 : 8;
            y_offset = sprite_height - 1 - y_offset;
        }
        PPU->TILE_ADDRESS += 2 * y_offset;
    }
    PPU->DATA_LOW = MEMORY[PPU->TILE_ADDRESS];
    PPU->PIXEL_TRANSFER_STATE = GET_DATA_HIGH;
}

static void get_tile_data_high() {
    PPU->DATA_HIGH = MEMORY[PPU->TILE_ADDRESS+1];
    if (PPU->FETCH_TYPE == BACKGROUND && PPU->RENDER_X == 0 && !PPU->FIRST_TILE_DONE) {
        PPU->FIRST_TILE_DONE = true;
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
    }
    else {
        PPU->PIXEL_TRANSFER_STATE = PUSH;
        construct_pixel_data();
    }
}

static void pixel_push() {
    if ((PPU->FETCH_TYPE != OBJECT) && (pixel_fifo_is_empty(PPU->BACKGROUND_FIFO))) {
        background_fifo_push(PPU->PIXEL_DATA);
        PPU->FETCHER_X++;
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
    }
    else if (PPU->FETCH_TYPE == OBJECT) {
        sprite_fifo_push(PPU->PIXEL_DATA);
        heap_delete_min();
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
        if ((PPU->RENDER_X >= MEMORY[WX] - 7) && PPU->Y_CONDITION && (MEMORY[LCDC] & 0x20)) {
            PPU->FETCH_TYPE = WINDOW;
        }
        else {
            PPU->FETCH_TYPE = BACKGROUND;
        }
        PPU->POP_ENABLE = true;
    }
}

static void pop_pixel() {
    PIXEL_DATA pixel_data;

    // Throw away scroll pixels
    if (PPU->NUM_SCROLL_PIXELS) {
        pixel_fifo_pop(PPU->BACKGROUND_FIFO, &pixel_data);
        PPU->NUM_SCROLL_PIXELS--;
        return;
    }
    // Merge pixel from both fifos
    else if (!pixel_fifo_is_empty(PPU->SPRITE_FIFO)) {
        PIXEL_DATA bg_pixel_data;
        PIXEL_DATA obj_pixel_data;
        pixel_fifo_pop(PPU->BACKGROUND_FIFO, &bg_pixel_data);
        pixel_fifo_pop(PPU->SPRITE_FIFO, &obj_pixel_data);

        if (MEMORY[LCDC] & 0x01) {
            bool transparent = obj_pixel_data.binary_data == 0x00;
            bool priority = obj_pixel_data.priority && (bg_pixel_data.binary_data != 0x00);
            pixel_data = transparent || priority ? bg_pixel_data : obj_pixel_data;
        }
        if (!(MEMORY[LCDC] & 0x02)) {
            pixel_data = bg_pixel_data;
        }
    }
    // Pop from background fifo
    else {
        pixel_fifo_pop(PPU->BACKGROUND_FIFO, &pixel_data);
    }
    lcd_update_pixel(&pixel_data);
    PPU->RENDER_X++;
    if (PPU->RENDER_X == 160) {
        PPU->FETCHER_X = 0;
        PPU->RENDER_X = 0;
        pixel_fifo_clear(PPU->BACKGROUND_FIFO);
        pixel_fifo_clear(PPU->SPRITE_FIFO);
        PPU->STATE = H_BLANK;
        MEMORY[STAT] = (MEMORY[STAT] & 0xFC);
        if (MEMORY[STAT] & 0x08) {
            MEMORY[IF] |= 0x02;
        }
    }
}

static void pixel_renderer() {
    if (!PPU->POP_ENABLE || PPU->STATE != PIXEL_TRANSFER) {
        return;
    }
    //Check for window
    if ((PPU->RENDER_X == MEMORY[WX] - 7) && PPU->Y_CONDITION && (PPU->FETCH_TYPE == BACKGROUND) && (MEMORY[LCDC] & 0x20)) {
        pixel_fifo_clear(PPU->BACKGROUND_FIFO);
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
        PPU->FETCHER_X = 0;
        PPU->FETCH_TYPE = WINDOW;
        PPU->INC_WINDOW_COUNTER = true;
        return;
    }
    // Check for object
    const OAM_STRUCT* obj = heap_peek();
    if (obj && obj->x_pos < PPU->RENDER_X + 8) {
        heap_delete_min();
        obj = heap_peek();
    }
    if (obj && obj->x_pos == PPU->RENDER_X + 8) {
        PPU->POP_ENABLE = false;
        PPU->FETCH_TYPE = OBJECT;
        PPU->PIXEL_TRANSFER_STATE = FETCH_TILE;
        return;
    }
    // Pixel_renderer if enough data in fifo
    if (!pixel_fifo_is_empty(PPU->BACKGROUND_FIFO)) {
        pop_pixel();
    }
}

static void check_lyc_interrupt() {
    if (MEMORY[LYC] == MEMORY[LY]) {
        MEMORY[STAT] = (MEMORY[STAT] & 0xFB) | 0x04;
        if (MEMORY[STAT] & 0x40) {
            MEMORY[IF] |= 0x02;
        }
    }
    else {
        MEMORY[STAT] &= 0xFB;
    }
}

void h_blank() {
    if (PPU->RENDER_LINE_CYCLE == CYCLES_PER_LINE) {
        MEMORY[LY]++;
        // "At the beginning of each scanline, if the value of WY is equal to LY, the Y condition becomes true (and remains so for subsequent scanlines)."
        if (MEMORY[LY] == MEMORY[WY] && (MEMORY[LCDC] & 0x20)) {
            PPU->Y_CONDITION = true;
            PPU->WINDOW_LINE_COUNTER = 0;
        }
        else if (PPU->INC_WINDOW_COUNTER) {
            PPU->INC_WINDOW_COUNTER = false;
            PPU->WINDOW_LINE_COUNTER++;
        }
        PPU->RENDER_LINE_CYCLE = 0;
        PPU->FIRST_TILE_DONE = false;
        check_lyc_interrupt();

        if (MEMORY[LY] < WINDOW_HEIGHT) {
            PPU->STATE = OAM_SEARCH;
            MEMORY[STAT] = (MEMORY[STAT] & 0xFC) | 0x02;
            if (MEMORY[STAT] & 0x20) {
                MEMORY[IF] |= 0x02;
            }
        }
        else {
            PPU->STATE = V_BLANK;
            MEMORY[STAT] = (MEMORY[STAT] & 0xFC) | 0x01;
            if (MEMORY[STAT] & 0x10) {
                MEMORY[IF] |= 0x02;
            }
            MEMORY[IF] |= 0x01;
        }
    }
}

void v_blank() {
    if (PPU->RENDER_LINE_CYCLE != CYCLES_PER_LINE) {
        return;
    }
    else if (MEMORY[LY] < 153) {
        MEMORY[LY]++;
        check_lyc_interrupt();
        PPU->RENDER_LINE_CYCLE = 0;
    }
    else {
        MEMORY[LY] = 0;
        check_lyc_interrupt();
        PPU->RENDER_LINE_CYCLE = 0;
        PPU->WINDOW_LINE_COUNTER = 0;
        PPU->Y_CONDITION = false;
        PPU->INC_WINDOW_COUNTER = false;
        PPU->FIRST_TILE_DONE = false;
        PPU->STATE = OAM_SEARCH;
        MEMORY[STAT] = (MEMORY[STAT] & 0xFC) | 0x02;
        if (MEMORY[STAT] & 0x20) {
            MEMORY[IF] |= 0x02;
        }
        heap_clear();
        set_refresh();
    }
}

void execute_next_PPU_cycle() {
    if (PPU->PENALTY) {
        if (PPU->STATE == PIXEL_TRANSFER) {
            pixel_renderer();
        }
        PPU->PENALTY--;
        PPU->RENDER_LINE_CYCLE++;
        return;
    }

    switch (PPU->STATE) {
        case OAM_SEARCH:
            PPU->RENDER_LINE_CYCLE % 2 ? oam_search_validate() : oam_search_store();
            break;
        case PIXEL_TRANSFER:
            switch (PPU->PIXEL_TRANSFER_STATE) {
                case FETCH_TILE:
                    fetch_tile();
                    PPU->PENALTY++;
                    break;
                case GET_DATA_LOW:
                    get_tile_data_low();
                    PPU->PENALTY++;
                    break;
                case GET_DATA_HIGH:
                    get_tile_data_high();
                    PPU->PENALTY++;
                    break;
                case PUSH:
                    pixel_push();
                    break;
            }
            pixel_renderer();
            break;
        case H_BLANK:
            h_blank();
            break;
        case V_BLANK:
            v_blank();
            break;
    }
    PPU->RENDER_LINE_CYCLE++;
}