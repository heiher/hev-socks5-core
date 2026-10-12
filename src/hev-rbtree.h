/*
 ============================================================================
 Name        : hev-rbtree.h
 Authors     : Andrea Arcangeli <andrea@suse.de>
               David Woodhouse <dwmw2@infradead.org>
               Michel Lespinasse <walken@google.com>
               Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : RedBlack Tree
 ============================================================================
 */

#ifndef __HEV_RBTREE_H__
#define __HEV_RBTREE_H__

#include <stddef.h>
#include <stdint.h>

/* Parent pointers use their two least significant bits for the node color. */
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _HevRBTree HevRBTree;
typedef struct _HevRBTreeNode HevRBTreeNode;

struct _HevRBTree
{
    HevRBTreeNode *root;
};

/* The parent pointer stores two tag bits, so alignment is part of the ABI. */
#if defined(_MSC_VER)
#if defined(_WIN64)
#define HEV_RBTREE_NODE_ALIGN_PREFIX __declspec (align (8))
#else
#define HEV_RBTREE_NODE_ALIGN_PREFIX __declspec (align (4))
#endif
#define HEV_RBTREE_NODE_ALIGN_SUFFIX
#else
#define HEV_RBTREE_NODE_ALIGN_PREFIX
#define HEV_RBTREE_NODE_ALIGN_SUFFIX \
    __attribute__ ((aligned (sizeof (uintptr_t))))
#endif

struct HEV_RBTREE_NODE_ALIGN_PREFIX _HevRBTreeNode
{
    uintptr_t __parent_color;
    HevRBTreeNode *right;
    HevRBTreeNode *left;
} HEV_RBTREE_NODE_ALIGN_SUFFIX;
#undef HEV_RBTREE_NODE_ALIGN_PREFIX
#undef HEV_RBTREE_NODE_ALIGN_SUFFIX

static inline int
hev_rbtree_node_empty (HevRBTreeNode *node)
{
    return node->__parent_color == (uintptr_t)node;
}

static inline HevRBTreeNode *
hev_rbtree_node_parent (HevRBTreeNode *node)
{
    return (HevRBTreeNode *)(node->__parent_color & ~3);
}

static inline void
hev_rbtree_node_link (HevRBTreeNode *node, HevRBTreeNode *parent,
                      HevRBTreeNode **link)
{
    node->__parent_color = (uintptr_t)parent;
    node->left = node->right = NULL;

    *link = node;
}

HevRBTreeNode *hev_rbtree_node_prev (HevRBTreeNode *node);
HevRBTreeNode *hev_rbtree_node_next (HevRBTreeNode *node);

HevRBTreeNode *hev_rbtree_first (HevRBTree *self);
HevRBTreeNode *hev_rbtree_last (HevRBTree *self);

void hev_rbtree_insert_color (HevRBTree *self, HevRBTreeNode *node);

void hev_rbtree_replace (HevRBTree *self, HevRBTreeNode *victim,
                         HevRBTreeNode *new);

void hev_rbtree_erase (HevRBTree *self, HevRBTreeNode *node);

#if defined(__cplusplus)
}
#endif

#endif /* __HEV_RBTREE_H__ */
