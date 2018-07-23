#ifndef CFENGINE_STACK_H
#define CFENGINE_STACK_H

#include <platform.h>

typedef struct Stack_ Stack;

/**
  @brief Creates a new stack with specified capacity.
  @param [in] initial_capacity Initial capacity, defaults to 1.
  @param [in] ItemDestroy Function used to destroy data elements.
  */
Stack *StackNew(size_t initial_capacity, void (*ItemDestroy) ());

/**
  @brief Destroys the stack and frees the memory it occupies.
  @param [in] stack The stack to destroy.
  */
void StackDestroy(Stack *stack);

/**
  @brief Frees the pointer to the data array and the stack pointer itself.
  @param [in] stack The stack to free.
  */
void StackSoftDestroy(Stack *stack);

/**
  @brief Adds a new item on top of the stack.
  @param [in] stack The stack to push to.
  @param [in] item The item to push.
  */
void StackPush(Stack *stack, void *item);

/**
  @brief Returns and removes the last element added to the stack.
  @note Will return NULL if stack is empty.
  @param [in] stack The stack to pop from.
  @return A pointer to the last data added.
  */
void *StackPop(Stack *stack);

/**
  @brief Get current size of stack.
  @note On NULL stack, returns 0.
  @param [in] stack The stack.
  @return The amount of elements in the stack.
  */
int StackSize(Stack const *stack);

#endif
