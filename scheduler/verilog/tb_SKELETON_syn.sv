`timescale 1ns / 1ps
`include "SKELETON_syn.v"

module tb_SKELETON;

    logic clock;
    logic reset;
    logic led;

    SKELETON skl(.clk(clock), .rst(reset), .led);

    always begin
		#10;
		clock = ~clock;
	end

    initial begin
        $monitor("Reset %b | Clock: %b | LED: %b", reset, clock, led);
        reset = 1;
        clock = 0;
        @(negedge clock);
        @(negedge clock);
        reset = 0;

        for (int i = 0; i < 100; i++) begin
			      @(negedge clock);
		    end
        $finish;
    end
endmodule
