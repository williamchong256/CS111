//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

//
//  SortedList.c
//  lab2_add
//
//  Created by William Chong on 5/10/20.
//  Copyright © 2020 William Chong. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "SortedList.h"


/**
* SortedList_insert ... insert an element into a sorted list
*
*  list goes from low to high (ascending)
 
*    The specified element will be inserted in to
*    the specified list, which will be kept sorted
*    in ascending order based on associated keys
*
* @param SortedList_t *list ... header for the list
* @param SortedListElement_t *element ... element to be added to the list
*/
void SortedList_insert(SortedList_t *list, SortedListElement_t * element)
{
    if (opt_yield & INSERT_YIELD)
    {
        sched_yield();
    }
    //lab2_list.c will handle malloc
    SortedListElement_t* curr = list;

    //if empty list insert after head, i.e. next pointer is the head, or has NULL key
    if ((curr->next)->key == NULL)
    {
        curr->next = element;
        curr->prev = element;
        element->next = list;
        element->prev = list;
        return;
    }

    curr = list->next;

    //not empty list, find correct place to put element
    //continue iterating through if havent come back to head,
    //and if the next listelement's key is still less than the one we wish to insert
    while ( (curr->key != NULL) && (strcmp(curr->key, element->key) <= 0) )
    {
        curr = curr->next;
    }

    //now curr points at the element before the element with a key greater than insertelement.
    //insert the specified element between curr and curr->next
    (curr->next)->prev = element;
    element->next = curr->next;
    element->prev = curr;
    curr->next = element;
}


/**
 * SortedList_delete ... remove an element from a sorted list
 * - the driving code handles freeing of memory here
 *
 *    The specified element will be removed from whatever
 *    list it is currently in.
 *
 *    Before doing the deletion, we check to make sure that
 *    next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete( SortedListElement_t *element)
{
    if (opt_yield & DELETE_YIELD)
    {
        sched_yield();
    }
        
    //check if previous element's next points to this
    //check if next element's prev points to this
    if ( ((element->prev)->next == element) && ((element->next)->prev == element) )
    {
        //if valid prev/next pointers, set the prev and next elements to link to each other
        (element->prev)->next = element->next;
        (element->next)->prev = element->prev;
        //then free
        //free(element);
        return 0;
    }
    return 1;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *    The specified list will be searched for an
 *    element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    if (opt_yield & LOOKUP_YIELD)
    {
           sched_yield();
    }
        
    SortedListElement_t *curr = list->next; //start with first node in list
    
   
    //iterate through list looking for key, stop if return back to head
    while ( curr->key != NULL )
    {
        if (strcmp(curr->key, key) == 0)
        {
            return curr;
        }
        curr = curr->next;
    }
    
    //if none found
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *    While enumerating list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *       -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
    int length=0;
    if (opt_yield & LOOKUP_YIELD)
    {
        sched_yield();
    }
       
    SortedListElement_t *curr = list->next; //start with first node after head
    
   
    //iterate through list until reach head
    while (curr->key != NULL)
    {
        if ( (curr->prev)->next != curr || (curr->next)->prev != curr)
        {
            //if corrupted
            return -1;
        }
        curr = curr->next;
        length++;
    }
    return length;
}

