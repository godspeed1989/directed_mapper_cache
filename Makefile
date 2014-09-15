TARGET=dmcache

.PHONY: all
all: vpi compile run

.PHONY: vpi
vpi: test.c
	iverilog-vpi $+

.PHONY: compile
compile:
	iverilog -o$(TARGET).vvp main.v

.PHONY: run
run:
	vvp -M. -mtest $(TARGET).vvp
.PHONY: clean

clean:
	rm -f *.vvp *.vpi *.o
