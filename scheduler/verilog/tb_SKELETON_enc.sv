`timescale 1ns / 1ps
`include "SKELETON_encounter.sv"

module tb_SKELETON();

    logic clk;
    logic reset;
    logic led;

    SKELETON skl(.clk, .reset, .led);

    always begin
		#10;
		clock = ~clock;
	end

    initial begin
        $monitor("Reset %b | Clock: %b | LED: %b", reset, clk, led);
        reset = 1;
        @(negedge clock);
        @(negedge clock);
        reset = 0;

        for (int i = 0; i < 100; i++) begin
			@(negedge clock);
		end
    end
endmodule
