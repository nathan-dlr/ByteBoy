#include <common.h>
#include <ppu.h>
#include <min_heap.h>
#include <stdio.h>

#define OBJ_HEAP_CAPACITY 10

void heap_init() {
    OBJ_HEAP = (object_min_heap*)malloc(sizeof(object_min_heap));
    OBJ_HEAP->size = 0;
    OBJ_HEAP->capacity = OBJ_HEAP_CAPACITY;
    OBJ_HEAP->objects = (OAM_STRUCT**)malloc(OBJ_HEAP_CAPACITY * sizeof(OAM_STRUCT*));

    for (int i = 0; i < OBJ_HEAP_CAPACITY; i++) {
        OBJ_HEAP->objects[i] = (OAM_STRUCT*) calloc(1, sizeof(OAM_STRUCT));
    }
}

void heap_free() {
    for (int i = 0; i < OBJ_HEAP_CAPACITY; i++) {
        free(OBJ_HEAP->objects[i]);
    }
    free(OBJ_HEAP->objects);
    free(OBJ_HEAP);
}

static void copy_object(OAM_STRUCT* dest, const OAM_STRUCT* src) {
    dest->y_pos = src->y_pos;
    dest->x_pos = src->x_pos;
    dest->tile_index = src->tile_index;
    dest->priority = src->priority;
    dest->y_flip = src->y_flip;
    dest->x_flip = src->x_flip;
    dest->palette = src->palette;
    dest->address = src->address;
}

 void heap_insert(const OAM_STRUCT* object) {
    if (OBJ_HEAP->size < OBJ_HEAP->capacity) {
        copy_object(OBJ_HEAP->objects[OBJ_HEAP->size], object);
        heapify_up(OBJ_HEAP->size);
        OBJ_HEAP->size++;
    }
}

OAM_STRUCT* heap_peek() {
    if (OBJ_HEAP->size == 0) {
        return NULL;
    }
    else {
        return OBJ_HEAP->objects[0];
    }
}

void heap_delete_min() {
    if (OBJ_HEAP->size == 0) return;
    copy_object(OBJ_HEAP->objects[0], OBJ_HEAP->objects[OBJ_HEAP->size - 1]);
    OBJ_HEAP->size--;
    heapify_down();
}

void heapify_up(uint8_t index) {
    if (index == 0) {
        return;
    }

    while (index > 0) {
        uint8_t parent = (index - 1) / 2;

        OAM_STRUCT* child_obj = OBJ_HEAP->objects[index];
        OAM_STRUCT* parent_obj = OBJ_HEAP->objects[parent];

        if (child_obj->x_pos >= parent_obj->x_pos) {
            break;
        }

        OAM_STRUCT* temp = OBJ_HEAP->objects[parent];
        OBJ_HEAP->objects[parent] = OBJ_HEAP->objects[index];
        OBJ_HEAP->objects[index] = temp;

        index = parent;
    }
}

static int8_t compare_nodes(int8_t current, int8_t left, int8_t right) {
    int8_t min = current;

    if (left < OBJ_HEAP->size) {
        OAM_STRUCT* left_obj = OBJ_HEAP->objects[left];
        OAM_STRUCT* current_obj = OBJ_HEAP->objects[min];
        if (left_obj->x_pos < current_obj->x_pos) {
            min = left;
        }
    }

    if (right < OBJ_HEAP->size) {
        OAM_STRUCT* right_obj = OBJ_HEAP->objects[right];
        OAM_STRUCT* current_obj = OBJ_HEAP->objects[min];
        if (right_obj->x_pos < current_obj->x_pos) {
            min = right;
        }
    }
    return min;
}

void heapify_down() {
    OAM_STRUCT* temp = NULL;
    int8_t left = 1;
    int8_t right = 2;
    int8_t current = 0;
    int8_t min = compare_nodes(current, left, right);

    while (min != current) {
        temp = OBJ_HEAP->objects[min];
        OBJ_HEAP->objects[min] = OBJ_HEAP->objects[current];
        OBJ_HEAP->objects[current] = temp;

        current = min;
        left = current * 2 + 1;
        right = current * 2 + 2;
        min = compare_nodes(current, left, right);
    }
}

void heap_clear() {
    OBJ_HEAP->size = 0;
}
