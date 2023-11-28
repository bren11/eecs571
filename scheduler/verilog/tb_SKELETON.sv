`timescale 1ns / 1ps
`include "SKELETON.sv"

module tb_SKELETON;

    logic clk;
    logic rst;
    logic en;

    TASK_TABLE_INPUT input_task;
    logic [`MAX_TASK_BITS-1:0] input_task_id;

    logic wakeup_valid;
    logic [`MAX_TASK_BITS-1:0] wakeup_id;

    logic completion_valid;
    logic completion_succesful;

    logic [3:0][`MAX_TASK_BITS-1:0] transition_nums;

    logic [`MAX_TASK_BITS-1:0] running_task;
    logic [`MAX_TASK_BITS-1:0] next_task;

    logic running_valid;
    logic next_valid;

    logic cpu_interrupt;

    SKELETON skl(.clk, .rst, .en, .input_task, .wakeup_valid, .wakeup_id, .completion_valid,
    .completion_succesful, .transition_nums, .running_task, .next_task, .running_valid, .next_valid, .cpu_interrupt);

    always begin
		#1;
		clk = ~clk;
	end

    initial begin
        rst = 1;
        clk = 0;
        en = 1;

        input_task = 0;
        wakeup_valid = 0;
        wakeup_id = 0;
        completion_succesful = 0;
        completion_valid = 0;
        transition_nums[0] = 1;
        transition_nums[1] = 2;
        transition_nums[2] = 3;
        transition_nums[3] = 4;

        @(negedge clk);
        @(negedge clk);
        rst = 0;
        @(negedge clk);
        @(negedge clk);
        @(negedge clk);
        @(negedge clk);
        @(negedge clk);
        input_task.valid = 1;
        input_task.period = 20;
        input_task.ex_low = 5;
        @(negedge clk);
        input_task.period = 40;
        input_task.virtual_deadline = 30;
        input_task.criticality = HIGH_CRIT;
        input_task.ex_low = 5;
        input_task.ex_high = 10;
        @(negedge clk);
        input_task.valid = 0;
        for (int i = 0; i < 100; i++) begin
			@(negedge clk);
		end
        $finish;
    end
endmodule
