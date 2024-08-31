#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "linked_list.c"

#define N 800
#define MAX_LAYER 9
#define SPLIT_THRESHOLD 4

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 3

#define SPEED 0.1

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define clamp(value, min_value, max_value) min(max(value, min_value), max_value)

Object objects[N * sizeof(Object)];

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

                /*struct LinkedListNode *current_node = node->data->left;*/
                /**/
                /*struct LinkedListNode* to_remove[node->data->len];*/
                /*uint8_t to_remove_len = 0;*/
                /**/
                /*while (current_node != NULL)*/
                /*{*/
                /*	Object object = current_node->object;*/
                /**/
                /*	if (CheckCollisionPointRec(object.pos, new_node->rect))*/
                /*	{*/
                /*		to_remove[to_remove_len] = current_node;*/
                /*		to_remove_len++;*/
                /*		linked_list_push(new_node->data, object);*/
                /*	}*/
                /**/
                /*	current_node = current_node->right;*/
                /*}*/
                /**/
                /**/
                /*for (uint8_t index = 0; index < to_remove_len; ++index) {*/
                /*	linked_list_remove(node->data, to_remove[index]);*/
                /*}*/

                struct LinkedListNode *to_remove[node->data->len];
                uint8_t to_remove_len = 0;

                struct LinkedListNode *current_node = node->data->left;

                while (current_node != NULL)
                {
					Object *object = &objects[current_node->element];

                    if (CheckCollisionPointRec(object->pos, new_node->rect))
                    {
                        to_remove[to_remove_len] = current_node;

                        linked_list_push(new_node->data, current_node->element);
                    }

                    current_node = current_node->right;
                }

                node->children[i][j] = new_node;

                for (uint8_t index = 0; index < to_remove_len; ++index)
                {
                    linked_list_remove(node->data, to_remove[index]);
                    free(to_remove[index]);
                }
            }
        }

        node->data->len = 0;
        node->data->left = NULL;
    }
}

struct Node *find_node_by_pos(struct Node *node, Vector2 pos)
{
    if (node->children == NULL)
    {
        return node;
    }
    else
    {
		uint8_t i = (pos.y - node->rect.y) >= (node->rect.height / 2.0);
		uint8_t j = (pos.x - node->rect.x) >= (node->rect.width / 2.0);

        return find_node_by_pos(node->children[i][j], pos);
    }
}

/*struct Node *find_node_by_pos(struct Node *node, Vector2 pos)*/
/*{*/
/*    if (node->parent == NULL || CheckCollisionPointRec(pos, node->rect))*/
/*    {*/
/*        if (node->children == NULL)*/
/*        {*/
/*            return node;*/
/*        }*/
/*        else*/
/*        {*/
/*            uint8_t i = min((pos.y - node->rect.y) / (node->rect.height / 2.0), 1);*/
/*            uint8_t j = min((pos.x - node->rect.x) / (node->rect.width / 2.0), 1);*/
/**/
/*            return find_node_by_pos(node->children[i][j], pos);*/
/*        }*/
/*    }*/
/**/
/*    return find_node_by_pos(node->parent, pos);*/
/*}*/

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

void move_objects() {
	for (uint64_t i = 0; i < N; ++i) {
		Object *object = &objects[i];

		object->pos.x += object->speed.x;
		object->pos.y += object->speed.y;

		bool bounced = false;

		if (object->pos.x < OBJECT_RADIUS || object->pos.x > SCREEN_WIDTH - OBJECT_RADIUS)
		{
			object->speed.x = -object->speed.x;
			bounced = true;
		}

		if (object->pos.y < OBJECT_RADIUS || object->pos.y > SCREEN_HEIGHT - OBJECT_RADIUS)
		{
			object->speed.y = -object->speed.y;
			bounced = true;
		}

		if (bounced) {
			object->pos.x = clamp(object->pos.x, OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS);
			object->pos.y = clamp(object->pos.y, OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS);
		}

		DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, object->color);
	}
}

void handle_nodes(struct Node *node, struct Node *head)
{
    if (node->children == NULL)
    {
        DrawRectangleLines(node->rect.x, node->rect.y, node->rect.width, node->rect.height, DARKGRAY);

        struct LinkedListNode *to_remove[node->data->len];
        uint8_t to_remove_len = 0;

        struct LinkedListNode *current_node = node->data->left;

        while (current_node != NULL)
        {
            Object *object = &objects[current_node->element];

            if (!CheckCollisionPointRec(object->pos, node->rect))
            {
                struct Node *pos_node = find_node_by_pos(head, object->pos);

                to_remove[to_remove_len] = current_node;
                to_remove_len++;

                linked_list_push(pos_node->data, current_node->element);

                split_all(pos_node);

                current_node = current_node->right;

                continue;
            }
            current_node = current_node->right;

            DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, object->color);
        }

        for (uint8_t index = 0; index < to_remove_len; ++index)
        {
            linked_list_remove(node->data, to_remove[index]);
            free(to_remove[index]);
        }
    }
    else
    {
        for (uint8_t i = 0; i < 2; ++i)
        {
            for (uint8_t j = 0; j < 2; ++j)
            {
                handle_nodes(node->children[i][j], head);
            }
        }
    }
}

// Get a random value between min and max included
float rand_between(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

int main()
{
    struct Node head = {.parent = NULL,
                        .children = NULL,
                        .data = linked_list_new(),
                        .layer = 0,
                        .rect = (Rectangle){0.0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT}};

    srand(time(NULL));

    for (uint64_t i = 0; i < N; ++i)
    {
        float angle = rand_between(0, 2 * M_PI);


		objects[i] = (Object){
                                        .pos = (Vector2){rand_between(OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS),
                                                         rand_between(OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS)},
                                        .color = GRAY,
                                        .speed = (Vector2){ SPEED * cos(angle), SPEED * sin(angle) }
		};

        linked_list_push(head.data, i);
    }

    split_all(&head);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - basic window");

    while (!WindowShouldClose())
    {
        BeginDrawing();

		move_objects();
        handle_nodes(&head, &head);

        EndDrawing();

        ClearBackground(BLACK);
    }

    CloseWindow();

    return 0;
}
