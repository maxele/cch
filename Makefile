BUILD := ./build/

help:
	@echo select what to build

$(BUILD)%.c.o: %.c
	mkdir -p $(dir $@)
	gcc -c -o $@ $<

S_SRC := server.c client_list.c msg_list.c
S_OBJ := $(S_SRC:%.c=$(BUILD)%.c.o)
C_SRC := client.c
C_OBJ := $(C_SRC:%.c=$(BUILD)%.c.o)

$(BUILD)server: $(S_OBJ)
	gcc -o $@ $(S_OBJ)
run_server: clear $(BUILD)server
	$(BUILD)server

$(BUILD)client: $(C_OBJ)
	gcc -o $@ $(C_OBJ)
run_client: clear $(BUILD)client
	sleep 0.1
	$(BUILD)client user1


.PHONY: clear clean
clear:
	@clear
clean:
	rm -r $(BUILD)
