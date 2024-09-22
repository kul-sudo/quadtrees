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

#define N 1500
#define USE_CELLS true
#define G 1

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define OBJECT_RADIUS 3
/*#define OBJECT_RADIUS 2*/

#define SPEED 0.2

#define SCREEN_CONST 0.2
/*#define SCREEN_CONST 0.5*/

Object objects[N * sizeof(Object)];
size_t objects_len = N;

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
    Vector2 b = Vector2Subtract(lhs_object->pos, rhs_object->pos);

    float c = Vector2DotProduct(Vector2Subtract(lhs_object->speed, rhs_object->speed), b);
    Vector2 e = Vector2Scale(b, c);
	Vector2 d = Vector2Subtract(lhs_object->speed, Vector2Scale(e, 1 / distance_squared));

	/*if (isnan(d) || )*/

    return d;
}

void break_connection(Object *object) {
	objects[object->overlapped_object_id].overlapped_object_id = -1;
	object->overlapped_object_id = -1;
}

void split(struct Node *node)
{
    if (!USE_CELLS)
    {
        return;
    }

    register size_t len = 0;
    for (size_t i = 0; i < node->data_len; ++i) {
		Object *object = &objects[node->data[i]];

        if (CheckCollisionPointRec(object->pos, normalize_rect(&node->expanded_rect)))
        {
            len++;
        }
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

				new_node->expanded_rect = (Rectangle){
                    node->expanded_rect.x + j * (node->expanded_rect.width / 2.0 - OBJECT_RADIUS * 2),
                    node->expanded_rect.y + i * (node->expanded_rect.height / 2.0 - OBJECT_RADIUS * 2),
                    node->expanded_rect.width / 2.0 + OBJECT_RADIUS * 2,
                    node->expanded_rect.height / 2.0 + OBJECT_RADIUS * 2,
                };

				size_t m = 0;

				for (size_t k = 0; k < node->data_len; ++k) {
					Object *object = &objects[node->data[k]];

					if (CheckCollisionPointRec(object->pos, new_node->expanded_rect))
					{
						m++;
					}
				}

                new_node->parent = node;
                new_node->children = NULL;
                new_node->data = malloc(m * sizeof(size_t));
				new_node->data_len = 0;
                new_node->layer = node->layer + 1;

				for (size_t k = 0; k < node->data_len; ++k) {
					Object *object = &objects[node->data[k]];

					if (CheckCollisionPointRec(object->pos, new_node->expanded_rect))
					{
						new_node->data[new_node->data_len++] = node->data[k];
					}
				}

				for (size_t n = 0; n < new_node->data_len; ++n) {
					Object *object = &objects[new_node->data[n]];
				}

                node->children[from_2d(i, j)] = new_node;
            }
        }
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
 
        for (size_t i = 0; i < node->data_len; ++i) {
            Object *object = &objects[node->data[i]];

            object->is_in_normalized_rect = CheckCollisionPointRec(object->pos, normalized_rect);
		}
        
        for (size_t i = 0; i < node->data_len; ++i) {
            Object *object = &objects[node->data[i]];

            if (object->overlapped_object_id != -1)
            {
                continue;
            }

			for (size_t k = 0; k < i; ++k) {
                Object *object_nested = &objects[node->data[k]];

				if (
					object_nested->overlapped_object_id != -1 ||
					(!object->is_in_normalized_rect && !object_nested->is_in_normalized_rect))
				{
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

					object_nested->overlapped_object_id = node->data[i];
					object->overlapped_object_id = node->data[k];

					break;
				}
			}
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

void handle_objects()
{
    for (size_t i = 0; i < N; ++i)
    {
		Object *object = &objects[i];

        object->pos.x += object->speed.x;
        object->pos.y += object->speed.y;

        object->edge_collided = false;

        if (object->pos.x < OBJECT_RADIUS || object->pos.x > screen_size.x - OBJECT_RADIUS)
        {
            object->edge_collided = true;
			object->speed.x *= -1;
        }

        if (object->pos.y < OBJECT_RADIUS || object->pos.y > screen_size.y - OBJECT_RADIUS)
        {
            object->edge_collided = true;
			object->speed.y *= -1;
		}

        if (object->overlapped_object_id != -1 &&
            (object->edge_collided ||
             Vector2Distance(object->pos, objects[object->overlapped_object_id].pos) > OBJECT_RADIUS * 2))
        {
			break_connection(object);
        }

		if (object->edge_collided)
        {
            object->pos.x = Clamp(object->pos.x, OBJECT_RADIUS, screen_size.x - OBJECT_RADIUS);
            object->pos.y = Clamp(object->pos.y, OBJECT_RADIUS, screen_size.y - OBJECT_RADIUS);
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

                free(child->data);

				if (child->children != NULL) {
					free(child->children);
				}

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

		if (!tree_rebuild_needed) {
			for (size_t i = 0; i < node->data_len; ++i) {
				Object *object = &objects[node->data[i]];

				if (!CheckCollisionPointRec(object->pos, node->expanded_rect))
				{
					tree_rebuild_needed = true;
					break;
				}
			}
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
        for (size_t index = 0; index < N; ++index)
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
    }
}

int main()
{
	InitWindow(0, 0, "raylib [core] example - basic window");

    screen_size = (Vector2){GetScreenWidth(), GetScreenHeight()};

    ToggleFullscreen();

    struct Node head = {.parent = NULL,
                        .children = NULL,
						.data = malloc(N * sizeof(size_t)),
						.data_len = N,
                        .layer = 0,
                        .expanded_rect =
                            (Rectangle){-OBJECT_RADIUS * 2, -OBJECT_RADIUS * 2, screen_size.x + OBJECT_RADIUS * 4,
                                        screen_size.y + OBJECT_RADIUS * 4}};
    ;

    srand(time(NULL));

    for (size_t i = 0; i < N; ++i)
    {
        float angle = rand_between(0, 2 * M_PI);

        objects[i] = (Object){.pos = find_position(&head),
			.speed = (Vector2){SPEED * cos(angle), SPEED * sin(angle)},
			.edge_collided = false,
			.overlapped_object_id = -1,
			.is_in_normalized_rect = false,
		};

		head.data[i] = i;
    }

    split_all(&head);

    __int128_t clocks = 0;
    __int128_t cycles = 0;

    while (!WindowShouldClose())
    {
        BeginDrawing();

        size_t before = clock();

        handle_collisions(&head);
   
        clocks += clock() - before;
        cycles++;

		handle_objects();
        
		if (IsKeyPressed(KEY_SPACE)) {
			for (size_t i = 0; i < N; ++i)
			{
				Object *object = &objects[i];

				float angle = rand_between(0, 2 * M_PI);

				if (object->overlapped_object_id != -1)
				{
					break_connection(object);
				}

				objects[i] = (Object){.pos = find_position(&head),
					.speed = (Vector2){SPEED * cos(angle), SPEED * sin(angle)},
					.edge_collided = false,
					.overlapped_object_id = -1,
					.is_in_normalized_rect = false,
				};
			}
		}

		handle_nodes(&head);

        DrawText(TextFormat("%.1e", (float)(clocks / cycles)), 10, 10, 40, WHITE);

        EndDrawing();

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
