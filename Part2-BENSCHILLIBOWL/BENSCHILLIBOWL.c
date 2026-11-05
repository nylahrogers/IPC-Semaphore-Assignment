
#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

/* Menu items */
MenuItem BENSCHILLIBOWLMenu[] = {
    "BensChilli",
    "BensHalfSmoke",
    "BensHotDog",
    "BensChilliCheeseFries",
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings"
};

int BENSCHILLIBOWLMenuLength = 10;

/* Helper function declarations */
bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);


/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {

    int index = rand() % BENSCHILLIBOWLMenuLength;

    return BENSCHILLIBOWLMenu[index];
}


/* Allocate and initialize the restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size,
                               int expected_num_orders) {

    BENSCHILLIBOWL* bcb =
        malloc(sizeof(BENSCHILLIBOWL));

    if (bcb == NULL) {
        perror("Unable to allocate restaurant");
        exit(EXIT_FAILURE);
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 0;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    if (pthread_mutex_init(&bcb->mutex, NULL) != 0) {
        free(bcb);
        perror("Unable to initialize mutex");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&bcb->can_add_orders, NULL) != 0) {
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        perror("Unable to initialize condition variable");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&bcb->can_get_orders, NULL) != 0) {
        pthread_cond_destroy(&bcb->can_add_orders);
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        perror("Unable to initialize condition variable");
        exit(EXIT_FAILURE);
    }

    printf("Restaurant is open!\n");

    return bcb;
}


/* Close the restaurant and release resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {

    pthread_mutex_lock(&bcb->mutex);

    assert(bcb->current_size == 0);
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_unlock(&bcb->mutex);

    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);
    pthread_mutex_destroy(&bcb->mutex);

    free(bcb);

    printf("Restaurant is closed!\n");
}


/* Check whether the restaurant is empty */
bool IsEmpty(BENSCHILLIBOWL* bcb) {

    return bcb->current_size == 0;
}


/* Check whether the restaurant is full */
bool IsFull(BENSCHILLIBOWL* bcb) {

    return bcb->current_size >= bcb->max_size;
}


/* Add an order to the back of the queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {

    pthread_mutex_lock(&bcb->mutex);

    /* Wait while the restaurant is full */
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    /* Assign an order number */
    order->order_number = bcb->next_order_number++;

    /* Add order to the queue */
    AddOrderToBack(&bcb->orders, order);

    bcb->current_size++;

    int order_number = order->order_number;

    /* Notify a waiting cook */
    pthread_cond_signal(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order_number;
}


/* Remove an order from the front of the queue */
Order* GetOrder(BENSCHILLIBOWL* bcb) {

    pthread_mutex_lock(&bcb->mutex);

    /*
     * Wait while the queue is empty and
     * customers may still submit orders.
     */
    while (IsEmpty(bcb) &&
           bcb->next_order_number < bcb->expected_num_orders) {

        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    /*
     * No more orders will arrive.
     * Tell other waiting cooks to finish.
     */
    if (IsEmpty(bcb)) {

        pthread_cond_broadcast(&bcb->can_get_orders);

        pthread_mutex_unlock(&bcb->mutex);

        return NULL;
    }

    /* Remove the first order */
    Order* order = bcb->orders;

    bcb->orders = order->next;

    order->next = NULL;

    bcb->current_size--;

    /* Notify customers that space is available */
    pthread_cond_signal(&bcb->can_add_orders);

    /*
     * If all orders have been submitted and the
     * queue is now empty, wake up other cooks.
     */
    if (IsEmpty(bcb) &&
        bcb->next_order_number >= bcb->expected_num_orders) {

        pthread_cond_broadcast(&bcb->can_get_orders);
    }

    pthread_mutex_unlock(&bcb->mutex);

    return order;
}


/* Add an order to the rear of the linked list */
void AddOrderToBack(Order **orders, Order *order) {

    order->next = NULL;

    if (*orders == NULL) {

        *orders = order;

        return;
    }

    Order* current = *orders;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = order;
}
