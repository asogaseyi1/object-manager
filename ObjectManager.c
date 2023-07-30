/**
 * Name of class or program : AsogaOluwaseyiObjMan.c
 *
 * COMP 2160 SECTION A01
 * INSTRUCTOR    Karel Kahula
 * ASSIGNMENT    Assignment 3, question 1
 * AUTHOR        Oluwaseyi Asoga, 007913224
 * DATE          20th June 2023
 *
 * PURPOSE: Object manager/garbage collector 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ObjectManager.h"

#define MAX_OBJECTS 1000000

typedef struct {
    Ref ref;
    void* address;
    ulong size;
    int refCount;
    int inUse;
    int marked;
} Object;

typedef struct Node {
    Object object;
    struct Node* next;
} Node;

static uchar b_1[MEMORY_SIZE];// buffer 1
static uchar b_2[MEMORY_SIZE];// buffer 2
static uchar* B = b_1;  // Start with b_1 as the target buffer
void compact();

static Node* index = NULL;
static int objectCount = 0;
static ulong nextAvailableIndex = 0;

void initPool() {
    objectCount = 0;
    nextAvailableIndex = 0;
    index = NULL;
    B = b_1;  // Start with b_1 as the target buffer
}


Ref insertObject(ulong size) {
    assert(size > 0);
    // Create a new object node
    Node* newNode = (Node*)malloc(sizeof(Node));
    assert(newNode != NULL);
    newNode->object.ref = objectCount + 1;
    newNode->object.size = size;
    newNode->object.refCount = 1;
    newNode->object.inUse = 1;
    newNode->object.address = NULL;  // Placeholder value

    if (size >= MEMORY_SIZE) {
        // Object size exceeds buffer size, cannot insert
        free(newNode);
        return NULL_REF;
    }

    // Check if there is enough space in the current buffer
    if (nextAvailableIndex + size > MEMORY_SIZE) {
        // Run out of memory, switch buffers and compact
        compact();

        // Check if there is enough space after compaction
        if (nextAvailableIndex + size > MEMORY_SIZE) {
            // Object size exceeds available space even after compaction
            free(newNode);
            return NULL_REF;
        }
    }

    // Update the address of the new node
    newNode->object.address = B + nextAvailableIndex;

    // Add the new node to the index
    newNode->next = NULL;
    if (index == NULL) {
        index = newNode;
    } else {
        Node* current = index;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    objectCount++;
    nextAvailableIndex += size;

    return newNode->object.ref;
}


void* retrieveObject(Ref ref) {
    Node* current = index;
    while (current != NULL) {
        if (current->object.ref == ref) {
            return current->object.address;
        }
        current = current->next;
    }
    return NULL;
}

void addReference(Ref ref) {
    assert(ref != NULL_REF);

    Node* current = index;
    while (current != NULL) {
        if (current->object.ref == ref) {
            current->object.refCount++;
            current->object.inUse = 1;  // Set inUse to 1 when adding a reference
            break;
        }
        current = current->next;
    }
}



void dropReference(Ref ref) { 
    assert(ref != NULL_REF);

    Node* current = index;
    while (current != NULL) {
        if (current->object.ref == ref) {
            current->object.refCount--;
            if (current->object.refCount <= 0) {
                current->object.inUse = 0;
                compact();
                objectCount--;
            }
            break;
        }
        current = current->next;
    }
}

void compact() {
    printf("Running garbage collector...\n");

    int numObjects = 0;
    int bytesInUse = 0;
    int bytesCollected = 0;

    // Mark phase
    Node* current = index;
    while (current != NULL) {
        if (current->object.inUse) {
            current->object.marked = 1;
            numObjects++;
            bytesInUse += current->object.size;
        }
        current = current->next;
    }

    // Switch buffers
    if (B == b_1) {
        B = b_2;
    } else {
        B = b_1;
    }

    // Sweep and compact phase
    ulong shift = 0;
    current = index;
    while (current != NULL) {
        if (!current->object.marked) {
            if (current->object.inUse) {
                bytesCollected += current->object.size;
                current->object.inUse = 0;
            }
        } else {
            current->object.marked = 0;
            current->object.address = (void*)(B + shift);
        }
        shift += current->object.size;
        current = current->next;
    }

    nextAvailableIndex = shift;

    printf("Number of objects: %d\n", numObjects);
    printf("Current bytes in use: %d\n", bytesInUse);
    printf("Bytes collected: %d\n", bytesCollected);
}




void dumpPool() {
    printf("---\n");
    printf("Object dump:\n");
    Node* current = index;
    ulong total = 0;
    while (current != NULL) {
        total += current->object.size;
        if (current->object.inUse) {
            printf("refid = %lu, start addr = %lu, num bytes = %lu, reference count = %d\n",
                   current->object.ref, (total-current->object.size), current->object.size, current->object.refCount);
        }
        current = current->next;
    }
    printf("next available index = %lu\n", nextAvailableIndex);
    printf("%s\n", "End of Processing");
}

void destroyPool() {
    Node* current = index;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    index = NULL;
    objectCount = 0;
}



