//Verilog HDL for "DIGITAL", "SKELETON", "functional"
`define MAX_TASKS 32

typedef enum logic [1:0] {
	PERIODIC = 2'h0,
	SPORADIC  = 2'h1,
    INTERRUPT = 2'h2
} TASK_TYPE;

module SKELETON (
    input clk,
    input rst,
    output reg led
);

reg [3:0]count;

always @(posedge clk) begin
    if (rst) begin
        count <= 0;
        led <= 0;
    end else if(count == 'hF) begin //Time is up
        count <= 0;             //Reset count register
        led <= ~led;            //Toggle led (in each second)
    end else begin
        count <= count + 1;     //Counts 100MHz clock
        end

end

endmodule
