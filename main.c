#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "voxel.h"

/* =========================================================
 *  Глобальные переменные — границы модели
 * ========================================================= */
float x_max = 0;
float x_min = 10;
float y_max = 0;
float y_min = 10;
float z_max = 0;
float z_min = 10;
int   vert_count = 0;

/* =========================================================
 *  make_voxel
 *  Создаёт пустой воксель. Память под вершины НЕ выделяется —
 *  она будет выделена при первом da_append в ind_finder.
 * ========================================================= */
Voxel make_voxel(int cap, float size)
{
    (void)cap; /* параметр оставлен для совместимости сигнатуры */
    Voxel vx = {
        .items    = NULL,
        .count    = 0,
        .capacity = 0,
        .vx_center = (Vector3){0.0f, 0.0f, 0.0f},
        .size     = size,
    };
    return vx;
}

/* =========================================================
 *  make_vxlist
 *  Создаёт список вокселей заданной ёмкости.
 *  calloc гарантирует, что все поля Voxel.items == NULL,
 *  count == 0, capacity == 0 — без дополнительной инициализации.
 * ========================================================= */
vxlist make_vxlist(int cap)
{
    Voxel *buf = calloc(cap, sizeof(Voxel));
    assert(buf != NULL);
    vxlist lst = {
        .items    = buf,
        .count    = 0,
        .capacity = cap,
    };
    return lst;
}

/* =========================================================
 *  freeContainer
 *  Освобождает все внутренние массивы вокселей,
 *  затем сам массив. Итерация по capacity — не по count,
 *  т.к. ind_finder пишет напрямую по индексу, минуя count.
 * ========================================================= */
void freeContainer(vxlist *c)
{
    if (c == NULL) return;
    if (c->items != NULL) {
        for (int i = 0; i < c->capacity; i++) {
            if (c->items[i].items != NULL) {
                free(c->items[i].items);
                c->items[i].items    = NULL;
                c->items[i].count    = 0;
                c->items[i].capacity = 0;
            }
        }
        free(c->items);
        c->items = NULL;
    }
    c->count    = 0;
    c->capacity = 0;
}

/* =========================================================
 *  verts_from_ply
 *  Считывает вершины XYZ из PLY-файла.
 *  Заполняет глобальные min/max и vert_count.
 * ========================================================= */
Vector3 *verts_from_ply(char *filename, Vector3 *verties)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Файл не был открыт\n");
        exit(EXIT_FAILURE);
    }

    char line[255];
    int  counter     = 1;
    int  vertex_num  = 0;
    bool end_header  = false;
    int  cycle_count = 100000; /* достаточно большое число */

    while (fgets(line, sizeof(line), f) && counter < cycle_count) {

        if (!strncmp(line, "element vertex", 14)) {
            char *ptr_x = strchr(line, 'x');
            if (ptr_x != NULL) {
                ptr_x += 2;
                vertex_num = atoi(ptr_x);
            }
        }

        if (!strncmp(line, "end_header", 10)) {
            end_header  = true;
            cycle_count = counter + vertex_num;
            verties     = malloc(vertex_num * sizeof(Vector3));
            assert(verties != NULL);
            continue;
        }

        counter++;

        if (end_header) {
            /* --- парсинг X --- */
            verties[vert_count].x = atof(line);
            if (verties[vert_count].x > x_max) x_max = verties[vert_count].x;
            if (verties[vert_count].x < x_min) x_min = verties[vert_count].x;

            /* --- парсинг Y --- */
            char *ptr_line = strchr(line, ' ');
            if (ptr_line == NULL) { vert_count++; continue; }
            ++ptr_line;
            verties[vert_count].y = atof(ptr_line);
            if (verties[vert_count].y > y_max) y_max = verties[vert_count].y;
            if (verties[vert_count].y < y_min) y_min = verties[vert_count].y;

            /* --- парсинг Z --- */
            ptr_line = strchr(ptr_line, ' ');
            if (ptr_line == NULL) { vert_count++; continue; }
            ++ptr_line;
            verties[vert_count].z = atof(ptr_line);
            if (verties[vert_count].z > z_max) z_max = verties[vert_count].z;
            if (verties[vert_count].z < z_min) z_min = verties[vert_count].z;

            vert_count++;
        }
    }

    fclose(f);
    return verties;
}

/* =========================================================
 *  normalize_verties
 *  Нормализует координаты в диапазон [0, norm_factor].
 *  Обновляет глобальные min/max.
 * ========================================================= */
void normalize_verties(Vector3 *verties, float norm_factor)
{
    float x_max_n = 0, y_max_n = 0, z_max_n = 0;
    float x_min_n = norm_factor, y_min_n = norm_factor, z_min_n = norm_factor;

    for (int i = 0; i < vert_count; i++) {
        verties[i].x = norm_factor * (verties[i].x - x_min) / (x_max - x_min);
        verties[i].y = norm_factor * (verties[i].y - y_min) / (y_max - y_min);
        verties[i].z = norm_factor * (verties[i].z - z_min) / (z_max - z_min);

        if (verties[i].x > x_max_n) x_max_n = verties[i].x;
        if (verties[i].x < x_min_n) x_min_n = verties[i].x;
        if (verties[i].y > y_max_n) y_max_n = verties[i].y;
        if (verties[i].y < y_min_n) y_min_n = verties[i].y;
        if (verties[i].z > z_max_n) z_max_n = verties[i].z;
        if (verties[i].z < z_min_n) z_min_n = verties[i].z;
    }

    x_max = x_max_n; x_min = x_min_n;
    y_max = y_max_n; y_min = y_min_n;
    z_max = z_max_n; z_min = z_min_n;
}

/* =========================================================
 *  parallel_mesh
 *  Заполняет mesh_vox вокселями равномерной сетки.
 *  Воксели создаются с items == NULL (память под вершины
 *  выделяется позже в ind_finder).
 * ========================================================= */
void parallel_mesh(int voxel_num, vxlist *mesh_vox,
                   float wall_w, float wall_h, float wall_l,
                   float voxel_w, Vector3 start_pos)
{
    /*
     * Выделяем сразу точный буфер под voxel_num вокселей через calloc —
     * это гарантирует items == NULL у каждого вокселя и избавляет от
     * цепочки realloc с незнулёванными хвостами.
     */
    if (mesh_vox->items != NULL) {
        free(mesh_vox->items);
    }
    mesh_vox->items    = calloc(voxel_num, sizeof(Voxel));
    assert(mesh_vox->items != NULL);
    mesh_vox->count    = voxel_num;
    mesh_vox->capacity = voxel_num;

    float cx = start_pos.x;
    float cy = start_pos.y;
    float cz = start_pos.z;

    for (int i = 0; i < voxel_num; i++) {
        mesh_vox->items[i].size        = voxel_w;
        mesh_vox->items[i].vx_center.x = cx;
        mesh_vox->items[i].vx_center.y = cy;
        mesh_vox->items[i].vx_center.z = cz;
        /* items/count/capacity уже 0/NULL благодаря calloc */

        /* Шаг по X */
        cx += voxel_w;
        if (cx >= start_pos.x + wall_w - voxel_w * 0.5f) {
            cx = start_pos.x;
            cy += voxel_w;
            if (cy >= start_pos.y + wall_h - voxel_w * 0.5f) {
                cy = start_pos.y;
                cz += voxel_w;
                if (cz >= start_pos.z + wall_l - voxel_w * 0.5f) {
                    cz = start_pos.z;
                }
            }
        }
    }
}

/* =========================================================
 *  point_in_voxel
 * ========================================================= */
bool point_in_voxel(Voxel vx, Vector3 vert)
{
    float half = vx.size * 0.5f;
    return (vert.x >= vx.vx_center.x - half && vert.x <= vx.vx_center.x + half &&
            vert.y >= vx.vx_center.y - half && vert.y <= vx.vx_center.y + half &&
            vert.z >= vx.vx_center.z - half && vert.z <= vx.vx_center.z + half);
}

/* =========================================================
 *  ind_finder
 *  Распределяет вершины по вокселям.
 *  nx/ny/nz вычисляются из кубического корня voxel_num,
 *  чтобы гарантировать ind < voxel_num без погрешностей float.
 * ========================================================= */
void ind_finder(vxlist *mesh, Vector3 *vert,
                float voxel_w, float parallel_x, float parallel_y, float parallel_z)
{
    /* n = кубический корень числа вокселей (5 для 125, 25 для 15625 и т.д.) */
    int n  = (int)round(cbrt((double)mesh->count));
    int nx = n, ny = n, nz = n;
    (void)parallel_x; (void)parallel_y; (void)parallel_z; /* не нужны при целом n */

    for (int i = 0; i < vert_count; i++) {
        int xi = (int)(vert[i].x / voxel_w);
        int yi = (int)(vert[i].y / voxel_w);
        int zi = (int)(vert[i].z / voxel_w);

        /* Зажимаем в допустимый диапазон */
        if (xi < 0)  xi = 0; else if (xi >= nx) xi = nx - 1;
        if (yi < 0)  yi = 0; else if (yi >= ny) yi = ny - 1;
        if (zi < 0)  zi = 0; else if (zi >= nz) zi = nz - 1;

        int ind = zi * (nx * ny) + yi * nx + xi;

        /* Первый вызов da_append для этого вокселя — memory выделяется здесь */
        Voxel *vx = &mesh->items[ind];
        if (vx->items == NULL) {
            vx->items    = calloc(1, sizeof(Vector3));
            assert(vx->items != NULL);
            vx->capacity = 1;
            vx->count    = 0;
        }

        /* da_append для вокселя */
        if (vx->count >= vx->capacity) {
            int new_cap  = vx->capacity * 2;
            Vector3 *tmp = realloc(vx->items, new_cap * sizeof(Vector3));
            assert(tmp != NULL);
            /* Обнуляем новую половину буфера */
            memset(tmp + vx->capacity, 0, vx->capacity * sizeof(Vector3));
            vx->items    = tmp;
            vx->capacity = new_cap;
        }
        vx->items[vx->count++] = vert[i];
    }
}

/* =========================================================
 *  vxCompare  (не используется активно, оставлена для совместимости)
 * ========================================================= */
int vxCompare(const void *a, const void *b)
{
    const Voxel *v1 = (const Voxel *)a;
    const Voxel *v2 = (const Voxel *)b;
    if (v1->count > v2->count) return  1;
    if (v1->count < v2->count) return -1;
    return 0;
}

/* =========================================================
 *  vx_swap
 * ========================================================= */
void vx_swap(Voxel *a, Voxel *b)
{
    Voxel tmp = *a;
    *a = *b;
    *b = tmp;
}

/* =========================================================
 *  create_mesh
 *  Пересоздаёт сетку с новым разрешением.
 *  freeContainer вызывается снаружи перед этой функцией.
 * ========================================================= */
void create_mesh(vxlist *mesh_vox, float cube_volume, int voxel_num,
                 float parallel_x, float parallel_y, float parallel_z,
                 float *voxel_w)
{
    float voxel_volume = cube_volume / voxel_num;
    *voxel_w = cbrtf(voxel_volume);
    Vector3 start_mesh = {
        x_min + (*voxel_w) * 0.5f,
        y_min + (*voxel_w) * 0.5f,
        z_min + (*voxel_w) * 0.5f
    };
    /* make_vxlist не нужен — parallel_mesh сам выделяет буфер нужного размера */
    mesh_vox->items    = NULL;
    mesh_vox->count    = 0;
    mesh_vox->capacity = 0;
    parallel_mesh(voxel_num, mesh_vox,
                  parallel_x, parallel_y, parallel_z,
                  *voxel_w, start_mesh);
}

/* =========================================================
 *  main
 * ========================================================= */
int main(void)
{
    InitWindow(800, 800, "3D Stuff");
    SetTargetFPS(60);

    /* --- Загрузка и нормализация модели --- */
    Vector3 *vertices = NULL;
    /* Укажите путь к PLY-файлу в папке models/ */
    char *obj = "models/bun_zipper.ply";
    vertices = verts_from_ply(obj, vertices);
    normalize_verties(vertices, 5.0f);

    float parallel_x  = x_max - x_min;
    float parallel_y  = y_max - y_min;
    float parallel_z  = z_max - z_min;
    float cube_volume = parallel_x * parallel_y * parallel_z;

    int   voxel_num    = 125;
    float voxel_volume = cube_volume / voxel_num;
    float voxel_w      = cbrtf(voxel_volume);

    /* --- Первоначальное построение сетки --- */
    vxlist mesh_vox = {.items = NULL, .count = 0, .capacity = 0};
    Vector3 start_mesh = {
        x_min + voxel_w * 0.5f,
        y_min + voxel_w * 0.5f,
        z_min + voxel_w * 0.5f
    };
    parallel_mesh(voxel_num, &mesh_vox,
                  parallel_x, parallel_y, parallel_z,
                  voxel_w, start_mesh);
    ind_finder(&mesh_vox, vertices, voxel_w,
               parallel_x, parallel_y, parallel_z);

    /* --- UI state --- */
    bool dropdownEditMode   = false;
    int  activeDropdownItem = 0;
    int  new_mesh_reslotion = 0;
    mesh_resolution mesh_r[3] = {low, middle, hight};

    bool cameraActive        = false;
    bool startClicked        = false;
    bool voxelezation_button = false;

    Camera camera = {0};
    camera.position   = (Vector3){10.0f, 0.0f, 10.0f};
    camera.target     = (Vector3){0.0f,  0.0f,  0.0f};
    camera.up         = (Vector3){0.0f,  1.0f,  0.0f};
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    /* -------------------------------------------------------
     *  Главный цикл
     * ----------------------------------------------------- */
    while (!WindowShouldClose()) {

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            cameraActive = !cameraActive;
            EnableCursor();
        }
        if (cameraActive) {
            UpdateCamera(&camera, CAMERA_THIRD_PERSON);
            DisableCursor();
        }

        bool rise_mesh_flag = false;

        BeginDrawing();
        ClearBackground(BLACK);

        /* Выпадающий список выбора разрешения */
        if (GuiDropdownBox((Rectangle){12, 100, 140, 28},
                           "125;15625;125000",
                           &activeDropdownItem, dropdownEditMode)) {
            dropdownEditMode = !dropdownEditMode;
            if (new_mesh_reslotion != activeDropdownItem) {
                new_mesh_reslotion = activeDropdownItem;
                rise_mesh_flag     = true;
            }
        }

        /* Кнопка отображения вершин */
        if (GuiButton((Rectangle){12, 70, 140, 28}, "Draw Verts")) {
            startClicked = !startClicked;
        }

        /* Кнопка вокселизации */
        if (GuiButton((Rectangle){12, 40, 140, 28}, "voxelization")) {
            voxelezation_button = !voxelezation_button;
        }

        /* Подсказки */
        DrawRectangle(380, 700, 420, 93, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines(380, 700, 420, 93, BLUE);
        DrawText("Program control keys:",                                           500, 720, 10, BLACK);
        DrawText("- To view the model in 3D mode, press the right mouse button",   390, 740, 10, BLACK);
        DrawText("- To draw vertices, use the ''Draw Verts'' button",               390, 760, 10, BLACK);
        DrawText("- To select the grid scale, use the dropdown in the top-left",   390, 780, 10, BLACK);

        BeginMode3D(camera);

            /* Вершины модели */
            if (startClicked) {
                for (int i = 0; i < vert_count; i++) {
                    DrawCube(vertices[i], 0.005f, 0.005f, 0.005f, GREEN);
                }
            }

            /* Ограничивающий параллелепипед */
            DrawCubeWires(
                (Vector3){x_min + parallel_x * 0.5f,
                          y_min + parallel_y * 0.5f,
                          z_min + parallel_z * 0.5f},
                parallel_x, parallel_y, parallel_z, RED);

            /* Смена разрешения сетки */
            if (rise_mesh_flag) {
                voxel_num = mesh_r[activeDropdownItem];
                freeContainer(&mesh_vox);
                create_mesh(&mesh_vox, cube_volume, voxel_num,
                            parallel_x, parallel_y, parallel_z, &voxel_w);
                ind_finder(&mesh_vox, vertices, voxel_w,
                           parallel_x, parallel_y, parallel_z);
            }

            /* Отрисовка сетки вокселей */
            for (int j = 0; j < voxel_num; j++) {
                Vector3 c = mesh_vox.items[j].vx_center;
                DrawCubeWires(c, voxel_w, voxel_w, voxel_w, RED);

                if (voxelezation_button && mesh_vox.items[j].count > 1) {
                    DrawCubeWires(c, 0.05f, 0.05f, 0.05f, GREEN);
                }
            }

        EndMode3D();

        DrawFPS(700, 20);
        EndDrawing();
    }

    /* --- Очистка --- */
    freeContainer(&mesh_vox);
    free(vertices);
    CloseWindow();
    return 0;
}
