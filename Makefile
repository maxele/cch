BUILD := ./build/

#DEBUG := -D__DEBUG__
#QUIET := -D__QUIET__

FLAGS := $(QUIET) $(DEBUG)

C_SRC := $(wildcard *.c)
C_OBJ := $(C_SRC:%.c=$(BUILD)%.c.o)

$(BUILD)cch: $(C_OBJ)
	mkdir -p $(dir $@)
	gcc -o $@ $(C_OBJ)

$(BUILD)%.c.o: %.c
	mkdir -p $(dir $@)
	gcc -c -o $@ $< $(FLAGS)

run_server: clear $(BUILD)cch
	$(BUILD)cch -s
run_client: clear $(BUILD)cch
	sleep 0.1
	$(BUILD)cch user1

run: clear $(BUILD)cch
	$(BUILD)cch user1

.PHONY: clear clean
clear:
	@clear
clean:
	rm -r $(BUILD)
