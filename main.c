#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 10
#define MAX_LAYER 10
#define SPLIT_THRESHOLD 1

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 5

#define SPEED 0

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define clamp(value, min_value, max_value) min(max(value, min_value), max_value)

typedef struct
{
    Vector2 pos;
    Color color;
    float angle;
    uint16_t id;
} Object;

/// Stack-based
typedef struct
{
    struct LinkedListNode *left;
    uint16_t len;
} LinkedList;

struct LinkedListNode
{
    struct LinkedListNode *left;
    struct LinkedListNode *right;
    Object object;
};

LinkedList *linked_list_new()
{
    LinkedList *new = malloc(sizeof(LinkedList));
    new->left = NULL;
    new->len = 0;

    return new;
}

void linked_list_push(LinkedList *linked_list, Object object)
{
    struct LinkedListNode *new = malloc(sizeof(struct LinkedListNode));

    new->right = linked_list->left;
    new->left = NULL;
    new->object = object;

	if (linked_list->left != NULL) {
		linked_list->left->left = new;
	}

    linked_list->left = new;
    linked_list->len++;
}

void linked_list_remove(LinkedList *linked_list, struct LinkedListNode *linked_list_node)
{
    // Only element
    if (linked_list_node->left == NULL && linked_list_node->right == NULL)
    {
        linked_list->left = NULL;
    }
    // Middle
    else if (linked_list_node->left != NULL && linked_list_node->right != NULL)
    {
        linked_list_node->left->right = linked_list_node->right;
        linked_list_node->right->left = linked_list_node->left;
    }
    // Left
    else if (linked_list_node->left == NULL && linked_list_node->right != NULL)
    {
        linked_list_node->right->left = NULL;
        linked_list->left = linked_list_node->right;
    }
    // Right
    else if (linked_list_node->left != NULL && linked_list_node->right == NULL)
    {
        linked_list_node->left->right = NULL;
    }

    linked_list->len--;
}

typedef enum
{
    HEAD,
    NODE,
    LEAF
} NodeType;

struct Node
{
    struct Node *parent;
    struct Node ***children;
    LinkedList *data;
    uint16_t layer;
    Rectangle rect;
};

void split(struct Node *node)
{
    if (node->layer < MAX_LAYER && node->data->len >= SPLIT_THRESHOLD)
    {
        node->children = malloc(4 * sizeof(struct Node));

        for (uint8_t index = 0; index < 2; ++index)
        {
            node->children[index] = malloc(2 * sizeof(struct Node));
        }

        for (uint8_t i = 0; i < 2; ++i)
        {
            for (uint8_t j = 0; j < 2; ++j)
            {
                struct Node *new_node = malloc(sizeof(struct Node));

                new_node->parent = node;
                new_node->children = NULL;
                new_node->data = linked_list_new();
                new_node->layer = node->layer + 1;
                new_node->rect = (Rectangle){
                    node->rect.x + j * node->rect.width / 2.0,
                    node->rect.y + i * node->rect.height / 2.0,
                    node->rect.width / 2.0,
                    node->rect.height / 2.0,
                };

                if (node->data->len > 0)
                {
                    struct LinkedListNode *current_node = node->data->left;

                    while (current_node != NULL)
                    {
                        Object object = current_node->object;

                        if (CheckCollisionPointRec(object.pos, new_node->rect))
                        {
                            linked_list_push(new_node->data, object);
                            linked_list_remove(node->data, current_node);
                        }

                        current_node = current_node->right;
                    }
                }

                node->children[i][j] = new_node;
            }
        }
    }
}

struct Node *find_node_by_pos(struct Node *node, Vector2 pos)
{
    if (node->parent == NULL || CheckCollisionPointRec(pos, node->rect))
    {
        if (node->children == NULL)
        {
            return node;
        }
        else
        {
            uint8_t i = min((pos.y - node->rect.y) / (node->rect.height / 2.0), 1);
            uint8_t j = min((pos.x - node->rect.x) / (node->rect.width / 2.0), 1);

            return find_node_by_pos(node->children[i][j], pos);
        }
    }

    return find_node_by_pos(node->parent, pos);
}

void split_all(struct Node *node)
{
    split(node);

    if (node->children != NULL)
    {
        for (uint8_t i = 0; i < 2; ++i)
        {
            for (uint8_t j = 0; j < 2; ++j)
            {
                split_all(node->children[i][j]);
            }
        }
    }
}

void draw_all(struct Node *node)
{
    if (node->children == NULL)
    {
        DrawRectangleLines(node->rect.x, node->rect.y, node->rect.width, node->rect.height, BLACK);
    }

    if (node->data->len > 0)
    {
        struct LinkedListNode *current_node = node->data->left;

        while (current_node != NULL)
        {
            Object *object = &current_node->object;

            if (!CheckCollisionPointRec(object->pos, node->rect))
            {
                struct Node *pos_node = find_node_by_pos(node, object->pos);

                linked_list_push(pos_node->data, *object);
                linked_list_remove(node->data, current_node);

                split_all(pos_node);

                current_node = current_node->right;

                continue;
            }

            if (object->pos.x < OBJECT_RADIUS || object->pos.x > SCREEN_WIDTH - OBJECT_RADIUS)
            {
                object->angle = M_PI - object->angle;
            }

            if (object->pos.y < OBJECT_RADIUS || object->pos.y > SCREEN_HEIGHT - OBJECT_RADIUS)
            {
                object->angle = 2.0 * M_PI - object->angle;
            }

            object->pos.x = clamp(object->pos.x, OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS);
            object->pos.y = clamp(object->pos.y, OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS);

            Vector2 deviation = (Vector2){SPEED * cos(object->angle), SPEED * sin(object->angle)};

            object->pos.x += deviation.x;
            object->pos.y += deviation.y;
            current_node = current_node->right;

            if (node->children == NULL)
            {
                DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, object->color);
            }
        }
    }

    if (node->children != NULL)
    {
        for (uint8_t i = 0; i < 2; ++i)
        {
            for (uint8_t j = 0; j < 2; ++j)
            {
                draw_all(node->children[i][j]);
            }
        }
    }
}

int main()
{
    struct Node head = {.parent = NULL,
                        .children = NULL,
                        .data = linked_list_new(),
                        .layer = 0,
                        .rect = (Rectangle){0.0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT}};

    SetRandomSeed(time(NULL));

    for (uint64_t i = 0; i < N; ++i)
    {
        linked_list_push(head.data, (Object){
                                        .pos = (Vector2){GetRandomValue(OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS),
                                                         GetRandomValue(OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS)},
                                        .color = BLACK,
                                        .angle = GetRandomValue(0, 2 * M_PI),
                                    });
    }

    split_all(&head);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - basic window");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        draw_all(&head);

        EndDrawing();

        ClearBackground(RAYWHITE);
    }

    CloseWindow();

    return 0;
}
