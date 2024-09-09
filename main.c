#include <float.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "linked_list.c"

#define N 25500
#define USE_CELLS true
#define G 10

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 3
/*#define OBJECT_RADIUS 2*/

#define BOSS_RADIUS 10
#define BOSS_SPEED 1
#define GRAVITY 10560
/*#define GRAVITY 120*/

#define SPEED 0.1

#define SCREEN_CONST 1
/*#define SCREEN_CONST 0.5*/

Object objects[N * sizeof(Object)];
size_t objects_len = N;

Boss boss;
Vector2 screen_size;

Rectangle normalize_rect(Rectangle *rect)
{
    return (Rectangle){rect->x + OBJECT_RADIUS * 2, rect->y + OBJECT_RADIUS * 2, rect->width - OBJECT_RADIUS * 4,
                       rect->height - OBJECT_RADIUS * 4};
}

size_t from_2d(size_t i, size_t j)
{
    return i * 2 + j;
}

Vector2 get_speed(Object *lhs_object, Object *rhs_object, float distance_squared)
{
    Vector2 a = Vector2Scale(lhs_object->speed, distance_squared);
    Vector2 b = Vector2Subtract(lhs_object->pos, rhs_object->pos);

    float c = Vector2DotProduct(Vector2Subtract(lhs_object->speed, rhs_object->speed), b);
    Vector2 e = Vector2Scale(b, c);
    Vector2 d = Vector2Subtract(a, e);

    return Vector2Scale(d, 1 / distance_squared);
}

void split(struct Node *node)
{
    if (!USE_CELLS)
    {
        return;
    }

    size_t len = 0;

    struct LinkedListNode *current_node = node->data->left;

    while (current_node != NULL)
    {
        Object *object = &objects[current_node->element];

        if (CheckCollisionPointRec(object->pos, normalize_rect(&node->expanded_rect)))
        {
            len++;
        }

        current_node = current_node->right;
    }

    float wb = node->expanded_rect.width / (OBJECT_RADIUS * 4);
    float hb = node->expanded_rect.height / (OBJECT_RADIUS * 4);
    float b = ((wb + 1) * (hb + 1)) / (4 * wb * hb);

    if ((len * b) >= 1 && len * (1 - 4 * pow(b, 2)) > G)
    {
        node->children = malloc(sizeof(struct Node[2][2]));

        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                struct Node *new_node = malloc(sizeof(struct Node));

                new_node->parent = node;
                new_node->children = NULL;
                new_node->data = linked_list_new();
                new_node->layer = node->layer + 1;
                new_node->expanded_rect = (Rectangle){
                    node->expanded_rect.x + j * (node->expanded_rect.width / 2.0 - OBJECT_RADIUS * 2),
                    node->expanded_rect.y + i * (node->expanded_rect.height / 2.0 - OBJECT_RADIUS * 2),
                    node->expanded_rect.width / 2.0 + OBJECT_RADIUS * 2,
                    node->expanded_rect.height / 2.0 + OBJECT_RADIUS * 2,
                };

                struct LinkedListNode *current_node = node->data->left;

                while (current_node != NULL)
                {
                    Object *object = &objects[current_node->element];

                    if (CheckCollisionPointRec(object->pos, new_node->expanded_rect))
                    {
                        linked_list_push(new_node->data, current_node->element);
                    }

                    current_node = current_node->right;
                }

                node->children[from_2d(i, j)] = new_node;
            }
        }

        /*node->data->len = 0;*/
        /*node->data->left = NULL;*/
    }
}

void split_all(struct Node *node)
{
    split(node);

    if (node->children != NULL)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                split_all(node->children[from_2d(i, j)]);
            }
        }
    }
}

void adjust_speed_to_boss(Object *object)
{
    float distance_to_boss = Vector2Distance(object->pos, boss.pos);

    object->speed = Vector2Add(
        object->speed, Vector2Scale(Vector2Subtract(boss.pos, object->pos), GRAVITY / pow(distance_to_boss, 3)));
}

void handle_collisions(struct Node *node)
{
    if (node->children == NULL)
    {
        Rectangle normalized_rect = normalize_rect(&node->expanded_rect);
        struct LinkedListNode *current_object = node->data->left;

        while (current_object != NULL)
        {
            Object *object = &objects[current_object->element];
            object->is_in_normalized_rect = CheckCollisionPointRec(object->pos, normalized_rect);
            current_object = current_object->right;
        }

        current_object = node->data->left;
        while (current_object != NULL)
        {
            Object *object = &objects[current_object->element];

            if (object->edge_collided || object->overlapped_object_id != -1)
            {
                current_object = current_object->right;
                continue;
            }

            struct LinkedListNode *current_object_nested = node->data->left;

            while (current_object_nested->element != current_object->element)
            {
                Object *object_nested = &objects[current_object_nested->element];
                if (object_nested->edge_collided || object_nested->overlapped_object_id != -1 ||
                    (!object->is_in_normalized_rect && !object_nested->is_in_normalized_rect))
                {
                    current_object_nested = current_object_nested->right;
                    continue;
                }

                float distance = Vector2Distance(object->pos, object_nested->pos);
                if (distance < OBJECT_RADIUS * 2)
                {
                    float distance_squared = pow(distance, 2);

                    Vector2 object_nested_speed = get_speed(object_nested, object, distance_squared);
                    Vector2 object_speed = get_speed(object, object_nested, distance_squared);

                    object->speed = object_speed;
                    object_nested->speed = object_nested_speed;

                    object_nested->overlapped_object_id = current_object->element;
                    object->overlapped_object_id = current_object_nested->element;

                    break;
                }

                current_object_nested = current_object_nested->right;
            }

            current_object = current_object->right;
        }
    }
    else
    {
        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                handle_collisions(node->children[from_2d(i, j)]);
            }
        }
    }
}

void move_objects()
{
    for (size_t i = 0; i < N; ++i)
    {
        Object *object = &objects[i];

        adjust_speed_to_boss(object);

        object->pos.x += object->speed.x;
        object->pos.y += object->speed.y;

		/*if (isnan(object->pos.x) || isnan(object->pos.y)) {*/
		/*	for (size_t k = i; k < objects_len; ++k) {*/
		/*		objects[k] = objects[k + 1];*/
		/*	}*/
		/**/
		/*	objects_len--;*/
		/*}*/

        if (object->edge_collided)
        {
            object->pos.x = Clamp(object->pos.x, OBJECT_RADIUS, screen_size.x - OBJECT_RADIUS);
            object->pos.y = Clamp(object->pos.y, OBJECT_RADIUS, screen_size.y - OBJECT_RADIUS);
        }

        DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, DARKGREEN);
    }
}

void move_boss()
{
    boss.pos.x += boss.speed.x;
    boss.pos.y += boss.speed.y;

    if (boss.pos.x < -BOSS_RADIUS)
    {
        boss.pos.x = screen_size.x + BOSS_RADIUS;
    }
    else if (boss.pos.x > screen_size.x + BOSS_RADIUS)
    {
        boss.pos.x = -BOSS_RADIUS;
    }

    if (boss.pos.y < -BOSS_RADIUS)
    {
        boss.pos.y = screen_size.y + BOSS_RADIUS;
    }
    else if (boss.pos.y > screen_size.y + BOSS_RADIUS)
    {
        boss.pos.y = -BOSS_RADIUS;
    }
}

void free_tree(struct Node *node)
{
    if (node->children != NULL)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                struct Node *child = node->children[from_2d(i, j)];

                free_tree(child);

                struct LinkedListNode *current_node = child->data->left;

                while (current_node != NULL)
                {
                    struct LinkedListNode *next_node = current_node->right;

                    free(current_node);

                    current_node = next_node;
                }

                free(child->data);
                free(child->children);
                free(child);
            }
        }
    }
}

bool tree_rebuild_needed = false;

void handle_nodes(struct Node *node)
{
    if (node->children == NULL)
    {
        Rectangle normalized_rect = normalize_rect(&node->expanded_rect);
        DrawRectangleLines(normalized_rect.x, normalized_rect.y, normalized_rect.width, normalized_rect.height,
                           DARKGRAY);

        struct LinkedListNode *current_node = node->data->left;

        while (current_node != NULL)
        {
            Object *object = &objects[current_node->element];

            if (!CheckCollisionPointRec(object->pos, node->expanded_rect))
            {
                tree_rebuild_needed = true;
            }

            current_node = current_node->right;
        }
    }
    else
    {
        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                handle_nodes(node->children[from_2d(i, j)]);
            }
        }
    }
}

// Get a random value between min and max included
float rand_between(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

Vector2 find_position(struct Node *head)
{
    Vector2 pos;

    while (true)
    {
        pos = (Vector2){rand_between(OBJECT_RADIUS, fmin(screen_size.x * SCREEN_CONST, screen_size.x - OBJECT_RADIUS)),
                        rand_between(OBJECT_RADIUS, fmin(screen_size.y * SCREEN_CONST, screen_size.y - OBJECT_RADIUS))};

        bool found = true;
        for (size_t index = 0; index < head->data->len; ++index)
        {
            if (Vector2Distance(objects[index].pos, pos) < OBJECT_RADIUS * 2)
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            return pos;
        }
    }
}

void overlapped_info_cleanup()
{
    for (size_t i = 0; i < N; ++i)
    {
        Object *object = &objects[i];

        object->edge_collided = false;

        if (object->pos.x < OBJECT_RADIUS || object->pos.x > screen_size.x - OBJECT_RADIUS)
        {
            object->edge_collided = true;
        }

        if (object->pos.y < OBJECT_RADIUS || object->pos.y > screen_size.y - OBJECT_RADIUS)
        {
            object->edge_collided = true;
        }

        if (object->edge_collided)
        {
            object->speed = (Vector2){0, 0};
        }

        if (object->overlapped_object_id != -1 &&
            (object->edge_collided ||
             Vector2Distance(object->pos, objects[object->overlapped_object_id].pos) > OBJECT_RADIUS * 2))
        {
            objects[object->overlapped_object_id].overlapped_object_id = -1;
            object->overlapped_object_id = -1;
        }
    }
}

int main()
{
    InitWindow(0, 0, "raylib [core] example - basic window");

    int display = GetCurrentMonitor();
    screen_size = (Vector2){GetMonitorWidth(display), GetMonitorHeight(display)};

    ToggleFullscreen();

    struct Node head = {.parent = NULL,
                        .children = NULL,
                        .data = linked_list_new(),
                        .layer = 0,
                        .expanded_rect =
                            (Rectangle){-OBJECT_RADIUS * 2, -OBJECT_RADIUS * 2, screen_size.x + OBJECT_RADIUS * 4,
                                        screen_size.y + OBJECT_RADIUS * 4}};
    ;

    srand(time(NULL));

    float boss_angle = rand_between(0, 2 * M_PI);
    boss = (Boss){(Vector2){screen_size.x / 2, screen_size.y / 2},
                  (Vector2){BOSS_SPEED * cos(boss_angle), BOSS_SPEED * sin(boss_angle)}};

    for (size_t i = 0; i < N; ++i)
    {
        float angle = rand_between(0, 2 * M_PI);

        objects[i] = (Object){.pos = find_position(&head),
                              .speed = (Vector2){SPEED * cos(angle), SPEED * sin(angle)},
                              .edge_collided = false,
                              .overlapped_object_id = -1,
                              .is_in_normalized_rect = false};

        linked_list_push(head.data, i);
    }

    split_all(&head);

    __int128_t clocks = 0;
    __int128_t cycles = 0;

    while (!WindowShouldClose())
    {
        if (IsMouseButtonDown(0))
        {
            boss.pos = (Vector2){Clamp(-BOSS_RADIUS, screen_size.x + BOSS_RADIUS, GetMouseX()),
                                 Clamp(-BOSS_RADIUS, screen_size.y + BOSS_RADIUS, GetMouseY())};
        }

        BeginDrawing();

        size_t before = clock();

        overlapped_info_cleanup();

        handle_collisions(&head);
        handle_nodes(&head);
        move_objects();

        clocks += clock() - before;
        cycles++;

        DrawText(TextFormat("%.1e", (float)(clocks / cycles)), 10, 10, 40, WHITE);

        DrawCircle(boss.pos.x, boss.pos.y, BOSS_RADIUS, YELLOW);

        EndDrawing();

        move_boss();

        if (tree_rebuild_needed)
        {
            free_tree(&head);

            srand(time(NULL));

            split_all(&head);

            tree_rebuild_needed = false;
        }

        ClearBackground(BLACK);
    }

    CloseWindow();

    return 0;
}
