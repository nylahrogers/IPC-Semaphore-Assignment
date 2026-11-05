
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "BENSCHILLIBOWL.h"

/* Restaurant settings */
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS (NUM_CUSTOMERS * ORDERS_PER_CUSTOMER)

/* Global restaurant */
BENSCHILLIBOWL *bcb;


/*
 * Customer thread:
 * - Creates orders
 * - Selects random menu items
 * - Adds orders to the restaurant
 */
void* BENSCHILLIBOWLCustomer(void* tid) {

    int customer_id = (int)(long)tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {

        /* Allocate memory for an order */
        Order* order = malloc(sizeof(Order));

        if (order == NULL) {
            perror("Unable to allocate order");
            exit(EXIT_FAILURE);
        }

        /* Select a menu item */
        order->menu_item = PickRandomMenuItem();

        /* Assign customer ID */
        order->customer_id = customer_id;

        /* Initialize order number and next pointer */
        order->order_number = -1;
        order->next = NULL;

        /* Add order to the restaurant */
        AddOrder(bcb, order);
    }

    return NULL;
}


/*
 * Cook thread:
 * - Retrieves orders
 * - Fulfills orders
 * - Frees order memory
 * - Continues until no orders remain
 */
void* BENSCHILLIBOWLCook(void* tid) {

    int cook_id = (int)(long)tid;

    int orders_fulfilled = 0;

    while (1) {

        /* Get the next available order */
        Order* order = GetOrder(bcb);

        /* No more orders */
        if (order == NULL) {
            break;
        }

        /* Fulfill the order */
        orders_fulfilled++;

        /* Record completed order safely */
        pthread_mutex_lock(&bcb->mutex);

        bcb->orders_handled++;

        pthread_mutex_unlock(&bcb->mutex);

        /* Release order memory */
        free(order);
    }

    printf("Cook #%d fulfilled %d orders\n",
           cook_id, orders_fulfilled);

    return NULL;
}


/*
 * Main program:
 * - Opens restaurant
 * - Creates customers and cooks
 * - Joins all threads
 * - Closes restaurant
 */
int main() {

    srand(time(NULL));

    /* Open restaurant */
    bcb = OpenRestaurant(
        BENSCHILLIBOWL_SIZE,
        EXPECTED_NUM_ORDERS
    );

    /* Thread arrays */
    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    /* Create cook threads */
    for (long i = 0; i < NUM_COOKS; i++) {

        if (pthread_create(
                &cooks[i],
                NULL,
                BENSCHILLIBOWLCook,
                (void*)i
            ) != 0) {

            perror("Unable to create cook thread");
            exit(EXIT_FAILURE);
        }
    }

    /* Create customer threads */
    for (long i = 0; i < NUM_CUSTOMERS; i++) {

        if (pthread_create(
                &customers[i],
                NULL,
                BENSCHILLIBOWLCustomer,
                (void*)i
            ) != 0) {

            perror("Unable to create customer thread");
            exit(EXIT_FAILURE);
        }
    }

    /* Wait for all customers to finish */
    for (int i = 0; i < NUM_CUSTOMERS; i++) {

        pthread_join(customers[i], NULL);
    }

    /* Wait for all cooks to finish */
    for (int i = 0; i < NUM_COOKS; i++) {

        pthread_join(cooks[i], NULL);
    }

    /* Close restaurant */
    CloseRestaurant(bcb);

    return 0;
}
