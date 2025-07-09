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

#include "HashTable.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "CSE333.h"
#include "HashTable_priv.h"
#include "LinkedList.h"

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions.
//
#define INVALID_IDX -1

// Grows the hashtable (ie, increase the number of buckets) if its load
// factor has become too high.
static void MaybeResize(HashTable *ht);

int HashKeyToBucketNum(HashTable *ht, HTKey_t key) {
  return key % ht->num_buckets;
}

// Deallocation functions that do nothing.  Useful if we want to deallocate
// the structure (eg, the linked list) without deallocating its elements or
// if we know that the structure is empty.
static void LLNoOpFree(LLPayload_t freeme) {}
static void HTNoOpFree(HTValue_t freeme) {}

///////////////////////////////////////////////////////////////////////////////
// HashTable implementation.

HTKey_t FNVHash64(unsigned char *buffer, int len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //     http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *)buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  // FNV-1a hash each octet of the buffer.
  while (bp < be) {
    // XOR the bottom with the current octet.
    hval ^= (uint64_t)*bp++;
    // Multiply by the 64 bit FNV magic prime mod 2^64.
    hval *= FNV_64_PRIME;
  }
  return hval;
}

HashTable *HashTable_Allocate(int num_buckets) {
  HashTable *ht;
  int i;

  Verify333(num_buckets > 0);

  // Allocate the hash table record.
  ht = (HashTable *)malloc(sizeof(HashTable));
  Verify333(ht != NULL);

  // Initialize the record.
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets = (LinkedList **)malloc(num_buckets * sizeof(LinkedList *));
  Verify333(ht->buckets != NULL);
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = LinkedList_Allocate();
  }

  return ht;
}

void HashTable_Free(HashTable *table, ValueFreeFnPtr value_free_function) {
  int i;

  Verify333(table != NULL);

  // Free each bucket's chain.
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList *bucket = table->buckets[i];
    HTKeyValue_t *kv;

    // Pop elements off the chain list one at a time.  We can't do a single
    // call to LinkedList_Free since we need to use the passed-in
    // value_free_function -- which takes a HTValue_t, not an LLPayload_t -- to
    // free the caller's memory.
    while (LinkedList_NumElements(bucket) > 0) {
      Verify333(LinkedList_Pop(bucket, (LLPayload_t *)&kv));
      value_free_function(kv->value);
      free(kv);
    }
    // The chain is empty, so we can pass in the
    // null free function to LinkedList_Free.
    LinkedList_Free(bucket, LLNoOpFree);
  }

  // Free the bucket array within the table, then free the table record itself.
  free(table->buckets);
  free(table);
}

int HashTable_NumElements(HashTable *table) {
  Verify333(table != NULL);
  return table->num_elements;
}

// Declaration before definition
// Helper function created mainly for STEP 1 but also the other steps
// Inserts a (key,value) pair into the HashTable.
//
// Arguments:
// - table: the HashTable to insert into.
// - oldpair_ptr: a return parameter; if a previous pair is found,
//   the popped keyvalue is returned through this parameter. This is
//   a double pointer because we want to send addresses too
// - newkey: the new key we are tring to find. Just sending this seperately
//   instead of relying on oldpair_ptr is just unneccesary headache
// - chain: the actual linkedlist chain we want to insert into
//
// Returns:
//  - false: if the newkeyvalue was not found and there was no
//    existing (key,value) with that key.
//  - true: if the newkeyvalue was found and an old value
//    with the same key was also found and returned through
//    the oldpair_ptr return parameter.
bool HashTable_FindKey(HashTable *table, HTKeyValue_t **oldpair_ptr,
                       HTKey_t newkey, LinkedList *chain);

bool HashTable_Insert(HashTable *table, HTKeyValue_t newkeyvalue,
                      HTKeyValue_t *oldkeyvalue) {
  int bucket;
  LinkedList *chain;

  Verify333(table != NULL);
  MaybeResize(table);

  // Calculate which bucket and chain we're inserting into.
  bucket = HashKeyToBucketNum(table, newkeyvalue.key);
  chain = table->buckets[bucket];

  // STEP 1: finish the implementation of InsertHashTable.
  // This is a fairly complex task, so you might decide you want
  // to define/implement a helper function that helps you find
  // and optionally remove a key within a chain, rather than putting
  // all that logic inside here.  You might also find that your helper
  // can be reused in steps 2 and 3.

  HTKeyValue_t *temp;
  // must use temp because we need to both change oldkeyvalue and
  // also return it

  if (HashTable_FindKey(table, &temp, newkeyvalue.key, chain)) {
    // No need to remove/add nodes, just change the payload
    // FindKey just needs key value, we get the address of temp for
    // a double pointer

    *oldkeyvalue = *temp;
    *temp = newkeyvalue;
    // classic temp swap

    return true;
  } else {
    HTKeyValue_t *newpair_ptr = (HTKeyValue_t *)malloc(sizeof(HTKeyValue_t));
    *newpair_ptr = newkeyvalue;
    // we malloced space, but didnt copy that data in yet
    LinkedList_Append(chain, (LLPayload_t)newpair_ptr);
    // Must malloc new struct because it is sent as a copy as parameter
    // Also, llpayload_t is a pointer!!!!!

    table->num_elements++;
    return false;
  }
}

// Declared right above the insert function
bool HashTable_FindKey(HashTable *table, HTKeyValue_t **oldpair_ptr,
                       HTKey_t newkey, LinkedList *chain) {
  LLIterator *iter = LLIterator_Allocate(chain);
  HTKeyValue_t *payload;
  // getting back HTKeyValue_t

  while (LLIterator_IsValid(iter)) {
    LLIterator_Get(iter, (LLPayload_t *)&payload);
    // because get expects a pointer (llpayload is a pointer) and then
    // derefernces that, we send it the casted version of the address
    // of our payload

    if (payload->key == newkey) {
      // since payload was sent into get as a double pointer, we got the
      // address of our new struct, so we can set the dereferenced double
      // pointer to the address of our struct instead
      *oldpair_ptr = payload;
      LLIterator_Free(iter);
      return true;
    }
    LLIterator_Next(iter);
  }
  LLIterator_Free(iter);
  return false;
}

// We wrote a helper function for insert, but it turns out that
// it does practically the same thing as this function here
bool HashTable_Find(HashTable *table, HTKey_t key, HTKeyValue_t *keyvalue) {
  Verify333(table != NULL);

  // STEP 2: implement HashTable_Find.

  // Moved over this code from insert with some slight changes
  int bucket = HashKeyToBucketNum(table, key);
  LinkedList *chain = table->buckets[bucket];

  HTKeyValue_t *temp = NULL;
  bool toReturn = HashTable_FindKey(table, &temp, key, chain);
  // as mentioned before, we send in a double pointer such that
  // when we set another dereferenced double pointer equal to this
  // dereferenced double pointer, we are just sending a address from temp
  // to keyvalue
  if (toReturn) {
    *keyvalue = *temp;  // Safely dereference temp now
  }
  return toReturn;
}

bool HashTable_Remove(HashTable *table, HTKey_t key, HTKeyValue_t *keyvalue) {
  Verify333(table != NULL);

  // STEP 3: implement HashTable_Remove.

  int bucket = HashKeyToBucketNum(table, key);
  LinkedList *chain = table->buckets[bucket];
  HTKeyValue_t *temp;
  // similar reasoning as in find
  if (HashTable_FindKey(table, &temp, key, chain)) {
    LLIterator *iter = LLIterator_Allocate(chain);
    HTKeyValue_t *tempValue;
    // we need ANOTHER temp as the first one was an output parameter
    // do not want to override out value of keyvalue

    while (LLIterator_IsValid(iter)) {
      LLIterator_Get(iter, (LLPayload_t *)&tempValue);
      // through output parameters, will put value in tempValue output
      // double pointer

      if (tempValue->key == key) {
        LLIterator_Remove(iter, LLNoOpFree);
        break;
      }

      LLIterator_Next(iter);
    }
    // Actually copy over from temp structure into temp
    *keyvalue = *temp;
    LLIterator_Free(iter);
    free(tempValue);
    // must be free iterator too because we used it specially to help

    table->num_elements--;
    return true;

  } else {
    return false;
  }
}

///////////////////////////////////////////////////////////////////////////////
// HTIterator implementation.

HTIterator *HTIterator_Allocate(HashTable *table) {
  HTIterator *iter;
  int i;

  Verify333(table != NULL);

  iter = (HTIterator *)malloc(sizeof(HTIterator));
  Verify333(iter != NULL);

  // If the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->ht = table;
    iter->bucket_it = NULL;
    iter->bucket_idx = INVALID_IDX;
    return iter;
  }

  // Initialize the iterator.  There is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (LinkedList_NumElements(table->buckets[i]) > 0) {
      iter->bucket_idx = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLIterator_Allocate(table->buckets[iter->bucket_idx]);
  return iter;
}

void HTIterator_Free(HTIterator *iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it != NULL) {
    LLIterator_Free(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  free(iter);
}

bool HTIterator_IsValid(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 4: implement HTIterator_IsValid.

  if (iter->bucket_it == NULL || iter->bucket_idx == INVALID_IDX ||
      iter->ht->num_elements == 0) {
    return false;
  }
  // Where did we learn about the INVALID_IDX constant? The allocate function!

  if (!LLIterator_IsValid(iter->bucket_it) &&
      iter->bucket_idx == iter->ht->num_buckets) {
    // bucket_it is linked list iterator, or if we are at the last bucket
    return false;
  } else {
    return true;
  }
}

bool HTIterator_Next(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 5: implement HTIterator_Next.

  if (!LLIterator_IsValid(iter->bucket_it)) {
    return false;
  }

  // If it is valid right now, no need for anything complicated
  LLIterator_Next(iter->bucket_it);

  // Will exit when we do find a valid next or the table ends
  if (!LLIterator_IsValid(iter->bucket_it)) {
    iter->bucket_idx += 1;
    // Increment bucket index to check the next bucket.

    // Keep advancing until we find a non-empty bucket or run out of buckets.
    while (iter->bucket_idx < iter->ht->num_buckets &&
           LinkedList_NumElements(iter->ht->buckets[iter->bucket_idx]) == 0) {
      iter->bucket_idx += 1;
    }

    if (iter->bucket_idx == iter->ht->num_buckets) {
      return false;
    }
    // If we find a non-empty bucket, free the old iterator for the old chain
    LLIterator_Free(iter->bucket_it);

    iter->bucket_it = LLIterator_Allocate(iter->ht->buckets[iter->bucket_idx]);

    // Like the malloc null checks
    if (!LLIterator_IsValid(iter->bucket_it)) {
      return false;
    }
  }

  return true;
}

bool HTIterator_Get(HTIterator *iter, HTKeyValue_t *keyvalue) {
  Verify333(iter != NULL);

  // STEP 6: implement HTIterator_Get.

  HTKeyValue_t *payload;
  if (iter->ht->num_elements == 0 || !HTIterator_IsValid(iter)) {
    return false;
  }

  LLIterator_Get(iter->bucket_it, (LLPayload_t *)&payload);
  // As mentioned before:
  // because get expects a pointer (llpayload is a pointer) and then
  // derefernces that, we send it the casted version of the address
  // of our payload variable, so it gets a double variable
  keyvalue->key = payload->key;
  keyvalue->value = payload->value;

  return true;
}

bool HTIterator_Remove(HTIterator *iter, HTKeyValue_t *keyvalue) {
  HTKeyValue_t kv;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  if (!HTIterator_Get(iter, &kv)) {
    return false;
  }

  // Advance the iterator.  Thanks to the above call to
  // HTIterator_Get, we know that this iterator is valid (though it
  // may not be valid after this call to HTIterator_Next).
  HTIterator_Next(iter);

  // Lastly, remove the element.  Again, we know this call will succeed
  // due to the successful HTIterator_Get above.
  Verify333(HashTable_Remove(iter->ht, kv.key, keyvalue));
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return true;
}

static void MaybeResize(HashTable *ht) {
  HashTable *newht;
  HashTable tmp;
  HTIterator *it;

  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets) return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  newht = HashTable_Allocate(ht->num_buckets * 9);

  // Loop through the old ht copying its elements over into the new one.
  for (it = HTIterator_Allocate(ht); HTIterator_IsValid(it);
       HTIterator_Next(it)) {
    HTKeyValue_t item, unused;

    Verify333(HTIterator_Get(it, &item));
    HashTable_Insert(newht, item, &unused);
  }

  // Swap the new table onto the old, then free the old table (tricky!).  We
  // use the "no-op free" because we don't actually want to free the elements;
  // they're owned by the new table.
  tmp = *ht;
  *ht = *newht;
  *newht = tmp;

  // Done!  Clean up our iterator and temporary table.
  HTIterator_Free(it);
  HashTable_Free(newht, &HTNoOpFree);
}
