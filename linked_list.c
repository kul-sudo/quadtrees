LinkedList *linked_list_new()
{
    LinkedList *new = malloc(sizeof(LinkedList));
    new->left = NULL;
    new->len = 0;

    return new;
}

void linked_list_push(LinkedList *linked_list, size_t element)
{
    struct LinkedListNode *new = malloc(sizeof(struct LinkedListNode));

    new->right = linked_list->left;
    new->left = NULL;
    new->element = element;

    linked_list->left = new;

    if (new->right != NULL)
    {
        new->right->left = new;
    }

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
    /*free(linked_list_node);*/
}
