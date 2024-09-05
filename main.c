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

/*#define N 31000*/
/*#define MAX_LAYER 10*/
/*#define SPLIT_THRESHOLD 30*/
/**/
/*#define SCREEN_WIDTH 1920*/
/*#define SCREEN_HEIGHT 1080*/
/*#define OBJECT_RADIUS 1*/
/**/
/*#define SPEED 0.4*/
/**/
/*#define SCREEN_CONST 1.0*/

#define N 31000
#define MAX_LAYER 10
#define SPLIT_THRESHOLD 30

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 1

#define SPEED 0.4

#define SCREEN_CONST 0.4

Object objects[N * sizeof(Object)];

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

    if (node->layer < MAX_LAYER && len >= SPLIT_THRESHOLD)
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

        object->pos.x += object->speed.x;
        object->pos.y += object->speed.y;

        if (object->edge_collided)
        {
            object->pos.x = Clamp(object->pos.x, OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS);
            object->pos.y = Clamp(object->pos.y, OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS);
        }

        DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, DARKGREEN);
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
        pos = (Vector2){rand_between(OBJECT_RADIUS, fmin(SCREEN_WIDTH * SCREEN_CONST, SCREEN_WIDTH - OBJECT_RADIUS)),
                        rand_between(OBJECT_RADIUS, fmin(SCREEN_HEIGHT * SCREEN_CONST, SCREEN_HEIGHT - OBJECT_RADIUS))};

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

        if (object->pos.x < OBJECT_RADIUS || object->pos.x > SCREEN_WIDTH - OBJECT_RADIUS)
        {
            object->speed.x = -object->speed.x;
            object->edge_collided = true;
        }

        if (object->pos.y < OBJECT_RADIUS || object->pos.y > SCREEN_HEIGHT - OBJECT_RADIUS)
        {
            object->speed.y = -object->speed.y;
            object->edge_collided = true;
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
    struct Node head = {.parent = NULL,
                        .children = NULL,
                        .data = linked_list_new(),
                        .layer = 0,
                        .expanded_rect =
                            (Rectangle){-OBJECT_RADIUS * 2, -OBJECT_RADIUS * 2, SCREEN_WIDTH + OBJECT_RADIUS * 4,
                                        SCREEN_HEIGHT + OBJECT_RADIUS * 4}};
    ;

    srand(time(NULL));

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

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - basic window");

    size_t clocks = 0;
	size_t cycles = 0;
	
    while (!WindowShouldClose())
    {
		size_t before = clock();

        overlapped_info_cleanup();

        BeginDrawing();

        handle_collisions(&head);
		handle_nodes(&head);
		move_objects();

		clocks += clock() - before;
		cycles++;

		DrawText(TextFormat("%.1e", (float)(clocks / cycles)), 10, 10, 40, WHITE);
        
		if (tree_rebuild_needed)
        {
            free_tree(&head);

            srand(time(NULL));

            split_all(&head);

            tree_rebuild_needed = false;
        }

        EndDrawing();

        ClearBackground(BLACK);
    }

    CloseWindow();

    return 0;
}
