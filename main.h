typedef struct
{
    Vector2 pos;
    Color color;
	Vector2 speed;
    size_t id;
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
    LinkedList *data;
    size_t layer;
    Rectangle expanded_rect;
};
