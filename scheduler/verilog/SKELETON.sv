//Verilog HDL for "DIGITAL", "SKELETON", "functional"
`define MAX_TASKS 32
`define MAX_TASK_BITS $clog2(`MAX_TASKS)

`define MAX_UTIL_PRECISION 1024
`define MAX_UTIL_BITS $clog2(`MAX_UTIL_PRECISION)

`define TIME_BITS 32

`define FALSE  1'h0
`define TRUE  1'h1

typedef enum logic {
	PERIODIC = 1'h0,
	SPORADIC  = 1'h1
} TASK_TYPE;

typedef enum logic [1:0] {
	LOW = 2'h0,
	HIGH_HIGH_MODE = 2'h1,
    HIGH_LOW_MODE = 2'h2
} TASK_CRITICALITY;

typedef enum logic {
	LOW_CRIT = 1'h0,
    HIGH_CRIT = 1'h1
} TASK_CRIT_IN;

typedef enum logic [1:0] {
	IDLE = 2'h0,
    DROPPED = 2'h1,
	BLOCKED = 2'h2,
    READY = 2'h3
} TASK_STATE;

typedef struct packed {
    logic valid;
    TASK_TYPE task_type;
    TASK_CRITICALITY criticality;
    TASK_STATE state;
    logic [`TIME_BITS-1:0] period;
    logic [`TIME_BITS-1:0] virtual_deadline;
    logic [`TIME_BITS-1:0] ex_high;
    logic [`TIME_BITS-1:0] ex_low;
    logic [`MAX_UTIL_BITS-1:0] utilization;

    logic [`TIME_BITS-1:0] wakeup;
    logic [`TIME_BITS-1:0] absolute_deadline;
    logic [`TIME_BITS-1:0] scheduling_deadline;
    logic [`TIME_BITS-1:0] ex_time;
} TASK_TABLE_ENTRY;

typedef struct packed {
    logic valid;
    logic [`MAX_TASK_BITS-1:0] id;
    TASK_TYPE task_type;
    TASK_CRIT_IN criticality;
    logic [`TIME_BITS-1:0] period;
    logic [`TIME_BITS-1:0] virtual_deadline;
    logic [`MAX_UTIL_BITS-1:0] utilization;
    logic [`TIME_BITS-1:0] ex_high;
    logic [`TIME_BITS-1:0] ex_low;
} TASK_TABLE_INPUT;

module SKELETON (
    input clk,
    input rst,
    input en,

    input TASK_TABLE_INPUT input_task,

    input wakeup_valid,
    input [`MAX_TASK_BITS-1:0] wakeup_id,

    input completion_valid,
    input completion_succesful,

    output [`MAX_TASK_BITS-1:0] running_task,
    output [`MAX_TASK_BITS-1:0] next_task,

    output running_valid,
    output next_valid,

    output cpu_interrupt
);

logic [`TIME_BITS-1:0] current_time;

always_ff @(posedge clk) begin
    if (rst || current_time[`TIME_BITS-1]) begin
        current_time <= 0;
    end else begin 
        current_time <= current_time + 1;
    end
end

logic [`MAX_UTIL_BITS-1:0] max_utilization;
logic [`MAX_UTIL_BITS-1:0] cur_utilization;
logic [`MAX_UTIL_BITS-1:0] n_cur_utilization;
logic [`MAX_UTIL_BITS-1:0] target_utilization;
logic [`MAX_UTIL_BITS-1:0] n_target_utilization;

TASK_TABLE_ENTRY [`MAX_TASKS-1:0] task_table;
TASK_TABLE_ENTRY [`MAX_TASKS-1:0] n_task_table;

logic [`MAX_TASKS-1:0][`MAX_TASK_BITS-1:0] ready_queue;
logic [`MAX_TASKS-1:0][`MAX_TASK_BITS-1:0] n_ready_queue;
logic [`MAX_TASK_BITS-1:0] queue_size;

logic insert_valid;
logic pop_valid;
logic drop_valid;
logic [`MAX_TASK_BITS-1:0] insert_id;
logic [`MAX_TASK_BITS-1:0] drop_index;

logic [`MAX_TASK_BITS-1:0] current_criticality;
logic criticality_transition;

assign running_task = ready_queue[0];
assign next_task = ready_queue[1];
assign running_valid = task_table[running_task].state == READY;
assign next_valid = task_table[next_task].state == READY && running_task != next_task;
assign cpu_interrupt = ready_queue[0] != n_ready_queue[0];

always_comb begin
    n_task_table = task_table;
    n_ready_queue = ready_queue;
    n_target_utilization = target_utilization;
    n_cur_utilization = cur_utilization;
    insert_valid = `FALSE;
    insert_id = 0;
    drop_valid = `FALSE;
    drop_index = 0;
    criticality_transition = `FALSE;
    pop_valid = completion_valid;

    // Insert inputted task to table
    if (input_task.valid) begin 
        n_task_table[input_task.id].valid = `TRUE;
        n_task_table[input_task.id].task_type = input_task.task_type;
        n_task_table[input_task.id].criticality = input_task.criticality == LOW_CRIT ? LOW : HIGH_LOW_MODE;
        n_task_table[input_task.id].state = IDLE;
        n_task_table[input_task.id].period = input_task.period;
        n_task_table[input_task.id].virtual_deadline = input_task.virtual_deadline;
        n_task_table[input_task.id].ex_high = input_task.ex_high;
        n_task_table[input_task.id].ex_low = input_task.ex_low;
        n_task_table[input_task.id].utilization = input_task.utilization;
        n_task_table[input_task.id].wakeup = 0;
        n_task_table[input_task.id].absolute_deadline = 0;
        n_task_table[input_task.id].scheduling_deadline = 0;
        n_task_table[input_task.id].ex_time = 0;
    end

    // Wakeup periodic tasks
    for (int unsigned i = 0; i < `MAX_TASKS; ++i) begin
        if (task_table[i].valid && task_table[i].task_type == PERIODIC && task_table[i].state == IDLE && task_table[i].wakeup <= current_time) begin
            if (task_table[wakeup_id].state == DROPPED && task_table[i].criticality == LOW) begin
                n_task_table[i].wakeup = n_task_table[i].wakeup + task_table[i].period;
            end else begin
                insert_valid = `TRUE;
                insert_id = i;
                break;
            end
        end
    end
    
    // wakeup blocked tasks or interrupts
    if (wakeup_valid) begin
        if (task_table[wakeup_id].state == DROPPED) begin
            n_task_table[wakeup_id].state = IDLE;
            n_task_table[wakeup_id].ex_time = 0;
            if (task_table[wakeup_id].task_type == PERIODIC) begin
                n_task_table[wakeup_id].wakeup = n_task_table[wakeup_id].wakeup + task_table[wakeup_id].period;
            end
        end else if (task_table[wakeup_id].state == BLOCKED) begin
            insert_valid = `TRUE;
            insert_id = wakeup_id;
        end else if (task_table[wakeup_id].state == IDLE && task_table[wakeup_id].task_type == SPORADIC) begin
            insert_valid = `TRUE;
            insert_id = wakeup_id;
        end
    end

    // Handle removal of tasks from queue
    if ((completion_valid && completion_succesful) || 
        (running_valid && current_time >= task_table[running_task].absolute_deadline) ||
        (running_valid && task_table[running_task].ex_time >= task_table[running_task].ex_low && task_table[running_task].criticality != HIGH_LOW_MODE) ||
        (running_valid && task_table[running_task].state == DROPPED)) begin
        pop_valid = `TRUE;
        n_task_table[running_task].state = IDLE;
        n_task_table[running_task].ex_time = 0;
        if (task_table[running_task].task_type == PERIODIC) begin
            n_task_table[running_task].wakeup = task_table[running_task].wakeup + task_table[running_task].period;
        end
    end else if (running_valid && task_table[running_task].ex_time >= task_table[running_task].ex_low && task_table[running_task].criticality == HIGH_LOW_MODE) begin
        n_task_table[running_task].criticality = HIGH_HIGH_MODE;
        n_task_table[running_task].scheduling_deadline = task_table[running_task].absolute_deadline;
        n_task_table[running_task].ex_time = task_table[running_task].ex_time + 1;
        criticality_transition = `TRUE;
        n_target_utilization = target_utilization - task_table[running_task].utilization;
    end else if (completion_valid && ~completion_succesful) begin 
        n_task_table[running_task].state = BLOCKED;
    end else if (running_valid) begin 
        n_task_table[running_task].ex_time = task_table[running_task].ex_time + 1;
    end

    // Handle insertion of tasks to queue
    if (insert_valid) begin
        if (task_table[insert_id].task_type == PERIODIC) begin
            n_task_table[insert_id].absolute_deadline = task_table[insert_id].wakeup + task_table[insert_id].period;
            if (task_table[insert_id].criticality == HIGH_LOW_MODE) begin
                n_task_table[insert_id].scheduling_deadline = task_table[insert_id].wakeup + task_table[insert_id].virtual_deadline;
            end else begin
                n_task_table[insert_id].scheduling_deadline = task_table[insert_id].wakeup + task_table[insert_id].period;
            end
        end else begin
            n_task_table[insert_id].absolute_deadline = current_time + task_table[insert_id].period;
            if (task_table[insert_id].criticality == HIGH_LOW_MODE) begin
                n_task_table[insert_id].scheduling_deadline = current_time + task_table[insert_id].virtual_deadline;
            end else begin
                n_task_table[insert_id].scheduling_deadline = current_time + task_table[insert_id].period;
            end
        end
        n_task_table[insert_id].state = READY;
        n_task_table[insert_id].ex_time = task_table[insert_id].ex_time + 1;
    end

    // Actually add or remove from queue
    if (insert_valid && pop_valid) begin
        for (int i = 1; i < `MAX_TASKS; ++i) begin
            if (i >= queue_size || task_table[ready_queue[i]].scheduling_deadline > n_task_table[insert_id].scheduling_deadline) begin
                n_ready_queue[i - 1] = insert_id;
                break;
            end
            n_ready_queue[i - 1] = ready_queue[i];
        end
    end else if (insert_valid) begin
        for (int i = `MAX_TASKS - 1; i >= 0; --i) begin
            if (i == 0) begin
                n_ready_queue[i] = insert_id;
                break;
            end
            if (i <= queue_size && task_table[ready_queue[i - 1]].scheduling_deadline <= n_task_table[insert_id].scheduling_deadline) begin
                n_ready_queue[i] = insert_id;
                break;
            end
            n_ready_queue[i] = ready_queue[i - 1];
        end
    end else if (pop_valid) begin
        for (int i = 1; i < `MAX_TASKS; ++i) begin
            n_ready_queue[i - 1] = ready_queue[i];
        end
    end

    if (target_utilization > cur_utilization) begin
        for (int i = 0; i < `MAX_TASKS; i++) begin
            if (task_table[i].valid && task_table[i].criticality == LOW && task_table[i].state != DROPPED && 
                (~drop_valid || task_table[i].utilization > task_table[drop_index].utilization)) begin
                drop_valid = `TRUE;
                drop_index = i;
            end
        end
        if (drop_valid) begin
            n_task_table[drop_index].state = DROPPED;
        end
    end
    
    if (~running_valid && current_criticality > 0) begin
        for (int i = 0; i < `MAX_TASKS - 1; ++i) begin
            if (task_table[i].valid && task_table[i].criticality == HIGH_HIGH_MODE) begin
                n_task_table[i].criticality = HIGH_LOW_MODE;
            end else if (task_table[i].valid && task_table[i].criticality == LOW && task_table[i].state == DROPPED) begin
                n_task_table[i].state = IDLE;
            end
        end
    end

    if (current_time[`TIME_BITS-1]) begin
        for (int i = 0; i < `MAX_TASKS - 1; ++i) begin
            n_task_table[i].wakeup[`TIME_BITS - 1] = 0;
            n_task_table[i].absolute_deadline[`TIME_BITS - 1] = 0;
            n_task_table[i].scheduling_deadline[`TIME_BITS - 1] = 0;
        end
    end
end

always_ff @(posedge clk) begin
    if (rst) begin
        task_table <= 0;
        ready_queue <= 0;
        current_criticality <= 0;
        queue_size <= 0;
        max_utilization <= 0;
        target_utilization <= 0;
        cur_utilization <= 0;
    end else if (en) begin 
        if (input_task.valid && input_task.criticality == LOW_CRIT) begin
            target_utilization <= n_target_utilization + input_task.utilization;
            cur_utilization <= n_cur_utilization + input_task.utilization;
            max_utilization <= max_utilization + input_task.utilization;
        end else begin
            target_utilization <= n_target_utilization;
            cur_utilization <= n_cur_utilization;
        end
        task_table <= n_task_table;
        ready_queue <= n_ready_queue;
        if (insert_valid & ~pop_valid) begin
            queue_size <= queue_size + 1;
        end else if (~insert_valid & pop_valid) begin
            queue_size <= queue_size - 1;
        end
        if (criticality_transition) begin
            current_criticality <= current_criticality + 1;
        end else if (~running_valid) begin 
            current_criticality <= 0;
            cur_utilization <= max_utilization;
            target_utilization <= max_utilization;
        end
    end
end

endmodule
