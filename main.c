#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "linked_list.c"

#define N 400
#define MAX_LAYER 9
#define SPLIT_THRESHOLD 2

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 3

#define SPEED 0.1

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define clamp(value, min_value, max_value) min(max(value, min_value), max_value)

Object objects[N * sizeof(Object)];

Rectangle normalize_rect(Rectangle *rect)
{
    return (Rectangle){rect->x + OBJECT_RADIUS * 2, rect->y + OBJECT_RADIUS * 2, rect->width - OBJECT_RADIUS * 4,
                       rect->height - OBJECT_RADIUS * 4};
}

size_t from_2d(size_t i, size_t j) {
	return i * 2 + j;
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

void move_objects()
{
    for (size_t i = 0; i < N; ++i)
    {
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

        if (bounced)
        {
            object->pos.x = clamp(object->pos.x, OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS);
            object->pos.y = clamp(object->pos.y, OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS);
        }

        DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, object->color);
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

void handle_nodes(struct Node *node, struct Node *head)
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

            DrawCircle(object->pos.x, object->pos.y, OBJECT_RADIUS, object->color);
        }
    }
    else
    {
        for (size_t i = 0; i < 2; ++i)
        {
            for (size_t j = 0; j < 2; ++j)
            {
                handle_nodes(node->children[from_2d(i, j)], head);
            }
        }
    }
}

// Get a random value between min and max included
float rand_between(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

float Vector2Distance(Vector2 lhs, Vector2 rhs) {
	return sqrt(pow((lhs.x - rhs.x), 2) + pow((lhs.y - rhs.y), 2));
}

Vector2 find_position(struct Node *head) {
	Vector2 pos;

	while (true) {
		pos = (Vector2){rand_between(OBJECT_RADIUS, SCREEN_WIDTH - OBJECT_RADIUS),
			rand_between(OBJECT_RADIUS, SCREEN_HEIGHT - OBJECT_RADIUS)};

		bool found = true;
		for (size_t index = 0; index < head->data->len; ++index) {
			if (Vector2Distance(objects[index].pos, pos) <= OBJECT_RADIUS * 2) {
				found = false;
				break;
			}
		}

		if (found) {
			return pos;
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
                              .color = GRAY,
                              .speed = (Vector2){SPEED * cos(angle), SPEED * sin(angle)}};

		linked_list_push(head.data, i);
    }

    split_all(&head);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - basic window");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        move_objects();
        handle_nodes(&head, &head);

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
