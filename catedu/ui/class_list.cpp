#include "class_list.hpp"
#include "catedu/core/alloc/free_list.hpp"
#include <cstring>

static UiClassNode *alloc_node(FreeList<UiClassNode> &list, const char *name)
{
    char *namecpy = (char *)list.arena.alloc(strlen(name) + 1);
    memcpy(namecpy, name, strlen(name) + 1);

    UiClassNode *node = list.alloc({});
    node->sub = list.arena.alloc(UiClassList::create(list.arena));
    node->sub->name = namecpy;

    return node;
}

UiClassList UiClassList::create(Arena alloc)
{
    UiClassList result = {};
    result.class_nodes = FreeList<UiClassNode>::create(alloc);
    return result;
}

void UiClassList::destroy()
{
    this->class_nodes.destroy();
}

UiClassList *UiClassList::add_class(const char *cl, void *data)
{
    if (this->root == nullptr)
    {
        this->root = alloc_node(class_nodes, cl);
        return this->root->sub;
    }

    UiClassNode *node = this->root;
    while (true)
    {
        int cmp = strcmp(cl, node->sub->name);
        if (cmp == 0)
        {
            return node->sub;
        }
        else if (cmp < 0)
        {
            if (node->left)
            {
                node = node->left;
            }
            else
            {
                node = node->left = alloc_node(class_nodes, cl);
                return node->sub;
            }
        }
        else
        {
            if (node->right)
            {
                node = node->right;
            }
            else
            {
                node = node->right = alloc_node(class_nodes, cl);
                return node->sub;
            }
        }
    }

    assert(false);
}

UiClassList *UiClassList::get_class(const char *cl)
{
    UiClassNode *node = this->root;

    while (node)
    {
        int cmp = strcmp(cl, node->sub->name);
        if (cmp == 0)
        {
            return node->sub;
        }
        else if (cmp < 0)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }
    }

    return nullptr;
}
