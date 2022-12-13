#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#define RED printf("\033[0;31m");
#define GREEN printf("\033[0;32m");
#define YELLOW printf("\033[0;33m");
#define BLUE printf("\033[0;34m");
#define MAGENTA printf("\033[0;35m");
#define CYAN printf("\033[0;36m");
#define WHITE printf("\033[0;37m");
#define RESET printf("\033[0m");
#define BOLD printf("\033[1m");

int start_time;

typedef struct pizza_details
{
    int pizza_id;
    int prep_time;
    int num_special_ingredients;
    int *special_ingredients_list;
} pizza_details;

typedef struct ingredient_details
{
    int ingredient_id;
    int ingredient_amount;
} ingredient_details;

typedef struct chef_details
{
    int chef_num;
    int entry_time;
    int exit_time;
} chef_details;

typedef struct customer_order_details
{
    int customer_number;
    int entry_time;
    int num_pizzas_tbo;
    int *pizzas_tbo_list;
} customer_order_details;

sem_t chef_sem, oven_sem;
int n, m, i, c, o, k;
pizza_details * pizza_list;
ingredient_details * ingredient_list;
chef_details * chef_list;
customer_order_details * customer_order_list;
chef_details curr_chef;

int compare1(const void * p1, const void * p2) {
    return (*(int*)p1 - *(int*)p2);
}

int compare2(const void * a, const void * b) {
    customer_order_details *detailsA = (customer_order_details *)a;
    customer_order_details *detailsB = (customer_order_details *)b;
    if (detailsA->entry_time != detailsB->entry_time) {
        return (detailsA->entry_time - detailsB->entry_time);
    }
    else if (detailsA->entry_time == detailsB->entry_time && detailsA->customer_number != detailsB->customer_number) {
        return (detailsA->customer_number - detailsB->customer_number);
    }
}

void *thread(void* args) {
    int arrival_time = customer_order_list[*(int*) args].entry_time;
    int c_num = customer_order_list[*(int*) args].customer_number;
    sleep(arrival_time);
    int ct = time(NULL);
    WHITE
    printf("%d: Customer %d arrives at time %d.\n", ct - start_time, c_num, arrival_time);
    RESET
    YELLOW
    printf("%d: Order %d placed by customer %d has pizzas { ", ct - start_time, c_num, c_num);
    int order_size = customer_order_list[*(int*) args].num_pizzas_tbo;
    for (int i = 0; i < order_size; i++) {
        printf("%d ", customer_order_list[*(int*) args].pizzas_tbo_list[i]);
    }
    printf("}.\n");
    RESET
    BLUE
    for (int i = 0; i < order_size; i++) {
        printf("Order for pizza %d placed by customer %d awaits processing.\n", customer_order_list[*(int*) args].pizzas_tbo_list[i], c_num);
    }
    RESET
    for (int i = 0; i < order_size; i++) {
        sem_wait(&chef_sem);
        int p_id = customer_order_list[*(int*) args].pizzas_tbo_list[i];
        int pizza_index_in_list = -1;
        for (int j = 0; j < m; j++) {
            if(pizza_list[j].pizza_id == p_id) {
                pizza_index_in_list = j;
                break;
            }
        }
        ct = time(NULL);
        if (curr_chef.exit_time >= (ct - start_time) + pizza_list[pizza_index_in_list].prep_time) {
            int flag = 0;
            for (int j = 0; j < pizza_list[pizza_index_in_list].num_special_ingredients; j++) {
                for (int k = 0; k < sizeof(ingredient_list) / sizeof(ingredient_list[0]); k++) {
                    if(ingredient_list[k].ingredient_id == pizza_list[pizza_index_in_list].special_ingredients_list[j]) {
                        ingredient_list[k].ingredient_amount -= 1;
                        if (ingredient_list[k].ingredient_amount < 0) {
                            flag = 1;
                            ingredient_list[k].ingredient_amount += 1;
                            break;
                        }
                    }
                }
                if (flag == 1) {
                    break;
                }
            }
            if (flag == 1) {
                RED
                printf("Customer %d rejected.\n", c_num);
                RESET
                sem_post(&chef_sem);
                continue;
            }
            CYAN
            printf("%d: Chef assigned for order %d pizza %d with prep time %d.\n", ct - start_time, c_num, customer_order_list[*(int*) args].pizzas_tbo_list[i], pizza_list[pizza_index_in_list].prep_time);
            printf("Order for pizza %d placed by customer %d is being processed.\n", customer_order_list[*(int*) args].pizzas_tbo_list[i], c_num);
            RESET
            sleep(3);
            sem_wait(&oven_sem);
            ct = time(NULL);
            YELLOW
            printf("Chef has put pizza %d for order %d in the oven at time %d.\n", customer_order_list[*(int*) args].pizzas_tbo_list[i], c_num, ct - start_time);
            sleep(pizza_list[pizza_index_in_list].prep_time - 3);
            sem_post(&oven_sem);
            ct = time(NULL);
            printf("%d: Chef has prepared order %d for customer %d pizza %d and it was picked up from the oven at time %d.\n", ct - start_time, c_num, c_num, customer_order_list[*(int*) args].pizzas_tbo_list[i], ct - start_time);
            printf("Customer %d picks up their pizza %d.\n", c_num, customer_order_list[*(int*) args].pizzas_tbo_list[i]);
            RESET
            sem_post(&chef_sem);
        }
        else {
            RED
            printf("Customer %d rejected.\n", c_num);
            RESET
            sem_post(&chef_sem);
            continue;
        }
    }
    GREEN
    printf("Customer %d exits the drive-thru zone.\n", c_num);
    RESET
}

void *thread2(void* args) {
    int arrival_time = chef_list[*(int*) args].entry_time;
    int exit_time = chef_list[*(int*) args].exit_time;
    int c_num = chef_list[*(int*) args].chef_num;
    sleep(arrival_time);
    int ct = time(NULL);
    WHITE
    printf("%d: Chef %d arrives at time %d.\n", ct - start_time, c_num, arrival_time);
    RESET
    sem_post(&chef_sem);
    curr_chef.entry_time = chef_list[*(int*) args].entry_time;
    curr_chef.exit_time = chef_list[*(int*) args].exit_time;
    curr_chef.chef_num = chef_list[*(int*) args].chef_num;
    sleep(exit_time - arrival_time);
    ct = time(NULL);
    GREEN
    printf("%d: Chef %d exits at time %d.\n", ct - start_time, c_num, exit_time);
    RESET
    sem_wait(&chef_sem);
}

int main()
{
    scanf("%d %d %d %d %d %d", &n, &m, &i, &c, &o, &k);
    pizza_list = malloc(sizeof * pizza_list * m);
    int all_special_ingredients[100];
    int k = 0;
    for (int i = 0; i < m; i++) {
        scanf("%d", &pizza_list[i].pizza_id);
        scanf("%d", &pizza_list[i].prep_time);
        scanf("%d", &pizza_list[i].num_special_ingredients);
        pizza_list[i].special_ingredients_list = malloc(sizeof * pizza_list[i].special_ingredients_list * pizza_list[i].num_special_ingredients);
        for (int j = 0; j < pizza_list[i].num_special_ingredients; j++) {
            scanf("%d", &pizza_list[i].special_ingredients_list[j]);
            all_special_ingredients[k] = pizza_list[i].special_ingredients_list[j];
            k++;
        }
    }
    qsort(all_special_ingredients, k, sizeof(int), compare1);
    int iter_till = all_special_ingredients[k - 1];
    ingredient_list = malloc(sizeof * ingredient_list * iter_till);
    for (int i = 0; i < iter_till; i++) {
        ingredient_list[i].ingredient_id = i + 1;
        scanf("%d", &ingredient_list[i].ingredient_amount);
    }

    chef_list = malloc(sizeof * chef_list * n);
    for (int i = 0; i < n; i++) {
        chef_list[i].chef_num = i + 1;
        scanf("%d", &chef_list[i].entry_time);
        scanf("%d", &chef_list[i].exit_time);
    }

    customer_order_list = malloc(sizeof * customer_order_list * c);
    for (int i = 0; i < c; i++) {
        customer_order_list[i].customer_number = i + 1;
        scanf("%d", &customer_order_list[i].entry_time);
        scanf("%d", &customer_order_list[i].num_pizzas_tbo);
        customer_order_list[i].pizzas_tbo_list = malloc(sizeof * customer_order_list[i].pizzas_tbo_list * customer_order_list[i].num_pizzas_tbo);
        for (int j = 0; j < customer_order_list[i].num_pizzas_tbo; j++) {
            scanf("%d", &customer_order_list[i].pizzas_tbo_list[j]);
        }
    }
    qsort(customer_order_list, c, sizeof(customer_order_details), compare2);
    start_time = time(NULL);
    BOLD
    printf("Simulation Started.\n");
    RESET
    sem_init(&chef_sem, 0, 0);
    sem_init(&oven_sem, 0, o);
    pthread_t th[c], th2[n];
    for (int i = 0; i < n; i++) {
        int *a = malloc(sizeof(int));
        *a = i;
        if(pthread_create(&th2[i], NULL, &thread2, a) != 0) {
            perror("Failed to create thread\n");
        }
    }
    for (int i = 0; i < c; i++) {
        int *a = malloc(sizeof(int));
        *a = i;
        if(pthread_create(&th[i], NULL, &thread, a) != 0) {
            perror("Failed to create thread\n");
        }
    }
    for (int i = 0; i < n; i++) {
        if(pthread_join(th2[i], NULL) != 0) {
            perror("Failed to join thread\n");
        }
    }
    for (int i = 0; i < c; i++) {
        if(pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread\n");
        }
    }
    sem_destroy(&oven_sem);
    sem_destroy(&chef_sem);
    BOLD
    printf("Simulation Ended.\n");
    RESET
}


/*

3 3 4 3 5 3
1 20 3 1 2 3
2 30 2 2 3
3 30 1 4
10 5 3 0
0 60 20 60 30 120
1 1 1
2 2 1 2
4 1 3

*/