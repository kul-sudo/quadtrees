typedef struct
{
    Vector2 pos;
	Vector2 speed;
	size_t overlapped_object_id;
	bool edge_collided;
	bool is_in_normalized_rect;
} Object;

/// Stack-based
typedef struct
{
    struct LinkedListNode *left;
    size_t len;
} LinkedList;

struct LinkedListNode
{
    struct LinkedListNode *left;
    struct LinkedListNode *right;
    size_t element;
};

struct Node
{
    struct Node *parent;
    struct Node **children;
    size_t *data;
	size_t data_len;
    size_t layer;
    Rectangle expanded_rect;
};

typedef struct {
	Vector2 pos;
	Vector2 speed;
} Boss;
