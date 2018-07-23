#include <alloc.h>
#include <logging.h>
#include <mutex.h>
#include <stack.h>

#define STACK_BOTTOM 0
#define EXPAND_FACTOR 2

struct Stack_ {
    pthread_mutex_t *lock;
    void **data;
    int top;
    int capacity;
    void (*ItemDestroy) (void *item);
};

static void DestroyRange(Stack *stack, int start, int end);
static void ExpandIfNecessary(Stack *stack);

Stack *StackNew(size_t initial_capacity, void (ItemDestroy) (void *item))
{
    Stack *stack = xmalloc(sizeof(Stack));

    if (initial_capacity <= 0)
    {
        initial_capacity = 1;
    }

    pthread_mutex_init(stack->lock, NULL);
    if (stack->lock == NULL)
    {
        Log(LOG_ERR,
            "Failed to initialise mutex lock for thread-safe stack: "
            "(pthread_mutex_init: %s)",
            GetErrorStrFromCode(errno));
        free(stack);
        return NULL;
    }

    stack->capacity = initial_capacity;
    stack->top = 0;
    stack->data = xcalloc(sizeof(void *), initial_capacity);
    stack->ItemDestroy = ItemDestroy;

    return stack;
}

void StackDestroy(Stack *stack)
{
    if (stack != NULL)
    {
        DestroyRange(stack, STACK_BOTTOM, stack->top + 1);
    }

    StackSoftDestroy(stack);
}

void StackSoftDestroy(Stack *stack)
{
    if (stack != NULL)
    {
        if (stack->lock != NULL)
        {
            pthread_mutex_destroy(stack->lock);
            stack->lock = NULL;
        }

        free(stack->data);
        free(stack);
    }
}

void *StackPop(Stack *stack)
{
    assert(stack);

    ThreadLock(stack->lock);
    void *data = stack->data[stack->top];
    stack->data[stack->top] = NULL;

    if (stack->top > STACK_BOTTOM)
    {
        stack->top--;
    }

    ThreadUnlock(stack->lock);

    return data;
}

void StackPush(Stack *stack, void *data)
{
    assert(stack);

    ThreadLock(stack->lock);
    ExpandIfNecessary(stack);

    stack->data[++stack->top] = data;

    ThreadUnlock(stack->lock);
}

int StackSize(Stack const *stack)
{
    assert(stack);
    return stack->top + 1;
}

static void DestroyRange(Stack *stack, int start, int end)
{
    if (stack->ItemDestroy)
    {
        for (int i = start; i < end; i++)
        {
            stack->ItemDestroy(stack->data[i]);
        }
    }
}

static void ExpandIfNecessary(Stack *stack)
{
    assert(stack->top < stack->capacity);

    if (stack->top + 1 == stack->capacity)
    {
        stack->capacity *= EXPAND_FACTOR;
        stack->data = xrealloc(stack->data, sizeof(void *) * stack->capacity);
    }
}
