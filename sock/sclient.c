#include "sclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "sutil.h"
#include "cmd/argtable3.h"

static void right_rotate(struct sclient *fn, struct sclient **root);
static void left_right_rotate(struct sclient *fn, struct sclient **root);
static void left_rotate(struct sclient *fn, struct sclient **root);
static void right_left_rotate(struct sclient *fn, struct sclient **root);
static void sclient_line_node_print(struct sclient *c);
static void sclient_size_cb(struct sclient *cur, void *data);
static struct sclient *sclient_new(int fd, struct sockaddr_in *caddr, struct sclient *fn);
static struct sclient *sclient_prev_pn_find(struct sclient *cur);
static struct sclient **sclient_prev_ppn_find(struct sclient *cur);
static struct sclient **sclient_ppn_find(struct sclient **ppn, int fd);
static struct sclient **sclient_fn_ppn_find(struct sclient **ppn, struct sclient **fn, int fd);
static struct sclient **sclient_max_ppn_find(struct sclient *cur);
static void sclient_swap(struct sclient *d, struct sclient *s);
static int sclient_close(struct sclient *pn);

/******************************************************************************
* brief    : init a AVL tree
* Param    : 
* Return   :
* Note     :
******************************************************************************/
void sclient_init(struct sclient_mgr *m)
{
    if(NULL != m)
        m->root = NULL;
}

/******************************************************************************
* brief    : add a node into avl tree
* Param    : 
* Return   :
* Note     :
******************************************************************************/
int sclient_add(struct sclient_mgr *m, int fd, struct sockaddr_in *addr)
{
    struct sclient **ppn = &m->root;
    struct sclient **root = ppn;
    struct sclient *fn = NULL;
    struct sclient *cn = NULL;     /* current node */

    ppn = sclient_fn_ppn_find(ppn, &fn, fd);
    if(NULL != *ppn)
    {
        SOCK_ERR("fd(%d) has been added! -> ignore\r\n", fd);
        close(fd);      /* close client's connection */
        return -1;
    }

    *ppn = sclient_new(fd, addr, fn);
    
    /* do add balance */
    cn = *ppn;
    while(NULL != (fn=cn->p))
    {
        if(cn == fn->l)
        {
            if(1 == fn->factor)
            {
                if(-1 == cn->factor)
                    left_right_rotate(fn, root);
                else if(1 == cn->factor)
                    right_rotate(fn, root);
                else
                    ;            /* impossible */
                break;
            }
            else if(-1 == fn->factor)
            {
                fn->factor = 0;
                break;
            }
            else
                fn->factor = 1;
        }
        else
        {
            if(-1 == fn->factor)
            {
                if(1 == cn->factor)
                    right_left_rotate(fn, root);
                else if(-1 == cn->factor)
                    left_rotate(fn, root);
                else
                    ;           /* impossible */
                break;
            }
            else if(1 == fn->factor)
            {
                fn->factor = 0;
                break;
            }
            else
                fn->factor = -1;
        }

        cn = cn->p;     /* continue backtrace: update father node's factor */
    }
    
    return 0;
}

/******************************************************************************
* brief    : do balance after del a node 
* Param    : 
* Return   :
* Note     :
******************************************************************************/
static int del_balance(struct sclient *cn, struct sclient **root)
{
    struct sclient *fn;            /* father node */
    struct sclient *sn = NULL;     /* slibing node */

    while(NULL!=cn && NULL!=(fn=cn->p))
    {
        if(cn == fn->l)
        {
            if(-1 == fn->factor)    /* cn==fn->l && -1==fn->factor indicating that sn is not NULL */
            {
                sn = fn->r;
                if(0==sn->factor) 
                {
                    left_rotate(fn, root);
                    break;          /* stop backtrace : height is not changed*/
                }
                else if(-1 == sn->factor)
                {
                    left_rotate(fn, root);
                    cn = cn->p;     /* must do: update factor from cn->father */
                }
                else if(1 == sn->factor)
                {
                    right_left_rotate(fn, root);
                    cn = cn->p;     /* must do: update factor from cn->father */
                }
            }
            else if(0 == fn->factor)
            {
                fn->factor = -1;
                break;
            }
            else
                fn->factor = 0;
        }
        else
        {
            if(1 == fn->factor)     /* cn==fn->r && 1==fn->factor indicating that sn is not NULL */
            {
                sn = fn->l;
                if(0 == sn->factor)
                {
                    right_rotate(fn, root);
                    break;          /* stop backtrace : height is not changed*/
                }
                else if(1 == sn->factor)
                {
                    right_rotate(fn, root);
                    cn = cn->p;     /* must do: update factor from cn->father */
                }
                else if(-1 == sn->factor)
                {
                    left_right_rotate(fn, root);
                    cn = cn->p;     /* must do: update factor from cn->father */
                }
            }
            else if(0 == fn->factor)
            {
                fn->factor = 1; 
                break;
            }
            else
                fn->factor = 0;
        }

        cn = cn->p;     /* continue backtrace: update father node's factor */
    }

    return 0;
}

/******************************************************************************
* brief    : del a node from avl tree
* Param    : 
* Return   :
* Note     :
******************************************************************************/
int sclient_del(struct sclient_mgr *m, int fd)
{
    struct sclient **ppn = &m->root;
    struct sclient **root = ppn;
    struct sclient *cn = NULL;
    struct sclient *fn = NULL;     /* father node */
    struct sclient *nn = NULL;     /* next node */

    ppn = sclient_ppn_find(ppn, fd);
    if(NULL == *ppn)
    {
        SOCK_ERR("del: fd(%d) is not found! -> ignore\r\n", fd);
        return -1;
    }
    
    cn = *ppn;      /* save current client */
    if(NULL!=cn->l &&  NULL!=cn->r)
    {
        ppn = sclient_max_ppn_find(cn);
        sclient_swap(cn, *ppn);
        cn = *ppn;  /* change left child's max client */
    }
    
    nn = (NULL==cn->l)? cn->r: cn->l;
    if(NULL == nn)  /* deleted node has no child, responding to if(NULL != nn) as following */
        del_balance(cn, root);

    fn = cn->p;     /* record deleted's father node */
    if(NULL != fn)
        ppn = (cn==fn->l)? &(fn->l): &(fn->r);
    else 
        ppn = root;
    *ppn = nn;      /* remove cn node from tree */

    if(NULL != nn)  /* deleted node has one child */
    {
        nn->p = fn;
        del_balance(nn, root);
    }

    sclient_close(cn);

    return 0;
}


/******************************************************************************
* brief    : find a client by fd
* Param    : 
* Return   :
* Note     :
******************************************************************************/
struct sclient *sclient_find(struct sclient_mgr *m, int fd)
{
    struct sclient *pn = m->root;

    while(NULL != pn)
    {
        if(fd < pn->fd)
            pn = pn->l;
        else if(pn->fd < fd)
            pn = pn->r;
        else
            break;
    }

    return pn;
}

/******************************************************************************
* brief    : iterate avl tree, call cb callback for every node 
* Param    : 
* Return   :
* Note     :
******************************************************************************/
void sclient_iterate(struct sclient_mgr *m, sclient_iterate_t cb, void *data)
{
    struct sclient *cur = m->root;
    struct sclient *pn = NULL;

    while(NULL != cur) 
    {
        if(NULL == cur->l)
        {
            cb(cur, data);      /* call function */
            cur = cur->r;
        }
        else
        {                       /* find previous node of current node */
            pn = sclient_prev_pn_find(cur);
            if(NULL == pn->r)   /* first meet, change cn->r = cn */
            {
                pn->r = cur;
                cur = cur->l;
            }
            else                /* second meet, change cn->r = NULL back */
            {
                pn->r = NULL;
                cb(cur, data);  /* call function */
                cur = cur->r;
            }
        }
    }

    return;
}

/******************************************************************************
* brief    : iterate avl tree by level method
* Param    : 
* Return   :
* Note     :
******************************************************************************/
void sclient_level_iterate(struct sclient_mgr *m, sclient_iterate_t cb, void *data)
{
    struct sclient *cur = m->root;
    struct sclient **que = NULL;
    int front = 0;
    int rear = 0;

    if(NULL==cur || NULL==(que = malloc(sizeof(struct client *) * sclient_size(m))))
        return;
    
    while(rear!=front || NULL!=cur)
    {
        if(NULL != cur)
        {
            cb(cur, data);
            if(NULL != cur->l)
                que[front++] = cur->l;
            if(NULL != cur->r)
                que[front++] = cur->r;
            cur = NULL;
        }
        else
            cur = que[rear++];
    }
    
    return;
}

/******************************************************************************
* brief    : get avl tree size
* Param    : 
* Return   :
* Note     :
******************************************************************************/
int sclient_size(struct sclient_mgr *m)
{
    int n = 0;

    sclient_iterate(m, sclient_size_cb, &n);

    return n;
}

/******************************************************************************
* brief    : hand all client's event
* Param    : 
* Return   :
* Note     :
******************************************************************************/
int sclient_hand(struct sclient_mgr *m, struct pollfd *fds)
{
    struct sclient **ppn = &m->root;
    struct sclient *pn = *ppn;
    ssize_t len = 0;
    u8_t buff[100];

    bzero(buff, sizeof(buff));
    len = read(fds->fd, buff, sizeof(buff)-1);
    if(len <= 0)        /* close    */
        sclient_del(m, fds->fd);
    else                /* read or write data */
    {
        pn = sclient_find(m, fds->fd);
        if(NULL != pn)
            sclient_msg_print(pn, buff, len);
    }

    return 0;
}

/******************************************************************************
* brief    : uninit the whole client AVL tree
* Param    : 
* Return   :
* Note     :
******************************************************************************/
void sclient_uninit(struct sclient_mgr *m)
{
    struct sclient *cur = m->root;
    struct sclient *prev;
    struct sclient *next;
    struct sclient **ppn;

    while(NULL != cur)
    {
        if(NULL == cur->l)
        {
            next = cur->r;
            sclient_close(cur);
            cur = next;
        }
        else
        {
            ppn = sclient_max_ppn_find(cur);
            (*ppn)->r = cur;
            prev = cur->l;
            cur->l = NULL;
            cur = prev;
        }
    }

    m->root = NULL;

    return;
}

/****** for debug ******/
void sclient_msg_print(struct sclient *c, u8_t *msg, size_t len)
{
    SOCK_PRINT("(%s:%u(%d)) # %lu # %s\r\n", c->ip, c->port, c->fd, len, msg);
}

void sclient_del_print(struct sclient *c)
{
    SOCK_INFO("del client: ");
    sclient_line_node_print(c);
}

void sclient_list_node_recursive(struct sclient *c)
{
    if(NULL != c)
    {
        sclient_list_node_recursive(c->l);
        SOCK_PRINT( SOCK_SPLIT );
        sclient_line_node_print(c);
        sclient_list_node_recursive(c->r);
    }
    return;
}
void sclient_list_print(struct sclient_mgr *m)
{
    sclient_list_node_recursive(m->root);

    return;
}

int sclient_list(struct sclient_mgr *m, arg_dstr_t ds)
{
    struct sclient *c = m->root;

    arg_dstr_catf(ds, "****** tcps client list summary ******\r\n");


    sclient_iterate(m, sclient_dump_cb, ds);

    return 0;
}

int sclient_level_list(struct sclient_mgr *m, arg_dstr_t ds)
{
    arg_dstr_catf(ds, "****** tcps client level list summary ******\r\n");

    sclient_level_iterate(m, sclient_dump_cb, ds);

    return 0;

}
int sclient_line_dump(struct sclient *c, arg_dstr_t ds)
{
    arg_dstr_catf(ds, "| %-15p | %-15p | %-15p | %-5d | %-15s | %-5u | %-10s |\r\n", 
            c->l, c, c->r, c->fd, c->ip, c->port, c->ltime);


    return 0;
}
void sclient_dump_cb(struct sclient *c, void *data)
{
    arg_dstr_t ds = (arg_dstr_t)data;

    arg_dstr_catf(ds, SOCK_SPLIT);
    sclient_line_dump(c, ds);

    return;
}


void sclient_tree_node_print(struct sclient *n, int level, const char *fmt, ...)
{
    va_list ap;
    int i = 0;

    for(i=0; i < level; i++)
        putchar('\t');

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    return;
}

void sclient_tree_node_recursive(struct sclient *n, int level)
{
    if(NULL == n)
        sclient_tree_node_print(n, level, "%c\r\n", '~');
    else
    {
        sclient_tree_node_recursive(n->r, level+1);
        sclient_tree_node_print(n, level, "%d(%d)\r\n", n->fd, n->factor);
        sclient_tree_node_recursive(n->l, level+1);
    }

    return;
}

void sclient_tree_print(struct sclient_mgr *m)
{
    sclient_tree_node_recursive(m->root, 1);
    return;
}

/*******************************************************
          (1)                     
          k2(fn)                        k1
        /   \                         /    \
    (1)k1    |C|          ==>      |A|      k2
      /  \   (h)                 (h+1)     /  \
   |A|    |B|                            |B|    |C|
   (h+1)  (h)                           (h)    (h)

 k1   k2   | A   B  C   |  k1  k2 |     H        | do
---del--------------------------------------------------
 0    1    | h   h  h-1 | -1   1  | h+2 -> h+2   | break
 1    1    | h+1 h  h   |  0   0  | h+3 -> h+2   | backtrace
---add--------------------------------------------------
 1    1    | h+1 h  h   |  0   0  | h+2 -> h+2   | break

*******************************************************/
static void right_rotate(struct sclient *fn, struct sclient **root)
{
    struct sclient *sfn = fn->p;
    struct sclient **ppn = NULL;
    struct sclient *k2 = fn;
    struct sclient *k1 = k2->l;

    SOCK_INFO("right rotate -> fd(%d)!\r\n", fn->fd);

    if(NULL != sfn)
        ppn = (sfn->l==fn)? &(sfn->l): &sfn->r;
    else
        ppn = root;
    
    /* k2 node */
    k2->l = k1->r;
    if(k1->r)
        k1->r->p = k2;

    /* k1 node */
    k1->r = k2;
    k2->p = k1;
    
    *ppn = k1;
    k1->p = sfn;

    if(0 == k1->factor)
        k1->factor=-1, k2->factor=1;
    else
        k1->factor = k2->factor = 0;
}

/*********************************************************************
         (1)             
          k3(fn)                            k2
        /   \                            /      \
   (-1)k1    |D|         ==>           k1         k3
      /   \                           /  \       /  \
   |A|     k2                       |A|   |B|  |C|  |D|
          /  \
       |B|    |C|

k1  k2  k3  | A    B    C    D   | k1  k2  k3  |   H        |  do
---del-----------------------------------------------------------
-1  -1  1   | h+1  h    h+1  h+1 | 1   0   0   | h+4 -> h+3 | backtrace
-1  0   1   | h    h    h    h   | 0   0   0   | h+3 -> h+2 | backtrace
-1  1   1   | h+1  h+1  h    h+1 | 0   0   -1  | h+4 -> h+3 | backtrace
---add-----------------------------------------------------------
-1  1   1   | h+1  h+1  h    h+1 | 0   0   -1  | h+3 -> h+3 | break;
-1  0   1   | h    h    h    h   | 0   0   0   | h+3 -> h+2 | break;  impossible
-1 -1   1   | h+1  h    h+1  h+1 | 1   0   0   | h+3 -> h+3 | break;

**********************************************************************/
static void left_right_rotate(struct sclient *fn, struct sclient **root)
{
    struct sclient *sfn = fn->p;
    struct sclient **ppn = NULL;
    struct sclient *k3 = fn;
    struct sclient *k1 = k3->l;
    struct sclient *k2 = k1->r;

    SOCK_INFO("left(%d) right(%d) rotate!\r\n", fn->l->fd, fn->fd);
    if(NULL != sfn)
        ppn = (sfn->l==fn)? &(sfn->l): &(sfn->r);
    else
        ppn = root;

    /* k1 node */
    k1->r = k2->l; 
    if(NULL != k2->l)
        k2->l->p = k1;
    k1->p = k2;

    /* k3 node */
    k3->l = k2->r;
    if(NULL != k2->r)
        k2->r->p = k3;
    k3->p = k2;

    /* k2 node */
    k2->l = k1; 
    k2->r = k3;
    k2->p = sfn;

    *ppn = k2;

    if(1 == k2->factor)
        k1->factor=0, k2->factor=0, k3->factor=-1;
    else if(0 == k2->factor)                /* impossible when avl_add */
        k1->factor = k2->factor = k3->factor = 0;
    else
        k1->factor=1, k2->factor=0, k3->factor=0;

    return;
}

/*******************************************************
          -1         
          k1(fn)                         k2
        /   \                          /   \
     |A|     k2(-1|0)   ==>          k1     |C|
            /  \                    /  \
         |B|    |C|              |A|    |B|
 
 k1   k2   | A   B   C   |  k1  k2 |     H        | do
---del-------------------------------------------------
-1   -1    | h   h   h+1 |  0   0  | h+3 -> h+2   | backtrace
-1    0    | h-1 h   h   | -1   1  | h+2 -> h+2   | break
---add-------------------------------------------------
-1   -1    | h   h   h+1 |  0   0  | h+2 -> h+2   | break

*******************************************************/
static void left_rotate(struct sclient *fn, struct sclient **root)
{
    struct sclient *sfn = fn->p;
    struct sclient **ppn = NULL;
    struct sclient *k1 = fn;
    struct sclient *k2 = k1->r;

    SOCK_INFO("left rotate -> fd(%d)!\r\n", fn->fd);
    if(NULL != sfn) 
        ppn = (fn==sfn->l)? &sfn->l: &sfn->r;
    else
        ppn = root;

    /* k1 node */
    k1->r = k2->l;
    if(NULL != k2->l)
        k2->l->p = k1;

    /* k2 node */
    k2->l = k1;
    k1->p = k2;

    k2->p = sfn;
    *ppn = k2;

    if(-1 == k2->factor)
        k1->factor = k2->factor = 0;
    else
        k1->factor=-1, k2->factor=1;
}
/**********************************************************************
            -1
             k1(fn)                        k2
           /    \                       /     \
        |A|       k3(1)               k1         k3
                /   \       ==>     /   \        /  \
             k2      |D|        |A|      |B|  |C|    |D|
            /  \
         |B|    |C|

k1  k2  k3  | A    B    C    D   | k1  k2  k3  |   H        |  do
---del----------------------------------------------------------
-1  1   1   | h+1  h+1  h    h+1 | 0   0   -1  | h+4 -> h+3 | backtrace;
-1  0   1   | h    h    h    h   | 0   0   0   | h+3 -> h+2 | backtrace;
-1 -1   1   | h+1  h    h+1  h+1 | 1   0   0   | h+4 -> h+3 | backtrace;
---add----------------------------------------------------------
-1  1   1   | h+1  h+1  h    h+1 | 0   0   -1  | h+3 -> h+3 | break;
-1  0   1   | h    h    h    h   | 0   0   0   | h+3 -> h+2 | break;  impossible
-1 -1   1   | h+1  h    h+1  h+1 | 1   0   0   | h+3 -> h+3 | break;

**********************************************************************/
static void right_left_rotate(struct sclient *fn, struct sclient **root)
{
    struct sclient *sfn = fn->p;
    struct sclient **ppn = NULL;
    struct sclient *k1 = fn;
    struct sclient *k3 = k1->r;
    struct sclient *k2 = k3->l;

    SOCK_INFO("right(%d) left(%d) rotate!\r\n", fn->r->fd, fn->fd);
    if(NULL != sfn)
        ppn = (fn==sfn->l)? &(sfn->l): &(sfn->r);
    else
        ppn = root;

    /* k1 node */
    k1->r = k2->l;
    if(NULL != k2->l)
        k2->l->p = k1;
    k1->p = k2;

    /* k3 node */
    k3->l = k2->r;
    if(NULL != k2->r)
        k2->r->p = k3;
    k3->p = k2;
    
    /* k2 node */
    k2->l = k1;
    k2->r = k3;
    k2->p = sfn;

    *ppn = k2;
    if(1 == k2->factor)
        k1->factor=0, k2->factor=0, k3->factor=-1;
    else if(0 == k2->factor)
        k1->factor = k2->factor = k3->factor = 0;
    else
        k1->factor=1, k2->factor=0, k3->factor=0;

    return;
}

static void sclient_size_cb(struct sclient *cur, void *data)
{
    (*(int *)data)++;
    return;
}

static void sclient_line_node_print(struct sclient *c)
{
    SOCK_PRINT("| %-15p | %-15p | %-15p | %-5d | %-15s | %-5u | %-10s |\r\n", 
            c->l, c, c->r, c->fd, c->ip, c->port, c->ltime);
}

static struct sclient *sclient_new(int fd, struct sockaddr_in *caddr, struct sclient *fn)
{
    struct sclient *c;
    time_t curtime;

    c = malloc(sizeof(struct sclient));
    if(NULL != c)
    {
        c->p = fn;
        c->l = c->r = NULL;
        c->factor = 0;
        c->fd = fd;
        c->port = ntohs(caddr->sin_port);
        snprintf(c->ip, sizeof(c->ip), "%s", inet_ntoa(caddr->sin_addr));
        time(&curtime);
        strftime(c->ltime, sizeof(c->ltime), "%H:%M:%S", localtime(&curtime));
    }
    
    return c;
}

static struct sclient *sclient_prev_pn_find(struct sclient *cur)
{
    struct sclient *pn = cur->l;

    while(NULL!=pn->r && cur!=pn->r)
        pn = pn->r;
    
    return pn;
}

static struct sclient **sclient_prev_ppn_find(struct sclient *cur)
{
    struct sclient **ppn = &(cur->l);

    while(NULL!=(*ppn)->r && cur!=(*ppn)->r)
        ppn = &((*ppn)->r);

    return ppn;
}

static struct sclient **sclient_ppn_find(struct sclient **ppn, int fd)
{
    while(NULL != *ppn)
    {
        if(fd < (*ppn)->fd)
            ppn = &((*ppn)->l);
        else if((*ppn)->fd < fd)
            ppn = &((*ppn)->r);
        else
            break;
    }

    return ppn;
}

static struct sclient **sclient_fn_ppn_find(struct sclient **ppn, struct sclient **fn, int fd)
{
    while(NULL != *ppn)
    {
        if(fd < (*ppn)->fd)
            *fn = *ppn, ppn = &((*ppn)->l);
        else if((*ppn)->fd < fd)
            *fn = *ppn, ppn = &((*ppn)->r);
        else
            break;
    }

    return ppn;
}

static struct sclient **sclient_max_ppn_find(struct sclient *cur)
{
    struct sclient **ppn = &(cur->l);

    while(NULL != (*ppn)->r)
        ppn = &((*ppn)->r);

    return ppn;
}

static void sclient_swap(struct sclient *d, struct sclient *s)
{
    struct sclient tmp;

    if(NULL!=d && NULL!=s)
    {
        tmp.fd = d->fd;
        tmp.port = d->port;
        strcpy(tmp.ip, d->ip);
        strcpy(tmp.ltime, d->ltime);

        d->fd = s->fd;
        d->port = s->port;
        strcpy(d->ip, s->ip);
        strcpy(d->ltime, s->ltime);

        s->fd = tmp.fd;
        s->port = tmp.port;
        strcpy(s->ip, tmp.ip);
        strcpy(s->ltime, tmp.ltime);
    }
    return;
}

/******************************************************************************
* brief    : close a client
* Param    : 
* Return   :
* Note     :
******************************************************************************/
static int sclient_close(struct sclient *pn)
{
    if(NULL != pn)
    {
        sclient_del_print(pn); /* only for debug */

        pn->p = pn->l = pn->r = NULL;
        pn->factor = 0;
        close(pn->fd);
        pn->fd = 0;
        pn->port = 0;
        free(pn);
    }

    return 0;
}
