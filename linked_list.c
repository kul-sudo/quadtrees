/*LinkedList *linked_list_new()*/
/*{*/
/*    LinkedList *new = malloc(sizeof(LinkedList));*/
/*    new->left = NULL;*/
/*    new->len = 0;*/
/**/
/*    return new;*/
/*}*/
/**/
/*void linked_list_push(LinkedList *linked_list, size_t element)*/
/*{*/
/*    struct LinkedListNode *new = malloc(sizeof(struct LinkedListNode));*/
/**/
/*    new->right = linked_list->left;*/
/*    new->left = NULL;*/
/*    new->element = element;*/
/**/
/*    linked_list->left = new;*/
/**/
/*    linked_list->len++;*/
/*}*/
