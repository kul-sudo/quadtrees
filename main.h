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
