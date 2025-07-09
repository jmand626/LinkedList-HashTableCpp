/*
 * Copyright ©2024 Hannah C. Tang.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Autumn Quarter 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include "LinkedList.h"

#include <stdio.h>
#include <stdlib.h>

#include "CSE333.h"
#include "LinkedList_priv.h"

///////////////////////////////////////////////////////////////////////////////
// LinkedList implementation.

LinkedList *LinkedList_Allocate(void) {
  // Allocate the linked list record.
  LinkedList *ll = (LinkedList *)malloc(sizeof(LinkedList));
  Verify333(ll != NULL);

  // STEP 1: initialize the newly allocated record structure.
  ll->num_elements = 0;
  ll->head = NULL;
  ll->tail = NULL;

  // Return our newly minted linked list.
  return ll;
}

void LinkedList_Free(LinkedList *list,
                     LLPayloadFreeFnPtr payload_free_function) {
  Verify333(list != NULL);
  Verify333(payload_free_function != NULL);

  // STEP 2: sweep through the list and free all of the nodes' payloads
  // (using the payload_free_function supplied as an argument) and
  // the nodes themselves.

  LinkedListNode *temp = NULL;
  while (list->head != NULL) {
    payload_free_function(list->head->payload);
    // format -> functname(parameters)

    temp = list->head;
    // if we would have freed list now we couldnt have moved forward
    list->head = list->head->next;
    free(temp);
    // Free the node itself with temp here. We can use free because
    // a singular listnode is also a list
  }

  // free the LinkedList
  free(list);
}

int LinkedList_NumElements(LinkedList *list) {
  Verify333(list != NULL);
  return list->num_elements;
}

void LinkedList_Push(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *)malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // STEP 3: typical case; list has >=1 elements
    list->num_elements++;
    list->head->prev = ln;
    // since we are making a new head element
    ln->prev = NULL;
    ln->next = list->head;
    // since the head has no prev and the next element would be the
    // old head value
    list->head = ln;
  }
}

bool LinkedList_Pop(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 4: implement LinkedList_Pop.  Make sure you test for
  // and empty list and fail.  If the list is non-empty, there
  // are two cases to consider: (a) a list with a single element in it
  // and (b) the general case of a list with >=2 elements in it.
  // Be sure to call free() to deallocate the memory that was
  // previously allocated by LinkedList_Push().
  if (list->num_elements == 0) {
    return false;
  }

  LinkedListNode *temp = list->head;
  // what we actually free later, do not want to free soemthing set to NULL

  // Crucially, does not free the entire list structure itself!
  if (list->num_elements >= 2) {
    *payload_ptr = list->head->payload;
    // guess who forgot to dereference payload_ptr :(
    list->head = list->head->next;
    list->head->prev = NULL;
    // do not have to set current temp's fields to null, free deletes it
    list->num_elements--;
  } else {  // list has a single element
    // already earlier had a section checking if num elements is 0
    // and num of elements definitely cannot be negative
    list->num_elements = 0;
    *payload_ptr = list->head->payload;
    list->head = NULL;
    list->tail = NULL;
  }
  // free head in both cases, could also do it here
  free(temp);

  return true;  // you may need to change this return value
}

void LinkedList_Append(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // STEP 5: implement LinkedList_Append.  It's kind of like
  // LinkedList_Push, but obviously you need to add to the end
  // instead of the beginning.

  // All of the following code is same as in push besides the else statement
  // There, the logic flips to add to the end of the list instead of begining

  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *)malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {  // this piece moved over from push method
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    // In a list with one element, the element is both the head and tail
    list->num_elements = 1;
  } else {  // Opposite of corresponding push code below
    // typical case; list has >=1 elements
    list->num_elements++;
    list->tail->next = ln;
    // since the tail is still the old last value, we attach our value
    ln->next = NULL;
    ln->prev = list->tail;
    list->tail = ln;
  }
}

void LinkedList_Sort(LinkedList *list, bool ascending,
                     LLPayloadComparatorFnPtr comparator_function) {
  Verify333(list != NULL);
  if (list->num_elements < 2) {
    // No sorting needed.
    return;
  }

  // We'll implement bubblesort! Nnice and easy, and nice and slow :)
  int swapped;
  do {
    LinkedListNode *curnode;

    swapped = 0;
    curnode = list->head;
    while (curnode->next != NULL) {
      int compare_result =
          comparator_function(curnode->payload, curnode->next->payload);
      if (ascending) {
        compare_result *= -1;
      }
      if (compare_result < 0) {
        // Bubble-swap the payloads.
        LLPayload_t tmp;
        tmp = curnode->payload;
        curnode->payload = curnode->next->payload;
        curnode->next->payload = tmp;
        swapped = 1;
      }
      curnode = curnode->next;
    }
  } while (swapped);
}

///////////////////////////////////////////////////////////////////////////////
// LLIterator implementation.

LLIterator *LLIterator_Allocate(LinkedList *list) {
  Verify333(list != NULL);

  // OK, let's manufacture an iterator.
  LLIterator *li = (LLIterator *)malloc(sizeof(LLIterator));
  Verify333(li != NULL);

  // Set up the iterator.
  li->list = list;
  li->node = list->head;

  return li;
}

void LLIterator_Free(LLIterator *iter) {
  Verify333(iter != NULL);
  free(iter);
}

bool LLIterator_IsValid(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);

  return (iter->node != NULL);
}

bool LLIterator_Next(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 6: try to advance iterator to the next node and return true if
  // you succeed, false otherwise
  // Note that if the iterator is already at the last node,
  // you should move the iterator past the end of the list

  if (iter->node->next == NULL) {
    // Moved past the end of the list
    iter->node = NULL;
    return false;
  }

  iter->node = iter->node->next;

  return true;  // you may need to change this return value
}

void LLIterator_Get(LLIterator *iter, LLPayload_t *payload) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  *payload = iter->node->payload;
}

bool LLIterator_Remove(LLIterator *iter,
                       LLPayloadFreeFnPtr payload_free_function) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 7: implement LLIterator_Remove.  This is the most
  // complex function you'll build.  There are several cases
  // to consider:
  // - degenerate case: the list becomes empty after deleting.
  // - degenerate case: iter points at head
  // - degenerate case: iter points at tail
  // - fully general case: iter points in the middle of a list,
  //                       and you have to "splice".
  //
  // Be sure to call the payload_free_function to free the payload
  // the iterator is pointing to, and also free any LinkedList
  // data structure element as appropriate.

  // since we always want to free the payload we do it here
  payload_free_function(iter->node->payload);

  LinkedListNode *temp = iter->node;
  // need this as we free this node after all conditions to avoid repeating

  if (iter->list->num_elements == 1) {
    iter->node = NULL;
    iter->list->head = NULL;
    iter->list->tail = NULL;
    // do not need to set prev/next to null as we already
    // set null to null -> Source of my segfault :(

    iter->list->num_elements = 0;

    free(temp);
    return false;
  } else if (iter->node == iter->list->head) {
    iter->node = iter->node->next;

    // After moving past the original head, we set head to the current node
    iter->list->head = iter->node;
    iter->node->prev = NULL;

  } else if (iter->node == iter->list->tail) {
    iter->node = iter->node->prev;

    // After moving back away from the orignal tail, we set tail to the curremt
    // node
    iter->list->tail = iter->node;
    iter->node->next = NULL;

  } else {  // splicing case
    // Set the previous nodes next to the next node, jumping over curr
    iter->node->prev->next = iter->node->next;
    iter->node->next->prev = iter->node->prev;
    // Set the next nodes prev to the previous node, jumping over curr

    // Should do this last to easily access surrounding nodes
    iter->node = iter->node->next;
  }

  // Wil always happen in these three last cases
  iter->list->num_elements--;

  // Must free temp pointer as other values have been deleted
  free(temp);

  return true;  // you may need to change this return value
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions

bool LLSlice(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 8: implement LLSlice.
  if (list->num_elements == 0) {
    return false;
  }

  LinkedListNode *temp = list->tail;
  // what is actually freed later, do not want to free value set to NULL

  *payload_ptr = list->tail->payload;

  // As append was similar to push, slice is similar to pop
  if (list->num_elements >= 2) {
    list->tail = list->tail->prev;
    list->tail->next = NULL;
    // do not have to set current node's fields to null, free deletes it
    list->num_elements--;

  } else {  // list has a single element
    // already earlier had a section checking if num elements is 0
    // and num of elements definitely cannot be negative

    list->num_elements = 0;
    list->head = NULL;
    list->tail = NULL;

    // Could use head as head and tail are the same when there is one element
  }
  // free head in both cases, could also do it here
  free(temp);

  return true;  // you may need to change this return value
}

void LLIteratorRewind(LLIterator *iter) { iter->node = iter->list->head; }
