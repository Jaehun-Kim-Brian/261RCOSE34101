 ### Gantt Chart 생성 및 시각화
`clear_gantt()`
`add_gantt_block`
`print_gantt_chart`

### .
`evaluate`
`copy_processes`

### 프로세스 생성 및 시각화
`create_processes`
-> `generate_random_pid`

`print_processes`

### 프로세스 상태 확인 및 결정
`should_start_io` --> 실행 중인 프로세스가 I/O burst로 전환돼야하는지 확인
-> `round_robin`


`start_io` --> I/O 프로세스로 전환
-> `rr_move_to_waiting_queue`

`update_io` --> 매 시간마다 모든 I/O 프로세스의 상태 업데이트
-> `rr_update_waiting_queue`

`is_ready` --> 당장 CPU busrt가 가능한 프로세스인지 판단


### RR 제외 스케줄링 알고리즘

`run_basic_scheduler`
-> `copy_processes`

-> `select_fcfs` --> 현 시간에 FCFS 우선 순위 판단
    -> `is_ready`

-> `select_sjf` --> 현 시간에 shortes job 프로세스 판단
    -> `is_ready`

-> `select_priority` --> 현시간에 priority value 평가
    -> `is_ready`

-> `update_io`
-> `should_start_io`
    -> `start_io`

-> `evaluate`

`fcfs`
`sjf_nonpreemptive`
`sjf_preemptive`
`priority_nonpreemptive`
`priority_preemptive`
-> `run_basic_scheduler`

### Queue 정의
`init_queue`
`is_queue_empty`
`is_queue_full`
`enqueue`
`dequeue`

### Round Robin
`set_rr_visualization_menu`

`print_rr_queues`
-> `print_rr_queue`

`rr_enqueue_new_arrivals` -> 아직 레디큐에 들어가본적 없는 프로세스 넣어주기
`rr_update_waiting_queue` -> I/O burst가 끝난 프로세스는 dequeue 및 I/O 프로세스 일괄 진행

`rr_move_to_waiting_queue` -> I/O 를 시작하여 waiting큐로 이전

`round_robin` 
-> `copy_processes`

-> `Config_RR`

-> `rr_enqueue_new_arrivals`

-> `rr_update_waiting_queue`

-> `should_start_io`
    -> `rr_move_to_waiting_queue`

-> `rr_enqueue_new_arrivals`