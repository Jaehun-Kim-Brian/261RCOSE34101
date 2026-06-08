// 헤더 load
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
// 매크로 정의 
#define MAX_PROCESSES 10
#define MAX_GANTT 1000 // Gantt 차트에서 저장할 최대 block 수
#define TIME_QUANTUM 3
// 각 프로세스 구조체
typedef struct {
    int pid;
    int arrival_time;
    int cpu_burst;
    int remaining_time;
    int priority;

    int completion_time;
    int turnaround_time;
    int waiting_time;
    int is_completed; // 완료 여부
} Process;
// Gantt 차트의 한 칸 표현을 위한 구조체
typedef struct {
    int pid;
    int start_time;
    int end_time;
} GanttBlock;
// 각 알고리즘 실행 결과 저장 구조체
typedef struct {
    double avg_waiting_time;
    double avg_turnaround_time;
} Result;

// global 변수 설정
Process original[MAX_PROCESSES]; // MAX_PROCESSES 개의 프로세스 미리 설정
int n;

GanttBlock gantt[MAX_GANTT]; // MAX_GANTT 개의 Gantt block 미리 설정
int gantt_count = 0;

/* ---------------- Utility Functions ---------------- */

// 새 알고리즘 전 Gantt 차트 초기화
void clear_gantt() {
    gantt_count = 0;
}
// Gantt block 추가
void add_gantt_block(int pid, int start_time, int end_time) {
    if (start_time == end_time) return;
    // 추가해야할 pid와 직전 pid가 같다면, 새 block을 추가하지 않음
    if (gantt_count > 0 && gantt[gantt_count - 1].pid == pid) {
        gantt[gantt_count - 1].end_time = end_time;
    } else {
        gantt[gantt_count].pid = pid;
        gantt[gantt_count].start_time = start_time;
        gantt[gantt_count].end_time = end_time;
        gantt_count++;
    }
}
// 최종 Gantt 차트 출력
void print_gantt_chart() {
    printf("\nGantt Chart:\n");

    for (int i = 0; i < gantt_count; i++) {
        if (gantt[i].pid == -1) {
            printf("| IDLE\t");
        } else {
            printf("| P%d\t", gantt[i].pid);
        }
    }
    printf("|\n");

    for (int i = 0; i < gantt_count; i++) {
        printf("%d\t", gantt[i].start_time);
    }

    if (gantt_count > 0) {
        printf("%d", gantt[gantt_count - 1].end_time);
    }
    printf("\n");
}
// 스케줄링 알고리즘이 수정할 수 있는 사본을 생성 dest로 src를 복사
void copy_processes(Process dest[], Process src[]) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
        dest[i].remaining_time = src[i].cpu_burst;
        dest[i].completion_time = 0;
        dest[i].turnaround_time = 0;
        dest[i].waiting_time = 0;
        dest[i].is_completed = 0;
    }
}
// process를 랜덤 값들로 생성
void create_processes() {
    srand(time(NULL));

    printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
    scanf("%d", &n);

    if (n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes.\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        original[i].pid = i + 1;
        original[i].arrival_time = rand() % 10;       // 0 ~ 9
        original[i].cpu_burst = (rand() % 9) + 1;     // 1 ~ 9
        original[i].remaining_time = original[i].cpu_burst;
        original[i].priority = (rand() % 5) + 1;      // 1 ~ 5, 작을 수록 높은 우선순위

        original[i].completion_time = 0;
        original[i].turnaround_time = 0;
        original[i].waiting_time = 0;
        original[i].is_completed = 0;
    }
}
// 생성된 전체 프로세스 시각화
void print_processes(Process p[]) {
    printf("\nGenerated Processes:\n");
    printf("PID\tArrival\tCPU Burst\tPriority\n");

    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].cpu_burst,
               p[i].priority);
    }
}
// 알고리즘 평가 함수 - waiting time, turnaround time 계산
Result evaluate(Process p[]) {
    double total_waiting_time = 0;
    double total_turnaround_time = 0;

    printf("\nEvaluation:\n");
    printf("PID\tArrival\tBurst\tPriority\tCompletion\tWaiting\tTurnaround\n");

    for (int i = 0; i < n; i++) {
        p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].cpu_burst;

        total_waiting_time += p[i].waiting_time;
        total_turnaround_time += p[i].turnaround_time;

        printf("P%d\t%d\t%d\t%d\t\t%d\t\t%d\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].cpu_burst,
               p[i].priority,
               p[i].completion_time,
               p[i].waiting_time,
               p[i].turnaround_time);
    }

    Result r;
    r.avg_waiting_time = total_waiting_time / n;
    r.avg_turnaround_time = total_turnaround_time / n;

    printf("\nAverage Waiting Time    : %.2f\n", r.avg_waiting_time);
    printf("Average Turnaround Time : %.2f\n", r.avg_turnaround_time);

    return r;
}

/* ---------------- Scheduling Algorithms ---------------- */
// FCFS --> FIFO
Result fcfs() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n================ FCFS ================\n");

    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int selected = -1;
        int earliest_arrival = INT_MAX;

        for (int i = 0; i < n; i++) {
            // 가장 일찍 도착한 process, 동일한 시간이면 프로세스 넘버 작은거 선택
            if (!p[i].is_completed && p[i].arrival_time < earliest_arrival) {
                earliest_arrival = p[i].arrival_time;
                selected = i;
            }
        }

        if (selected == -1) break;

        // 유휴 상태 표시
        if (current_time < p[selected].arrival_time) {
            add_gantt_block(-1, current_time, p[selected].arrival_time);
            current_time = p[selected].arrival_time;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + p[selected].cpu_burst);
        current_time += p[selected].cpu_burst;

        p[selected].completion_time = current_time;
        p[selected].is_completed = 1;
        completed++;
    }

    print_gantt_chart();
    return evaluate(p);
}
// nonpreemptive이므로 온 순서와 CPU burst timew 동시 판단
Result sjf_nonpreemptive() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n========== Non-Preemptive SJF ==========\n");

    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int selected = -1;
        int shortest_burst = INT_MAX;

        for (int i = 0; i < n; i++) {
            if (!p[i].is_completed &&
                p[i].arrival_time <= current_time &&
                p[i].cpu_burst < shortest_burst) {
                shortest_burst = p[i].cpu_burst;
                selected = i;
            }
        }

        if (selected == -1) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + p[selected].cpu_burst);
        current_time += p[selected].cpu_burst;

        p[selected].completion_time = current_time;
        p[selected].is_completed = 1;
        completed++;
    }

    print_gantt_chart();
    return evaluate(p);
}
// 매 시간마다 shortest job을 검증할 수 있도록 설계
Result sjf_preemptive() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n========== Preemptive SJF / SRTF ==========\n");

    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int selected = -1;
        int shortest_remaining = INT_MAX;

        for (int i = 0; i < n; i++) {
            if (!p[i].is_completed &&
                p[i].arrival_time <= current_time &&
                p[i].remaining_time < shortest_remaining) {
                shortest_remaining = p[i].remaining_time;
                selected = i;
            }
        }

        if (selected == -1) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + 1);
        p[selected].remaining_time--;
        current_time++;

        if (p[selected].remaining_time == 0) {
            p[selected].completion_time = current_time;
            p[selected].is_completed = 1;
            completed++;
        }
    }

    print_gantt_chart();
    return evaluate(p);
}
// priority 선택 후, CPU burst time 동안은 관리 필요 X
Result priority_nonpreemptive() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n======= Non-Preemptive Priority =======\n");

    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int selected = -1;
        int highest_priority = INT_MAX;
        // priority value가 낮을 수록 priority가 높음
        for (int i = 0; i < n; i++) {
            if (!p[i].is_completed &&
                p[i].arrival_time <= current_time &&
                p[i].priority < highest_priority) {
                highest_priority = p[i].priority;
                selected = i;
            }
        }

        if (selected == -1) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + p[selected].cpu_burst);
        current_time += p[selected].cpu_burst;

        p[selected].completion_time = current_time;
        p[selected].is_completed = 1;
        completed++;
    }

    print_gantt_chart();
    return evaluate(p);
}
// 추가로 도착하는 프로세스가 존재할 수 있으므로, 매 시간마다 priority 검증
Result priority_preemptive() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n========= Preemptive Priority =========\n");

    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int selected = -1;
        int highest_priority = INT_MAX;

        for (int i = 0; i < n; i++) {
            if (!p[i].is_completed &&
                p[i].arrival_time <= current_time &&
                p[i].priority < highest_priority) {
                highest_priority = p[i].priority;
                selected = i;
            }
        }

        if (selected == -1) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + 1);
        p[selected].remaining_time--;
        current_time++;

        if (p[selected].remaining_time == 0) {
            p[selected].completion_time = current_time;
            p[selected].is_completed = 1;
            completed++;
        }
    }

    print_gantt_chart();
    return evaluate(p);
}
// time quantum은 macro로 정의한 3인 상태임
Result round_robin() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n============= Round Robin =============\n");
    printf("Time Quantum = %d\n", TIME_QUANTUM);

    int current_time = 0;
    int completed = 0;
    int queue[MAX_GANTT];
    int front = 0, rear = 0;
    int in_queue[MAX_PROCESSES] = {0}; //queue에 한 사이클동안 들어갔는지 검증

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (p[i].arrival_time <= current_time &&
                !p[i].is_completed &&
                !in_queue[i]) {
                queue[rear++] = i;
                in_queue[i] = 1;
            }
        }
        // queue에 추가된 프로세스가 없을 때
        if (front == rear) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        int selected = queue[front++];

        int exec_time;
        if (p[selected].remaining_time > TIME_QUANTUM) {
            exec_time = TIME_QUANTUM;
        } else {
            exec_time = p[selected].remaining_time;
        }

        add_gantt_block(p[selected].pid, current_time, current_time + exec_time);

        p[selected].remaining_time -= exec_time;
        current_time += exec_time;

        // time quantum 동안 도착한 다른 프로세스 추가해줌
        for (int i = 0; i < n; i++) {
            if (p[i].arrival_time <= current_time &&
                !p[i].is_completed &&
                !in_queue[i]) {
                queue[rear++] = i;
                in_queue[i] = 1;
            }
        }

        if (p[selected].remaining_time == 0) {
            p[selected].completion_time = current_time;
            p[selected].is_completed = 1;
            completed++;
        } else {
            queue[rear++] = selected;
        }
    }
    print_gantt_chart();
    return evaluate(p);
}

/* ---------------- Comparison ---------------- */

void compare_results(Result results[], const char *names[], int count) {
    printf("\n\n================ Algorithm Comparison ================\n");
    printf("Algorithm\t\t\tAvg Waiting\tAvg Turnaround\n");

    int best_waiting_idx = 0;
    int best_turnaround_idx = 0;

    for (int i = 0; i < count; i++) {
        printf("%-30s\t%.2f\t\t%.2f\n",
               names[i],
               results[i].avg_waiting_time,
               results[i].avg_turnaround_time);

        if (results[i].avg_waiting_time < results[best_waiting_idx].avg_waiting_time) {
            best_waiting_idx = i;
        }

        if (results[i].avg_turnaround_time < results[best_turnaround_idx].avg_turnaround_time) {
            best_turnaround_idx = i;
        }
    }

    printf("\nBest Average Waiting Time    : %s\n", names[best_waiting_idx]);
    printf("Best Average Turnaround Time : %s\n", names[best_turnaround_idx]);
}

/* ---------------- Main Function ---------------- */

int main() {
    create_processes();
    print_processes(original);

    Result results[6];
    const char *names[6] = {
        "FCFS",
        "Non-Preemptive SJF",
        "Preemptive SJF / SRTF",
        "Non-Preemptive Priority",
        "Preemptive Priority",
        "Round Robin"
    };

    results[0] = fcfs();
    results[1] = sjf_nonpreemptive();
    results[2] = sjf_preemptive();
    results[3] = priority_nonpreemptive();
    results[4] = priority_preemptive();
    results[5] = round_robin();

    compare_results(results, names, 6);

    return 0;
}