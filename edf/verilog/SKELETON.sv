//Verilog HDL for "DIGITAL", "SKELETON", "functional"
`define MAX_TASKS 32
`define MAX_TASK_BITS $clog2(`MAX_TASKS)

`define MAX_UTIL_PRECISION 1024
`define MAX_UTIL_BITS $clog2(`MAX_UTIL_PRECISION)

`define TIME_BITS 8

`define FALSE  1'h0
`define TRUE  1'h1

typedef enum logic {
	PERIODIC = 1'h0,
	SPORADIC  = 1'h1
} TASK_TYPE;

typedef enum logic [1:0] {
	IDLE = 2'h0,
	BLOCKED = 2'h1,
    READY = 2'h2
} TASK_STATE;

typedef struct packed {
    logic valid;
    TASK_TYPE task_type;
    TASK_STATE state;
    logic [`TIME_BITS-1:0] period;
    logic [`TIME_BITS-1:0] ex_low;

    logic [`TIME_BITS-1:0] wakeup;
    logic [`TIME_BITS-1:0] absolute_deadline;
    logic [`TIME_BITS-1:0] ex_time;
} TASK_TABLE_ENTRY;

typedef struct packed {
    logic valid;
    logic [`MAX_TASK_BITS-1:0] id;
    TASK_TYPE task_type;
    logic [`TIME_BITS-1:0] period;
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

    output logic [`MAX_TASK_BITS-1:0] running_task,
    output logic [`MAX_TASK_BITS-1:0] next_task,

    output logic running_valid,
    output logic next_valid
);

logic [`TIME_BITS-1:0] current_time;

always_ff @(posedge clk) begin
    if (rst || current_time[`TIME_BITS-1]) begin
        current_time <= 0;
    end else begin 
        current_time <= current_time + 1;
    end
end

TASK_TABLE_ENTRY [`MAX_TASKS-1:0] task_table;
TASK_TABLE_ENTRY [`MAX_TASKS-1:0] n_task_table;

always_comb begin
    n_task_table = task_table;
    running_task = 0;
    next_task = 0;
    running_valid = `FALSE;
    next_valid = `FALSE;

    for (int i = 0; i < `MAX_TASKS; ++i) begin
        if (task_table[i].valid && task_table[i].state == READY) begin
            if (running_valid && task_table[i].absolute_deadline < task_table[running_task].absolute_deadline) begin
                next_valid = `TRUE;
                next_task = running_task;
                running_task = i;
            end else begin
                running_valid = `TRUE;
                running_task = i;
            end
        end
    end

    // Insert inputted task to table
    if (input_task.valid) begin 
        n_task_table[input_task.id].valid = `TRUE;
        n_task_table[input_task.id].task_type = input_task.task_type;
        n_task_table[input_task.id].state = IDLE;
        n_task_table[input_task.id].period = input_task.period;
        n_task_table[input_task.id].ex_low = input_task.ex_low;
        n_task_table[input_task.id].wakeup = 0;
        n_task_table[input_task.id].absolute_deadline = 0;
        n_task_table[input_task.id].ex_time = 0;
    end

    // Wakeup periodic tasks
    for (int unsigned i = 0; i < `MAX_TASKS; ++i) begin
        if (task_table[i].valid && task_table[i].task_type == PERIODIC && task_table[i].state == IDLE && task_table[i].wakeup <= current_time) begin
            if (task_table[i].task_type == PERIODIC) begin
                n_task_table[i].absolute_deadline = task_table[i].wakeup + task_table[i].period;
            end else begin
                n_task_table[i].absolute_deadline = current_time + task_table[i].period;
            end
            n_task_table[i].state = READY;
            n_task_table[i].ex_time = task_table[i].ex_time + 1;
        end
    end
    
    // wakeup blocked tasks or interrupts
    if (wakeup_valid && (task_table[wakeup_id].state == BLOCKED || task_table[wakeup_id].state == IDLE && task_table[wakeup_id].task_type == SPORADIC)) begin
        if (task_table[wakeup_id].task_type == PERIODIC) begin
            n_task_table[wakeup_id].absolute_deadline = task_table[wakeup_id].wakeup + task_table[wakeup_id].period;
        end else begin
            n_task_table[wakeup_id].absolute_deadline = current_time + task_table[wakeup_id].period;
        end
        n_task_table[wakeup_id].state = READY;
        n_task_table[wakeup_id].ex_time = task_table[wakeup_id].ex_time + 1;
    end

    // Handle removal of tasks from queue
    if ((completion_valid && completion_succesful) || 
        (running_valid && current_time >= task_table[running_task].absolute_deadline) ||
        (running_valid && task_table[running_task].ex_time >= task_table[running_task].ex_low)) begin
        n_task_table[running_task].state = IDLE;
        n_task_table[running_task].ex_time = 0;
        if (task_table[running_task].task_type == PERIODIC) begin
            n_task_table[running_task].wakeup = task_table[running_task].wakeup + task_table[running_task].period;
        end
    end else if (completion_valid && ~completion_succesful) begin 
        n_task_table[running_task].state = BLOCKED;
    end else if (running_valid) begin 
        n_task_table[running_task].ex_time = task_table[running_task].ex_time + 1;
    end

    if (current_time[`TIME_BITS-1]) begin
        for (int i = 0; i < `MAX_TASKS - 1; ++i) begin
            n_task_table[i].wakeup[`TIME_BITS - 1] = 0;
            n_task_table[i].absolute_deadline[`TIME_BITS - 1] = 0;
        end
    end
end

always_ff @(posedge clk) begin
    if (rst) begin
        task_table <= 0;
    end else if (en) begin 
        task_table <= n_task_table;
    end
end

endmodule
