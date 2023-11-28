//Verilog HDL for "DIGITAL", "SKELETON", "functional"
`define MAX_TASKS 32
`define MAX_TASK_BITS $clog2(`MAX_TASKS)

`define NUM_INTERRUPTS 8
`define NUM_INTERRUPT_BITS $clog2(`MAX_TASKS)

`define TIME_BITS 22

`define FALSE  1'h0
`define TRUE  1'h1

typedef enum logic [1:0] {
	PERIODIC = 2'h0,
	SPORADIC  = 2'h1,
    INTERRUPT = 2'h2
} TASK_TYPE;

typedef enum logic [1:0] {
	LOW = 2'h0,
	HIGH_HIGH_MODE = 2'h1,
    HIGH_LOW_MODE = 2'h2,
    INTERRUPT_CRITICALITY = 2'h3
} TASK_CRITICALITY;

typedef enum logic [1:0] {
	IDLE = 2'h0,
	BLOCKED = 2'h1,
    READY = 2'h2
} TASK_STATE;

typedef struct packed {
    logic valid;
    TASK_TYPE task_type;
    TASK_CRITICALITY criticality;
    logic [`TIME_BITS-1:0] next_wakeup;
    logic [`TIME_BITS-1:0] period;
    logic [`TIME_BITS-1:0] deadline;
    logic [`TIME_BITS-1:0] ex_high;
    logic [`TIME_BITS-1:0] ex_low;
} IDLE_TABLE_ENTRY;

typedef struct packed {
    logic valid;
    logic [`MAX_TASK_BITS-1:0] id;
    TASK_TYPE task_type;
    TASK_CRITICALITY criticality;
    logic [`TIME_BITS-1:0] ex_high;
    logic [`TIME_BITS-1:0] ex_low;

    logic [`TIME_BITS-1:0] absolute_deadline;
    logic [`TIME_BITS-1:0] ex_time;
} READY_QUEUE_ENTRY;

typedef struct packed {
    logic valid;
    TASK_TYPE task_type;
    TASK_CRITICALITY criticality;

    logic [`TIME_BITS-1:0] absolute_deadline;
    logic [`TIME_BITS-1:0] ex_time;
} BLOCKED_TABLE_ENTRY;

typedef struct packed {
    logic valid;
    logic [`MAX_TASK_BITS-1:0] id;
    TASK_TYPE task_type;
    TASK_CRITICALITY criticality;
    //logic [`NUM_INTERRUPT_BITS-1:0] interrupt_id;
    logic [`TIME_BITS-1:0] period;
    logic [`TIME_BITS-1:0] deadline;
    logic [`TIME_BITS-1:0] ex_high;
    logic [`TIME_BITS-1:0] ex_low;
} TASK_TABLE_INPUT;

module SKELETON (
    input clk,
    input rst,

    input TASK_TABLE_INPUT input_task,

    input wakeup_valid,
    input [`MAX_TASK_BITS-1:0] wakeup_id,
    //input [`NUM_INTERRUPTS-1:0] hardware_interrupts

    input completion_valid,
    input completion_succesful,

    input [3:0][`MAX_TASK_BITS-1:0] transition_nums,

    output [`MAX_TASK_BITS-1:0] running_task,
    output [`MAX_TASK_BITS-1:0] next_task,

    output running_valid,
    output next_valid,

    output cpu_interrupt
);

logic [`TIME_BITS-1:0] current_time;

IDLE_TABLE_ENTRY [`MAX_TASKS-1:0] idle_table;
IDLE_TABLE_ENTRY [`MAX_TASKS-1:0] n_idle_table;

BLOCKED_TABLE_ENTRY [`MAX_TASKS-1:0] blocked_table;
BLOCKED_TABLE_ENTRY [`MAX_TASKS-1:0] n_blocked_table;

READY_QUEUE_ENTRY [`MAX_TASKS-1:0] ready_queue;
READY_QUEUE_ENTRY [`MAX_TASKS-1:0] n_ready_queue;

READY_QUEUE_ENTRY queue_insert_task;

//logic [`MAX_TASK_BITS-1:0] queue_size;

logic pop_valid;

logic [`TIME_BITS-1:0] ex_limit;
logic [`MAX_TASK_BITS-1:0] current_criticality;
logic criticality_transition;

logic cpu_running;

always_comb begin
    cpu_running = running_task;
    for (int i = 0; i < `MAX_TASKS; ++i) begin
        cpu_running = cpu_running | blocked_table[i].valid;
    end
end

assign running_task = ready_queue[0].id;
assign next_task = ready_queue[1].id;
assign running_valid = ready_queue[0].valid;
assign next_valid = ready_queue[1].valid;
assign cpu_interrupt = ready_queue[0] != n_ready_queue[0];

always_comb begin
    n_idle_table = idle_table;
    n_blocked_table = blocked_table;
    n_ready_queue = ready_queue;
    criticality_transition = `FALSE;
    pop_valid = completion_valid;
    queue_insert_task = 0;

    // Insert inputted task to table
    if (input_task.valid) begin 
        n_idle_table[input_task.id].valid = `TRUE;
        n_idle_table[input_task.id].task_type = input_task.task_type;
        n_blocked_table[input_task.id].task_type = input_task.task_type;
        n_idle_table[input_task.id].criticality = input_task.criticality;
        n_idle_table[input_task.id].period = input_task.period;
        n_idle_table[input_task.id].deadline = input_task.deadline;
        n_idle_table[input_task.id].ex_high = input_task.ex_high;
        n_idle_table[input_task.id].ex_low = input_task.ex_low;
        if (input_task.task_type == PERIODIC) begin
            n_idle_table[input_task.id].next_wakeup = current_time;
        end else begin
            n_idle_table[input_task.id].next_wakeup = 0;
        end
    end


    for (int unsigned i = 0; i < `MAX_TASKS; ++i) begin
        if (blocked_table[i].valid && current_time >= blocked_table[i].absolute_deadline) begin
            n_idle_table[i].valid = `TRUE;
            n_idle_table[i].valid = blocked_table[i].criticality;
            n_blocked_table[i].valid = `FALSE;
            if (blocked_table[i].task_type == PERIODIC) begin
                n_idle_table[i].next_wakeup = idle_table[i].next_wakeup + idle_table[i].period;
            end
        end
    end

    if (wakeup_valid && blocked_table[wakeup_id].valid) begin
        if (current_time >= blocked_table[wakeup_id].absolute_deadline) begin
            queue_insert_task.valid = `FALSE;
        end else if (current_criticality >= transition_nums[3] && blocked_table[wakeup_id].criticality == LOW) begin
            n_idle_table[wakeup_id].valid = `TRUE;
            n_idle_table[wakeup_id].criticality = blocked_table[wakeup_id].criticality;
            n_blocked_table[wakeup_id].valid = `FALSE;
            if (blocked_table[wakeup_id].task_type == PERIODIC) begin
                n_idle_table[wakeup_id].next_wakeup = idle_table[wakeup_id].next_wakeup + idle_table[wakeup_id].period;
            end
        end else begin
            queue_insert_task.valid = `TRUE;
            queue_insert_task.id = wakeup_id;
            queue_insert_task.task_type = blocked_table[wakeup_id].task_type;
            queue_insert_task.criticality = blocked_table[wakeup_id].criticality;
            queue_insert_task.ex_high = idle_table[wakeup_id].ex_high;
            queue_insert_task.ex_low = idle_table[wakeup_id].ex_low;
            queue_insert_task.absolute_deadline = blocked_table[wakeup_id].absolute_deadline;
            queue_insert_task.ex_time = blocked_table[wakeup_id].ex_time;
        end
    end else if (wakeup_valid && idle_table[wakeup_id].valid && 
                (idle_table[wakeup_id].task_type == INTERRUPT || idle_table[wakeup_id].task_type == SPORADIC)) begin
        queue_insert_task.valid = `TRUE;
        queue_insert_task.id = wakeup_id;
        queue_insert_task.task_type = idle_table[wakeup_id].task_type;
        queue_insert_task.criticality = idle_table[wakeup_id].criticality;
        queue_insert_task.ex_high = idle_table[wakeup_id].ex_high;
        queue_insert_task.ex_low = idle_table[wakeup_id].ex_low;
        queue_insert_task.absolute_deadline = current_time + idle_table[wakeup_id].deadline;
        queue_insert_task.ex_time = 0;
    end 

    if (~cpu_running) begin
        for (int unsigned i = 0; i < `MAX_TASKS; ++i) begin
            if (idle_table[i].valid && idle_table[i].criticality == HIGH_HIGH_MODE) begin
                n_idle_table[i].criticality = HIGH_LOW_MODE;
            end
        end
    end

    // Determing execution limit based on criticality
    if (ready_queue[0].criticality == LOW) begin
        if (current_criticality >= transition_nums[2]) begin
            ex_limit = ready_queue[0].ex_low >> 2;
        end else if (current_criticality >= transition_nums[1]) begin
            ex_limit = ready_queue[0].ex_low >> 1;
        end else if (current_criticality >= transition_nums[0]) begin
            ex_limit = (ready_queue[0].ex_low >> 1) + (ready_queue[0].ex_low >> 2);
        end else begin
            ex_limit = ready_queue[0].ex_low;
        end
    end else if (ready_queue[0].criticality == HIGH_LOW_MODE) begin
        ex_limit = ready_queue[0].ex_low;
    end else if (ready_queue[0].criticality == HIGH_HIGH_MODE) begin
        ex_limit = ready_queue[0].ex_high;
    end else begin
        ex_limit = ready_queue[0].ex_low;
    end

    // Handle removal of tasks from queue
    if (completion_valid || (running_valid && current_time >= ready_queue[0].absolute_deadline)) begin
        if (completion_succesful || (running_valid && current_time >= ready_queue[0].absolute_deadline)) begin
            n_idle_table[running_task].valid = `TRUE;
            n_idle_table[running_task].criticality = ready_queue[0].criticality;
            n_ready_queue[0].valid = `FALSE;
            if (ready_queue[0].task_type == PERIODIC) begin
                n_idle_table[running_task].next_wakeup = idle_table[running_task].next_wakeup + idle_table[running_task].period;
            end
        end else begin
            n_blocked_table[running_task].valid = `TRUE;
            n_blocked_table[running_task].criticality = ready_queue[0].criticality;
            n_blocked_table[running_task].absolute_deadline = ready_queue[0].absolute_deadline;
            n_blocked_table[running_task].ex_time = ready_queue[0].ex_time;
            if (ready_queue[0].task_type == PERIODIC) begin
                n_idle_table[running_task].next_wakeup = idle_table[running_task].next_wakeup + idle_table[running_task].period;
            end
        end
    end else if (running_valid && ready_queue[0].ex_time >= ex_limit) begin
        if (ready_queue[0].criticality == HIGH_LOW_MODE) begin
            n_ready_queue[0].criticality = HIGH_HIGH_MODE;
            criticality_transition = `TRUE;
        end else begin
            pop_valid = `TRUE;
            n_idle_table[running_task].valid = `TRUE;
            n_idle_table[running_task].criticality = ready_queue[0].criticality;
            n_ready_queue[0].valid = `FALSE;
            if (ready_queue[0].task_type == PERIODIC) begin
                n_idle_table[running_task].next_wakeup = idle_table[running_task].next_wakeup + idle_table[running_task].period;
            end
        end
    end else if (running_valid) begin 
        n_ready_queue[0].ex_time = ready_queue[0].ex_time + 1;
    end else begin
        for (int unsigned i = 0; i < `MAX_TASKS; ++i) begin
            if (idle_table[i].valid && idle_table[i].task_type == PERIODIC && idle_table[i].next_wakeup <= current_time &&
                ~(current_criticality >= transition_nums[3] && idle_table[i].criticality == LOW)) begin
                queue_insert_task.valid = `TRUE;
                queue_insert_task.id = i;
                queue_insert_task.task_type = idle_table[i].task_type;
                queue_insert_task.criticality = idle_table[i].criticality;
                queue_insert_task.ex_high = idle_table[i].ex_high;
                queue_insert_task.ex_low = idle_table[i].ex_low;
                queue_insert_task.absolute_deadline = idle_table[i].next_wakeup + idle_table[i].deadline;
                queue_insert_task.ex_time = 0;
                break;
            end
        end
    end

    // Actually add or remove from queue
    if (queue_insert_task.valid && pop_valid) begin
        for (int i = 1; i < `MAX_TASKS; ++i) begin
            if (~ready_queue[i].valid || ready_queue[i].absolute_deadline > queue_insert_task.absolute_deadline) begin
                n_ready_queue[i - 1] = queue_insert_task;
                break;
            end
            n_ready_queue[i - 1] = ready_queue[i];
        end
    end else if (queue_insert_task.valid) begin
        for (int i = `MAX_TASKS - 1; i >= 0; --i) begin
            if (i == 0) begin
                n_ready_queue[i] = queue_insert_task;
                break;
            end
            if (ready_queue[i - 1].valid && ready_queue[i - 1].absolute_deadline <= queue_insert_task.absolute_deadline) begin
                n_ready_queue[i] = queue_insert_task;
                break;
            end
            n_ready_queue[i] = ready_queue[i - 1];
        end
    end else if (pop_valid) begin
        for (int i = 1; i < `MAX_TASKS; ++i) begin
            n_ready_queue[i - 1] = ready_queue[i];
        end
        n_ready_queue[`MAX_TASKS-1].valid = `FALSE;
    end
end

always_ff @(posedge clk) begin
    if (rst) begin
        idle_table <= 0;
        blocked_table <= 0;
        ready_queue <= 0;
        current_criticality <= 0;
        current_time <= 0;
    end else begin 
        idle_table <= n_idle_table;
        blocked_table <= n_blocked_table;
        ready_queue <= n_ready_queue;

        if (criticality_transition) begin
            current_criticality <= current_criticality + 1;
        end else if (~cpu_running) begin 
            current_criticality <= 0;
        end

        if (current_time[`TIME_BITS-1]) begin
            current_time <= 0;
        end else begin 
            current_time <= current_time + 1;
        end
    end
end

endmodule
