/*
 ============================================================================
 Name        : hev-socks5-authenticator.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2023 hev
 Description : Socks5 Authenticator
 ============================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "hev-compiler.h"
#include "hev-socks5-logger-priv.h"

#include "hev-socks5-authenticator.h"

typedef struct _HevSocks5AuthenticatorNode HevSocks5AuthenticatorNode;

struct _HevSocks5AuthenticatorNode
{
    HevRBTreeNode node;

    char *user;
    char *pass;
};

HevSocks5Authenticator *
hev_socks5_authenticator_new (void)
{
    HevSocks5Authenticator *self;
    int res;

    self = calloc (1, sizeof (HevSocks5Authenticator));
    if (!self)
        return NULL;

    res = hev_socks5_authenticator_construct (self);
    if (res < 0) {
        free (self);
        return NULL;
    }

    LOG_D ("%p socks5 authenticator new", self);

    return self;
}

int
hev_socks5_authenticator_add (HevSocks5Authenticator *self, const char *user,
                              const char *pass)
{
    HevRBTreeNode **new = &self->tree.root, *parent = NULL;
    HevSocks5AuthenticatorNode *curr;

    while (*new) {
        HevSocks5AuthenticatorNode *this;
        int res;

        this = container_of (*new, HevSocks5AuthenticatorNode, node);
        res = strcmp (this->user, user);

        parent = *new;
        if (res < 0)
            new = &((*new)->left);
        else if (res > 0)
            new = &((*new)->right);
        else
            return -1;
    }

    curr = calloc (1, sizeof (HevSocks5AuthenticatorNode));
    if (!curr)
        return -1;

    curr->user = strdup (user);
    curr->pass = strdup (pass);

    hev_rbtree_node_link (&curr->node, parent, new);
    hev_rbtree_insert_color (&self->tree, &curr->node);

    return 0;
}

int
hev_socks5_authenticator_del (HevSocks5Authenticator *self, const char *user)
{
    HevRBTreeNode *node = self->tree.root;

    while (node) {
        HevSocks5AuthenticatorNode *this;
        int res;

        this = container_of (node, HevSocks5AuthenticatorNode, node);
        res = strcmp (this->user, user);

        if (res < 0) {
            node = node->left;
        } else if (res > 0) {
            node = node->right;
        } else {
            hev_rbtree_erase (&self->tree, node);
            free (this->user);
            free (this->pass);
            free (this);

            return 0;
        }
    }

    return -1;
}

int
hev_socks5_authenticator_cmp (HevSocks5Authenticator *self, const char *user,
                              const char *pass)
{
    HevRBTreeNode *node = self->tree.root;

    while (node) {
        HevSocks5AuthenticatorNode *this;
        int res;

        this = container_of (node, HevSocks5AuthenticatorNode, node);
        res = strcmp (this->user, user);

        if (res < 0)
            node = node->left;
        else if (res > 0)
            node = node->right;
        else
            return strcmp (this->pass, pass);
    }

    return -1;
}

void
hev_socks5_authenticator_clear (HevSocks5Authenticator *self)
{
    HevRBTreeNode *n;

    while ((n = hev_rbtree_first (&self->tree))) {
        HevSocks5AuthenticatorNode *t;

        t = container_of (n, HevSocks5AuthenticatorNode, node);
        hev_rbtree_erase (&self->tree, n);
        free (t->user);
        free (t->pass);
        free (t);
    }
}

int
hev_socks5_authenticator_construct (HevSocks5Authenticator *self)
{
    int res;

    res = hev_object_construct (&self->base);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 authenticator construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_AUTHENTICATOR_TYPE;

    return 0;
}

static void
hev_socks5_authenticator_destruct (HevObject *base)
{
    HevSocks5Authenticator *self = HEV_SOCKS5_AUTHENTICATOR (base);

    LOG_D ("%p socks5 authenticator destruct", self);

    hev_socks5_authenticator_clear (self);

    HEV_OBJECT_TYPE->finalizer (base);
    free (base);
}

HevObjectClass *
hev_socks5_authenticator_class (void)
{
    static HevSocks5AuthenticatorClass klass;
    HevSocks5AuthenticatorClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        memcpy (kptr, HEV_OBJECT_TYPE, sizeof (HevObjectClass));

        okptr->name = "HevSocks5Authenticator";
        okptr->finalizer = hev_socks5_authenticator_destruct;
    }

    return okptr;
}
