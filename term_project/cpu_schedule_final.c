// 헤더 load
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

// 매크로 정의 
#define MAX_PROCESSES 9
#define MAX_GANTT 1000 // Gantt 차트에서 저장할 최대 block 수
#define TIME_QUANTUM 3

// 각 프로세스 구조체
typedef struct{
    // PID 기본 정보
    int pid;
    int arrival_time;
    int cpu_burst;
    int remaining_time;
    int priority;
    // I/O 프로세스일 때의 추가 정보
    int has_io;
    int io_start;
    int io_burst;
    int io_remaining;
    int io_done;
    int is_blocked;
    int executed_cpu;
    int ready_time;
    // 완료 및 평가지표
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int is_completed;
} Process;

// Gantt 차트의 한 칸 표현을 위한 구조체
typedef struct{
    int pid;
    int start_time;
    int end_time;
} GanttBlock;

// Round Robin에서 Ready큐, Waiting큐를 위한 큐 구조체 정의
typedef struct {
    int items[MAX_GANTT]; // 프로세스의 인덱스를 넣을 것임
    int front;
    int rear;
    int count;
    const char *name;
} Queue;

// 각 알고리즘 실행 결과 저장 구조체
typedef struct{
    double avg_waiting_time;
    double avg_turnaround_time;
} Result;

// Basic 알고리즘 구동을 위한 표시자
typedef enum {
    ALG_FCFS,
    ALG_SJF_NP,
    ALG_SJF_P,
    ALG_PRIORITY_NP,
    ALG_PRIORITY_P
} AlgorithmType;

// --> 전역 변수 설정
int n; // 실제 프로세스 개수
Process original[MAX_PROCESSES]; // MAX_PROCESSES 개의 프로세스 미리 설정

int gantt_count = 0;
GanttBlock gantt[MAX_GANTT]; // MAX_GANTT 개의 Gantt block 미리 설정

Queue rr_ready_queue;
Queue rr_waiting_queue;
int rr_has_arrived[MAX_PROCESSES]; // ready queue에 프로세스가 등록된적이 있는지
int rr_in_ready_queue[MAX_PROCESSES]; // 현재 ready queue 안에 프로세스가 있는지
int rr_in_waiting_queue[MAX_PROCESSES]; // 현재 waiting queue 안에 프로세스가 있는지
int rr_visualization_enabled = 0;

// --> 함수 정의

// Gantt Chart 관련
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
        dest[i].executed_cpu = 0;
        dest[i].io_remaining = 0;
        dest[i].io_done = 0;
        dest[i].is_blocked = 0;
        dest[i].ready_time = src[i].arrival_time;
        dest[i].completion_time = 0;
        dest[i].turnaround_time = 0;
        dest[i].waiting_time = 0;
        dest[i].is_completed = 0;
    }
}

// process를 랜덤 값들로 생성
int generate_random_pid(int process_index) {
    int pid;
    // index 0 -> 1000~1999
    // index 1 -> 2000~2999
    int random_suffix = rand() % 1000;  // 000 ~ 999
    pid = (process_index+1) * 1000 + random_suffix;
    return pid;
}

void create_processes() {
    srand(time(NULL));

    printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
    scanf("%d", &n);

    if (n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes.\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        original[i].pid = generate_random_pid(i);
        original[i].arrival_time = rand() % 10;       // 0 ~ 9
        original[i].cpu_burst = (rand() % 9) + 1;     // 1 ~ 9
        original[i].remaining_time = original[i].cpu_burst;
        original[i].priority = (rand() % 5) + 1;      // 1 ~ 5, 작을 수록 높은 우선순위
        
        // CPU burst가 3 이상인 프로세스에 대해 50%의 확률로 I/O 프로세스로 생성
        if (original[i].cpu_burst >= 3 && rand() % 2 == 1) {
            original[i].has_io = 1;
            original[i].io_start = (rand() % (original[i].cpu_burst - 1)) + 1; // CPU burst 종료 전에 I/O가 시작되도록
            original[i].io_burst = (rand() % 5) + 1;  // 1 ~ 5
        } else {
            original[i].has_io = 0;
            original[i].io_start = -1;
            original[i].io_burst = 0;
        }

        original[i].io_remaining = 0;
        original[i].io_done = 0;
        original[i].is_blocked = 0;
        original[i].executed_cpu = 0;
        original[i].ready_time = original[i].arrival_time;

        original[i].completion_time = 0;
        original[i].turnaround_time = 0;
        original[i].waiting_time = 0;
        original[i].is_completed = 0;
    }
}

// 생성된 전체 프로세스 시각화
void print_processes(Process p[]) {
    printf("\nGenerated Processes:\n");
    printf("PID\tArrival\tCPU Burst\tPriority\tI/O\tI/O Start\tI/O Burst\n");
    for (int i = 0; i < n; i++) {
        if (p[i].has_io) {
            printf("P%d\t%d\t%d\t\t%d\t\tYes\t%d\t\t%d\n",
                   p[i].pid,
                   p[i].arrival_time,
                   p[i].cpu_burst,
                   p[i].priority,
                   p[i].io_start,
                   p[i].io_burst);
        } else {
            printf("P%d\t%d\t%d\t\t%d\t\tNo\t-\t\t-\n",
                   p[i].pid,
                   p[i].arrival_time,
                   p[i].cpu_burst,
                   p[i].priority);
        }
    }
}

// 알고리즘 평가 함수 - waiting time, turnaround time 계산
Result evaluate(Process p[]) {
    double total_waiting_time = 0;
    double total_turnaround_time = 0;

    printf("\nEvaluation:\n");
    printf("PID\tArrival\tBurst\tI/O\tPriority\tCompletion\tWaiting\tTurnaround\n");

    for (int i = 0; i < n; i++) {
        int total_io_time = p[i].has_io ? p[i].io_burst : 0; // waiting time에서 I/O burst time은 배제

        p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].cpu_burst - total_io_time;

        total_waiting_time += p[i].waiting_time;
        total_turnaround_time += p[i].turnaround_time;

        printf("P%d\t%d\t%d\t%d\t%d\t\t%d\t\t%d\t%d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].cpu_burst,
               total_io_time,
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
// I/O burst 시작해야하는 시점
int should_start_io(Process *p) {
    return p->has_io &&
           !p->io_done &&
           p->executed_cpu == p->io_start &&
           p->remaining_time > 0;
}

// I/O 시작에 설정, I/O remaining 적용
void start_io(Process *p) {
    p->is_blocked = 1;
    p->io_done = 1;
    p->io_remaining = p->io_burst;
}

// 매시간마다 I/O process 처리
void update_io(Process p[], int current_time) {
    for (int i = 0; i < n; i++) {
        if (p[i].is_blocked) {
            p[i].io_remaining--;
            if (p[i].io_remaining <= 0) {
                p[i].is_blocked = 0; // 다시 CPU burst 시작 가능.1
                p[i].ready_time = current_time; // 다시 CPU burst 시작 가능.2
            }
        }
    }
}

// CPU_busrt를 진행하는 시간 판단
int is_ready(Process p[], int i, int current_time) {
    return !p[i].is_completed &&
           !p[i].is_blocked &&
           p[i].arrival_time <= current_time &&
           p[i].remaining_time > 0;
}

// FCFS --> FIFO
// 가장 일찍 준비된 프로세스의 인덱스 호출
int select_fcfs(Process p[], int current_time) {
    int selected = -1;
    int best_ready_time = INT_MAX;
    for (int i = 0; i < n; i++) {
        if (is_ready(p, i, current_time)) {
            if (p[i].ready_time < best_ready_time) {
                best_ready_time = p[i].ready_time;
                selected = i;
            }
        }
    }
    return selected;
}

// 준비된 프로세스 중 remaining time이 가장 짧은 프로세스 인덱스 호출
int select_sjf(Process p[], int current_time) {
    int selected = -1;
    int shortest_remaining = INT_MAX;
    for (int i = 0; i < n; i++) {
        if (is_ready(p, i, current_time)) {
            if (p[i].remaining_time < shortest_remaining) {
                shortest_remaining = p[i].remaining_time;
                selected = i;
            }
        }
    }
    return selected;
}

// 준비된 process 중 priority value가 가장 작은(우선순위가 높은) 프로세스 선택
int select_priority(Process p[], int current_time) {
    int selected = -1;
    int highest_priority = INT_MAX;
    for (int i = 0; i < n; i++) {
        if (is_ready(p, i, current_time)) {
            if (p[i].priority < highest_priority) {
                highest_priority = p[i].priority;
                selected = i;
            }
        }
    }
    return selected;
}

Result run_basic_scheduler(AlgorithmType type, const char *title) {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n================ %s ================\n", title);

    int current_time = 0;
    int completed = 0;
    int running = -1;

    while (completed < n) {
        // Non-preemptive 알고리즘에서는 running 프로세스가 있으면 계속 실행
        // Preemptive 알고리즘에서는 매시간마다 새로 선택
        if (type == ALG_FCFS ||
            type == ALG_SJF_NP ||
            type == ALG_PRIORITY_NP) {

            if (running == -1) {
                if (type == ALG_FCFS) {
                    running = select_fcfs(p, current_time);
                } else if (type == ALG_SJF_NP) {
                    running = select_sjf(p, current_time);
                } else {
                    running = select_priority(p, current_time);
                }
            }

        } else {
            if (type == ALG_SJF_P) {
                running = select_sjf(p, current_time);
            } else {
                running = select_priority(p, current_time);
            }
        }
        // 유휴 상태 처리
        if (running == -1) {
            add_gantt_block(-1, current_time, current_time + 1);
            current_time++;
            update_io(p, current_time);
            continue;
        }

        // 1 시간 진행
        add_gantt_block(p[running].pid, current_time, current_time + 1);
        p[running].remaining_time--;
        p[running].executed_cpu++;
        current_time++;

        // I/O 프로세스도 진행
        update_io(p, current_time);

        if (p[running].remaining_time == 0) {
            p[running].completion_time = current_time;
            p[running].is_completed = 1;
            completed++;
            running = -1;
        }
        
        // 프로세스가 I/O를 시작해야 하는 경우
        else if (should_start_io(&p[running])) {
            start_io(&p[running]);
            running = -1;
        }
        // Preemptive 알고리즘은 다음 루프에서 프로세스 선택을 완전히 다시 시작
        else if (type == ALG_SJF_P || type == ALG_PRIORITY_P) {
            running = -1;
        }
    }

    print_gantt_chart();
    return evaluate(p);
}

Result fcfs() {
    return run_basic_scheduler(ALG_FCFS, "FCFS");
}

Result sjf_nonpreemptive() {
    return run_basic_scheduler(ALG_SJF_NP, "Non-Preemptive SJF");
}

Result sjf_preemptive() {
    return run_basic_scheduler(ALG_SJF_P, "Preemptive SJF");
}

Result priority_nonpreemptive() {
    return run_basic_scheduler(ALG_PRIORITY_NP, "Non-Preemptive Priority");
}

Result priority_preemptive() {
    return run_basic_scheduler(ALG_PRIORITY_P, "Preemptive Priority");
}

// Queue 구조체 시동
void init_queue(Queue *q, const char *name) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->name = name;
}

int is_queue_empty(Queue *q) {
    return q->count == 0;
}

int is_queue_full(Queue *q) {
    return q->count == MAX_GANTT;
}

// 원형 큐를 구성했기에 % MAX_GANTT를 해줌
void enqueue(Queue *q, int process_index) {
    if (is_queue_full(q)) {
        printf("%s is full. Cannot enqueue process.\n", q->name);
        return;
    }
    q->items[q->rear] = process_index;
    q->rear = (q->rear + 1) % MAX_GANTT;
    q->count++;
}

// 원형 큐를 구성했기에 % MAX_GANTT를 해줌
int dequeue(Queue *q) {
    if (is_queue_empty(q)) {
        return -1;
    }

    int process_index = q->items[q->front];
    q->front = (q->front + 1) % MAX_GANTT;
    q->count--;
    return process_index;
}

// Config 함수를 통한 queue 설정
void Config_RR() {
    init_queue(&rr_ready_queue, "Ready Queue");
    init_queue(&rr_waiting_queue, "Waiting Queue");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        rr_has_arrived[i] = 0;
        rr_in_ready_queue[i] = 0;
        rr_in_waiting_queue[i] = 0;
    }
    printf("\n========== Round Robin Configuration ==========\n");
    printf("Ready Queue and Waiting Queue initialized.\n");
    printf("Time Quantum  : %d\n", TIME_QUANTUM);
    printf("Ready Queue   : stores processes ready to use CPU.\n");
    printf("Waiting Queue : stores processes blocked by I/O operation.\n");
    printf("==========================================\n");

}

void set_rr_visualization_menu() {
    int choice;

    printf("\n====== Round Robin Queue Visualization Setting ======\n");
    printf("Current setting: %s\n",
           rr_visualization_enabled ? "ON" : "OFF");
    printf("1. Turn ON queue visualization\n");
    printf("2. Turn OFF queue visualization\n");
    printf("Select: ");
    scanf("%d", &choice);

    if (choice == 1) {
        rr_visualization_enabled = 1;
        printf("Round Robin queue visualization is now ON.\n");
    } else if (choice == 2) {
        rr_visualization_enabled = 0;
        printf("Round Robin queue visualization is now OFF.\n");
    } else {
        printf("Invalid choice.\n");
    }
}

// demo를 위한 큐 시각화
void print_rr_queue(Queue *q, Process p[]) {
    printf("%s: ", q->name);
    if (is_queue_empty(q)) {
        printf("Empty\n");
        return;
    }

    int idx = q->front;
    for (int i = 0; i < q->count; i++) {
        int process_index = q->items[idx];
        printf("P%d ", p[process_index].pid);
        idx = (idx + 1) % MAX_GANTT;
    }
    printf("\n");
}

void print_rr_queues(Process p[]) {
    print_rr_queue(&rr_ready_queue, p);
    print_rr_queue(&rr_waiting_queue, p);
}

void rr_enqueue_new_arrivals(Process p[], int current_time) {
    for (int i = 0; i < n; i++) {
        if (!rr_has_arrived[i] &&
            !p[i].is_completed &&
            !p[i].is_blocked &&
            p[i].arrival_time <= current_time &&
            p[i].remaining_time > 0) {

            enqueue(&rr_ready_queue, i);

            rr_has_arrived[i] = 1;
            rr_in_ready_queue[i] = 1;
            if (rr_visualization_enabled == 1){
                printf("[Time %d] P%d arrives and enters Ready Queue.\n", current_time, p[i].pid);
            }
            
        }
    }
}

void rr_update_waiting_queue(Process p[], int current_time) {
    int waiting_count = rr_waiting_queue.count;

    for (int i = 0; i < waiting_count; i++) {
        int process_index = dequeue(&rr_waiting_queue);

        if (process_index == -1) {
            continue;
        }

        p[process_index].io_remaining--;

        if (p[process_index].io_remaining <= 0) {
            p[process_index].is_blocked = 0;
            p[process_index].ready_time = current_time;

            rr_in_waiting_queue[process_index] = 0;

            enqueue(&rr_ready_queue, process_index);
            rr_in_ready_queue[process_index] = 1;
            if (rr_visualization_enabled == 1){
                printf("[Time %d] P%d completes I/O and returns to Ready Queue.\n", current_time, p[process_index].pid);
            }
        } else {
            enqueue(&rr_waiting_queue, process_index);
            rr_in_waiting_queue[process_index] = 1;
        }
    }
}

void rr_move_to_waiting_queue(Process p[], int process_index, int current_time) {
    start_io(&p[process_index]);

    enqueue(&rr_waiting_queue, process_index);
    rr_in_waiting_queue[process_index] = 1;
    if (rr_visualization_enabled == 1){
        printf("[Time %d] P%d starts I/O and moves to Waiting Queue. I/O burst = %d\n", current_time, p[process_index].pid, p[process_index].io_burst);
    }
    
}

// time quantum은 macro로 정의한 3인 상태임
Result round_robin() {
    Process p[MAX_PROCESSES];
    copy_processes(p, original);
    clear_gantt();

    printf("\n============= Round Robin  =============\n");

    Config_RR();

    int current_time = 0;
    int completed = 0;
    int running = -1;
    int used_quantum = 0;

    while (completed < n) {
        // 처음 도착한 프로세스를 레디큐에 인큐
        rr_enqueue_new_arrivals(p, current_time);

        // running process 없으면 레디큐에서 디큐
        if (running == -1) {
            // 레디큐 비어잇으면 유휴 표시
            if (is_queue_empty(&rr_ready_queue)) {
                add_gantt_block(-1, current_time, current_time + 1);
                if (rr_visualization_enabled == 1){
                    printf("[Time %d] CPU is idle.\n", current_time);
                }
                
                current_time++;

                // 유휴 상태에서도 I/O 과정 업데이트
                rr_update_waiting_queue(p, current_time);

                continue;
            }

            running = dequeue(&rr_ready_queue);
            rr_in_ready_queue[running] = 0;
            used_quantum = 0;
            if (rr_visualization_enabled == 1){
                printf("[Time %d] P%d is dequeued from Ready Queue and gets CPU.\n", current_time, p[running].pid);
            }
        }

        // 1시간 진행
        add_gantt_block(p[running].pid, current_time, current_time + 1);

        p[running].remaining_time--;
        p[running].executed_cpu++;
        used_quantum++;
        if (rr_visualization_enabled == 1){
            printf("[Time %d ~ %d] P%d runs. Remaining CPU = %d\n", current_time, current_time + 1, p[running].pid, p[running].remaining_time);
        }
        current_time++;

        // I/O 프로세스도 1시간 진행
        rr_update_waiting_queue(p, current_time);

        // 프로세스 완료
        if (p[running].remaining_time == 0) {
            p[running].completion_time = current_time;
            p[running].is_completed = 1;
            if (rr_visualization_enabled == 1){
                printf("[Time %d] P%d completed.\n", current_time, p[running].pid);
            }
            completed++;
            running = -1;
            used_quantum = 0;
        }
        // 실행 중 I/O 발생
        else if (should_start_io(&p[running])) {
            rr_move_to_waiting_queue(p, running, current_time);
            running = -1;
            used_quantum = 0;
        }
        // Time Quantum 초과
        else if (used_quantum == TIME_QUANTUM) {
            // 현 프로세스를 레디큐에 놓기 전, 사이에 도착한 프로세스를 먼저 넣어줌
            rr_enqueue_new_arrivals(p, current_time);
            enqueue(&rr_ready_queue, running);
            rr_in_ready_queue[running] = 1;
            if (rr_visualization_enabled == 1){
                printf("[Time %d] Time quantum expired. P%d returns to Ready Queue.\n", current_time, p[running].pid);
            }
            
            running = -1;
            used_quantum = 0;
        }
        if (rr_visualization_enabled == 1){
            print_rr_queues(p);
        }
        
    }

    print_gantt_chart();
    return evaluate(p);
}

Result saved_results[6];
int result_available[6] = {0};
int process_created = 0;

const char *algorithm_names[6] = {
    "FCFS",
    "Non-Preemptive SJF",
    "Preemptive SJF / SRTF",
    "Non-Preemptive Priority",
    "Preemptive Priority",
    "Round Robin"
};

Result run_selected_algorithm(int choice) {
    switch (choice) {
        case 1:
            return fcfs();
        case 2:
            return sjf_nonpreemptive();
        case 3:
            return sjf_preemptive();
        case 4:
            return priority_nonpreemptive();
        case 5:
            return priority_preemptive();
        case 6:
            return round_robin();
        default:
            printf("Invalid algorithm choice.\n");
            Result empty = {0.0, 0.0};
            return empty;
    }
}

void run_all_algorithms() {
    if (!process_created) {
        printf("\nPlease create processes first.\n");
        return;
    }

    saved_results[0] = fcfs();
    result_available[0] = 1;
    saved_results[1] = sjf_nonpreemptive();
    result_available[1] = 1;
    saved_results[2] = sjf_preemptive();
    result_available[2] = 1;
    saved_results[3] = priority_nonpreemptive();
    result_available[3] = 1;
    saved_results[4] = priority_preemptive();
    result_available[4] = 1;
    saved_results[5] = round_robin();
    result_available[5] = 1;

    printf("\nAll scheduling algorithms have been executed.\n");
}

void run_one_algorithm_menu() {
    if (!process_created) {
        printf("\nPlease create processes first.\n");
        return;
    }

    int choice;

    printf("\n========== Select Scheduling Algorithm ==========\n");
    printf("1. FCFS\n");
    printf("2. Non-Preemptive SJF\n");
    printf("3. Preemptive SJF\n");
    printf("4. Non-Preemptive Priority\n");
    printf("5. Preemptive Priority\n");
    printf("6. Round Robin\n");
    printf("Select algorithm: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > 6) {
        printf("Invalid algorithm choice.\n");
        return;
    }

    saved_results[choice - 1] = run_selected_algorithm(choice);
    result_available[choice - 1] = 1;

    printf("\n%s has been executed and saved for comparison.\n",
           algorithm_names[choice - 1]);
}

void compare_saved_results() {
    int available_count = 0;

    for (int i = 0; i < 6; i++) {
        if (result_available[i]) {
            available_count++;
        }
    }

    if (available_count == 0) {
        printf("\nNo algorithm results available.\n");
        printf("Please run one or more scheduling algorithms first.\n");
        return;
    }

    printf("\n\n================ Algorithm Comparison ================\n");
    printf("Algorithm\t\t\tAvg Waiting\tAvg Turnaround\n");

    int best_waiting_idx = -1;
    int best_turnaround_idx = -1;

    for (int i = 0; i < 6; i++) {
        if (result_available[i]) {
            printf("%-30s\t%.2f\t\t%.2f\n",
                   algorithm_names[i],
                   saved_results[i].avg_waiting_time,
                   saved_results[i].avg_turnaround_time);

            if (best_waiting_idx == -1 ||
                saved_results[i].avg_waiting_time < saved_results[best_waiting_idx].avg_waiting_time) {
                best_waiting_idx = i;
            }
            if (best_turnaround_idx == -1 ||
                saved_results[i].avg_turnaround_time < saved_results[best_turnaround_idx].avg_turnaround_time) {
                best_turnaround_idx = i;
            }
        }
    }
    printf("\nBest Average Waiting Time    : %s\n", algorithm_names[best_waiting_idx]);
    printf("Best Average Turnaround Time : %s\n", algorithm_names[best_turnaround_idx]);
}

/* ---------------- Main Function ---------------- */
void print_main_menu() {
    printf("\n\n========== CPU Scheduling Simulator ==========\n");
    printf("1. Create Random Processes\n");
    printf("2. Run All Scheduling Algorithms\n");
    printf("3. Run One Scheduling Algorithm\n");
    printf("4. Compare Saved Results\n");
    printf("5. Print Current Process List\n");
    printf("6. Set Round Robin Queue Visualization [%s]\n", rr_visualization_enabled ? "ON" : "OFF");
    printf("0. Exit\n");
    printf("Select menu: ");
}

int main() {
    int menu_choice;

    while (1) {
        print_main_menu();
        scanf("%d", &menu_choice);
        switch (menu_choice) {
            case 1:
                create_processes();
                process_created = 1;
                for (int i = 0; i < 6; i++) {
                    result_available[i] = 0;
                }
                print_processes(original);
                printf("\nRandom processes have been created.\n");
                printf("Previous algorithm results have been cleared.\n");
                break;

            case 2:
                run_all_algorithms();
                break;

            case 3:
                run_one_algorithm_menu();
                break;

            case 4:
                compare_saved_results();
                break;

            case 5:
                if (!process_created) {
                    printf("\nNo processes have been created yet.\n");
                } else {
                    print_processes(original);
                }
                break;

            case 6:
                set_rr_visualization_menu();
                break;
            case 0:
                printf("\nExiting CPU Scheduling Simulator.\n");
                return 0;
            default:
                printf("\nInvalid menu choice. Please try again.\n");
                break;
        }
    }
    return 0;
}