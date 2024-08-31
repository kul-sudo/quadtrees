typedef struct
{
    Vector2 pos;
    Color color;
	Vector2 speed;
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
    uint64_t element;
};

struct Node
{
    struct Node *parent;
    struct Node ***children;
    LinkedList *data;
    uint16_t layer;
    Rectangle expanded_rect;
};
