
#include <stdio.h>
#include <malloc.h>
#include "TaxiElements.h"

/*
Taxi node to easily store dead taxis through node
*/

typedef struct _node  Node;
typedef struct _node
{
    Node *next;
    taxi *element;
    /*many more*/
} Node;

typedef struct _taxiNode
{
    Node *first;
    Node *last;
} taxiNode;

taxiNode *creator()
{

    taxiNode *x = (taxiNode *)malloc(sizeof(taxiNode));
    ;
    x->first = NULL;
    return x;
}

void addTaxi(taxiNode *nodes, taxi *a)
{
    Node *x = (Node *)malloc(sizeof(Node));
    x->element = a;
    x->next = NULL;

    if (nodes->first == NULL)
    {
        nodes->first = nodes->last = x;
    }
    else
    {
        nodes->last->next = x;
        nodes->last = x;
    }
}
