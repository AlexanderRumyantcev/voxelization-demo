#ifndef VOXEL_H
#define VOXEL_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* =========================================================
 *  Структуры данных
 * ========================================================= */

/**
 * @brief Воксель — кубическая ячейка сетки вокселизации.
 *
 * Хранит динамический список вершин 3D-модели, попавших
 * в данную ячейку, координаты центра и размер ребра куба.
 */
typedef struct Voxel {
    Vector3 *items;     /**< Динамический массив вершин внутри вокселя. */
    int      count;     /**< Текущее количество вершин в массиве.        */
    int      capacity;  /**< Вместимость массива (до следующего realloc). */
    Vector3  vx_center; /**< Координаты геометрического центра вокселя.  */
    float    size;      /**< Длина ребра куба (все рёбра равны).          */
} Voxel;

/**
 * @brief Динамический список вокселей — представляет сетку вокселизации.
 */
typedef struct vxlist {
    Voxel *items;    /**< Динамический массив вокселей. */
    int    count;    /**< Текущее количество вокселей.  */
    int    capacity; /**< Вместимость массива.           */
} vxlist;

/* =========================================================
 *  Перечисления
 * ========================================================= */

/**
 * @brief Варианты разрешения сетки вокселей.
 */
typedef enum {
    low    = 125,    /**< 5x5x5     =    125 вокселей. */
    middle = 15625,  /**< 25x25x25  = 15625 вокселей. */
    hight  = 125000, /**< 50x50x50  = 125000 вокселей. */
} mesh_resolution;

/* =========================================================
 *  Макросы
 * ========================================================= */

/**
 * @brief Добавляет элемент в конец динамического массива.
 *
 * При необходимости удваивает ёмкость через realloc и обнуляет
 * новую часть буфера, чтобы предотвратить чтение мусорных указателей.
 */
#define da_append(arr, item)                                                      \
    do {                                                                          \
        if ((arr)->count >= (arr)->capacity) {                                    \
            int old_cap = (arr)->capacity;                                        \
            (arr)->capacity = old_cap == 0 ? 1 : old_cap * 2;                    \
            (arr)->items = realloc((arr)->items,                                  \
                           (arr)->capacity * sizeof((arr)->items[0]));            \
            assert((arr)->items != NULL);                                         \
            if (old_cap > 0) {                                                    \
                memset((arr)->items + old_cap, 0,                                 \
                   ((arr)->capacity - old_cap) * sizeof((arr)->items[0]));        \
            }                                                                     \
        }                                                                         \
        (arr)->items[(arr)->count++] = (item);                                    \
    } while (0)

/* =========================================================
 *  Глобальные переменные (границы модели после парсинга)
 * ========================================================= */

extern float x_max;
extern float x_min;
extern float y_max;
extern float y_min;
extern float z_max;
extern float z_min;
extern int   vert_count;

/* =========================================================
 *  Прототипы функций
 * ========================================================= */

Voxel    make_voxel(int cap, float size);
vxlist   make_vxlist(int cap);
void     freeContainer(vxlist *c);

Vector3 *verts_from_ply(char *filename, Vector3 *verties);
void     normalize_verties(Vector3 *verties, float norm_factor);

void     parallel_mesh(int voxel_num, vxlist *mesh_vox,
                       float wall_w, float wall_h, float wall_l,
                       float voxel_w, Vector3 start_pos);
void     create_mesh(vxlist *mesh_vox, float cube_volume, int voxel_num,
                     float parallel_x, float parallel_y, float parallel_z,
                     float *voxel_w);

bool     point_in_voxel(Voxel vx, Vector3 vert);
void     ind_finder(vxlist *mesh, Vector3 *vert,
                    float voxel_w, float parallel_x, float parallel_y,
                    float parallel_z);

int      vxCompare(const void *a, const void *b);
void     vx_swap(Voxel *a, Voxel *b);
void     threeWayPartition(vxlist *arr, int low, int high, int *lt, int *gt);
void     threeWayQuickSort(vxlist *arr, int low, int high);

#endif /* VOXEL_H */
