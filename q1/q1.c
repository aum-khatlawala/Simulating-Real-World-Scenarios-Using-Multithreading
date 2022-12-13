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

typedef struct student_details
{
    int student_num;
    int T_i;
    int W_i;
    int P_i;
} student_details;

sem_t washing_machine_sem;
int N, M;
student_details *list;

int left_wo_washing = 0;
time_t time_wasted = 0;

int compare1(const void *a, const void *b)
{
    student_details *detailsA = (student_details *)a;
    student_details *detailsB = (student_details *)b;
    if (detailsA->T_i != detailsB->T_i)
    {
        return (detailsA->T_i - detailsB->T_i);
    }
    else if (detailsA->T_i == detailsB->T_i && detailsA->student_num != detailsB->student_num)
    {
        return (detailsA->student_num - detailsB->student_num);
    }
}

void *thread(void* args) {
    int arrival_time = list[*(int*) args].T_i;
    int student_num = list[*(int*) args].student_num;
    int patience_time = list[*(int*) args].P_i;
    sleep(arrival_time);
    WHITE
    printf("%d: Student %d arrives\n", arrival_time, student_num);
    RESET
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    time_t st = time(NULL);
    start.tv_sec += patience_time + 1;
    int rc = sem_timedwait(&washing_machine_sem, &start);
    if (errno == ETIMEDOUT) {
        time_t ct = time(NULL);
        time_wasted += (ct - st) - 1;
        RED
        printf("%d: Student %d leaves without washing\n", arrival_time + patience_time, student_num);
        RESET
        left_wo_washing++;
    }
    else {
        time_t ct = time(NULL);
        time_wasted += (ct - st);
        GREEN
        printf("%ld: Student %d starts washing\n", (ct - st) + arrival_time, student_num);
        RESET
        int washing_time = list[*(int*) args].W_i;
        sleep(washing_time);
        YELLOW
        printf("%ld: Student %d leaves after washing\n", (ct - st) + washing_time + arrival_time, student_num);
        RESET
        sem_post(&washing_machine_sem);
    }
}

int main()
{
    scanf("%d %d", &N, &M);
    list = malloc(sizeof * list * N);
    for (int i = 0; i < N; i++)
    {
        list[i].student_num = i + 1;
        scanf("%d", &list[i].T_i);
        scanf("%d", &list[i].W_i);
        scanf("%d", &list[i].P_i);
    }
    qsort(list, N, sizeof(student_details), compare1);
    pthread_t th[N];
    sem_init(&washing_machine_sem, 0, M);
    int i;
    for (i = 0; i < N; i++) {
        int *a = malloc(sizeof(int));
        *a = i;
        if(pthread_create(&th[i], NULL, &thread, a) != 0) {
            perror("Failed to create thread\n");
        }
    }
    for (i = 0; i < N; i++) {
        if(pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread\n");
        }
    }
    sem_destroy(&washing_machine_sem);
    float frac_left_wo_washing = (float)left_wo_washing / (float)N;
    int more_needed_bool;
    if (frac_left_wo_washing >= 0.25) {
        printf("%d\n%ld\nYes\n", left_wo_washing, time_wasted);
    }
    else {
        printf("%d\n%ld\nNo\n", left_wo_washing, time_wasted);
    }
    return 0;
}

/*

5 2
6 3 5
3 4 3
6 5 2
2 9 6
8 5 2

3 1
2 5 1
1 2 4
2 4 2

*/
