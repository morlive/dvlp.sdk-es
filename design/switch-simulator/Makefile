# Makefile для проекта switch-simulator
# ----------------------------------------------------
# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -I./include -I./drivers/include -I./bsp/include
SRC_DIR = src
OBJ_DIR_DEBUG = ./build
OBJ_DIR_CORE = ./build/core
TARGET_DIR = ./bin

# Выходные исполняемые файлы
SWITCH_SIM = switch-simulator
CLI_TOOL = switch-cli
SAI_TOOL = sai-tool
NETWORK_SIM = network-simulator

# Объектные файлы для основной программы
SWITCH_SIM_OBJS = \
	$(OBJ_DIR_CORE)/main.o \
	$(OBJ_DIR_CORE)/common/logging.o \
	$(OBJ_DIR_CORE)/common/utils.o \
	$(OBJ_DIR_CORE)/hal/hw_simulation.o \
	$(OBJ_DIR_CORE)/hal/packet.o \
	$(OBJ_DIR_CORE)/hal/port.o \
	$(OBJ_DIR_CORE)/l2/mac_learning.o \
	$(OBJ_DIR_CORE)/l2/mac_table.o \
	$(OBJ_DIR_CORE)/l2/stp.o \
	$(OBJ_DIR_CORE)/l2/vlan.o \
	$(OBJ_DIR_CORE)/l3/arp.o \
	$(OBJ_DIR_CORE)/l3/ip_processing.o \
	$(OBJ_DIR_CORE)/l3/routing_table.o \
	$(OBJ_DIR_CORE)/l3/routing_protocols/ospf.o \
	$(OBJ_DIR_CORE)/l3/routing_protocols/rip.o \
	$(OBJ_DIR_CORE)/management/cli_engine.o \
	$(OBJ_DIR_CORE)/management/config_manager.o \
	$(OBJ_DIR_CORE)/management/stats_collector.o \
	$(OBJ_DIR_CORE)/sai/sai_adapter.o \
	$(OBJ_DIR_CORE)/sai/sai_port.o \
	$(OBJ_DIR_CORE)/sai/sai_route.o \
	$(OBJ_DIR_CORE)/sai/sai_vlan.o \
	$(OBJ_DIR_CORE)/bsp/bsp_config.o \
	$(OBJ_DIR_CORE)/bsp/bsp_drivers.o \
	$(OBJ_DIR_CORE)/bsp/bsp_init.o \
	$(OBJ_DIR_CORE)/drivers/ethernet_driver.o \
	$(OBJ_DIR_CORE)/drivers/sim_driver.o

# Объектные файлы для CLI инструмента
CLI_TOOL_OBJS = \
	$(OBJ_DIR_CORE)/management/cli_engine.o \
	$(OBJ_DIR_CORE)/common/logging.o \
	$(OBJ_DIR_CORE)/common/utils.o

# Объектные файлы для SAI инструмента
SAI_TOOL_OBJS = \
	$(OBJ_DIR_CORE)/sai/sai_adapter.o \
	$(OBJ_DIR_CORE)/sai/sai_port.o \
	$(OBJ_DIR_CORE)/sai/sai_route.o \
	$(OBJ_DIR_CORE)/sai/sai_vlan.o \
	$(OBJ_DIR_CORE)/common/logging.o

# Объектные файлы для сетевого симулятора
NETWORK_SIM_OBJS = \
	$(OBJ_DIR_CORE)/tools/traffic_generator.o \
	$(OBJ_DIR_CORE)/common/logging.o

# Цели по умолчанию
all: $(SWITCH_SIM) $(CLI_TOOL) $(SAI_TOOL) $(NETWORK_SIM)

# Правила создания исполняемых файлов
# ------------------------------
$(SWITCH_SIM): $(SWITCH_SIM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

$(CLI_TOOL): $(CLI_TOOL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SAI_TOOL): $(SAI_TOOL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(NETWORK_SIM): $(NETWORK_SIM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правила компиляции для src каталога
# ----------------------
$(OBJ_DIR_CORE)/main.o: $(SRC_DIR)/main.c
	@mkdir -p $(OBJ_DIR_CORE)
	$(CC) $(CFLAGS) -c $< -o $@

# Common
$(OBJ_DIR_CORE)/common/logging.o: $(SRC_DIR)/common/logging.c
	@mkdir -p $(OBJ_DIR_CORE)/common
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/common/utils.o: $(SRC_DIR)/common/utils.c
	@mkdir -p $(OBJ_DIR_CORE)/common
	$(CC) $(CFLAGS) -c $< -o $@

# HAL
$(OBJ_DIR_CORE)/hal/hw_simulation.o: $(SRC_DIR)/hal/hw_simulation.c
	@mkdir -p $(OBJ_DIR_CORE)/hal
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/hal/packet.o: $(SRC_DIR)/hal/packet.c
	@mkdir -p $(OBJ_DIR_CORE)/hal
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/hal/port.o: $(SRC_DIR)/hal/port.c
	@mkdir -p $(OBJ_DIR_CORE)/hal
	$(CC) $(CFLAGS) -c $< -o $@

# L2
$(OBJ_DIR_CORE)/l2/mac_learning.o: $(SRC_DIR)/l2/mac_learning.c
	@mkdir -p $(OBJ_DIR_CORE)/l2
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l2/mac_table.o: $(SRC_DIR)/l2/mac_table.c
	@mkdir -p $(OBJ_DIR_CORE)/l2
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l2/stp.o: $(SRC_DIR)/l2/stp.c
	@mkdir -p $(OBJ_DIR_CORE)/l2
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l2/vlan.o: $(SRC_DIR)/l2/vlan.c
	@mkdir -p $(OBJ_DIR_CORE)/l2
	$(CC) $(CFLAGS) -c $< -o $@

# L3
$(OBJ_DIR_CORE)/l3/arp.o: $(SRC_DIR)/l3/arp.c
	@mkdir -p $(OBJ_DIR_CORE)/l3
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l3/ip_processing.o: $(SRC_DIR)/l3/ip_processing.c
	@mkdir -p $(OBJ_DIR_CORE)/l3
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l3/routing_table.o: $(SRC_DIR)/l3/routing_table.c
	@mkdir -p $(OBJ_DIR_CORE)/l3
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l3/routing_protocols/ospf.o: $(SRC_DIR)/l3/routing_protocols/ospf.c
	@mkdir -p $(OBJ_DIR_CORE)/l3/routing_protocols
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/l3/routing_protocols/rip.o: $(SRC_DIR)/l3/routing_protocols/rip.c
	@mkdir -p $(OBJ_DIR_CORE)/l3/routing_protocols
	$(CC) $(CFLAGS) -c $< -o $@

# Management
$(OBJ_DIR_CORE)/management/cli_engine.o: $(SRC_DIR)/management/cli_engine.c
	@mkdir -p $(OBJ_DIR_CORE)/management
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/management/config_manager.o: $(SRC_DIR)/management/config_manager.c
	@mkdir -p $(OBJ_DIR_CORE)/management
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/management/stats_collector.o: $(SRC_DIR)/management/stats_collector.c
	@mkdir -p $(OBJ_DIR_CORE)/management
	$(CC) $(CFLAGS) -c $< -o $@

# SAI
$(OBJ_DIR_CORE)/sai/sai_adapter.o: $(SRC_DIR)/sai/sai_adapter.c
	@mkdir -p $(OBJ_DIR_CORE)/sai
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/sai/sai_port.o: $(SRC_DIR)/sai/sai_port.c
	@mkdir -p $(OBJ_DIR_CORE)/sai
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/sai/sai_route.o: $(SRC_DIR)/sai/sai_route.c
	@mkdir -p $(OBJ_DIR_CORE)/sai
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/sai/sai_vlan.o: $(SRC_DIR)/sai/sai_vlan.c
	@mkdir -p $(OBJ_DIR_CORE)/sai
	$(CC) $(CFLAGS) -c $< -o $@

# BSP
$(OBJ_DIR_CORE)/bsp/bsp_config.o: bsp/src/bsp_config.c
	@mkdir -p $(OBJ_DIR_CORE)/bsp
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/bsp/bsp_drivers.o: bsp/src/bsp_drivers.c
	@mkdir -p $(OBJ_DIR_CORE)/bsp
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/bsp/bsp_init.o: bsp/src/bsp_init.c
	@mkdir -p $(OBJ_DIR_CORE)/bsp
	$(CC) $(CFLAGS) -c $< -o $@

# Drivers
$(OBJ_DIR_CORE)/drivers/ethernet_driver.o: drivers/src/ethernet_driver.c
	@mkdir -p $(OBJ_DIR_CORE)/drivers
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR_CORE)/drivers/sim_driver.o: drivers/src/sim_driver.c
	@mkdir -p $(OBJ_DIR_CORE)/drivers
	$(CC) $(CFLAGS) -c $< -o $@

# Tools
$(OBJ_DIR_CORE)/tools/traffic_generator.o: tools/simulators/traffic_generator.c
	@mkdir -p $(OBJ_DIR_CORE)/tools
	$(CC) $(CFLAGS) -c $< -o $@

# Установка символических ссылок в директории bin
.PHONY: install
install: $(SWITCH_SIM) $(CLI_TOOL) $(SAI_TOOL) $(NETWORK_SIM)

ifndef TARGET_DIR
    TARGET_DIR = ./bin
    $(warning TARGET_DIR не указан. Будет использована локальная директория ./bin. Можно использовать 'make install TARGET_DIR=<путь> для установки в другую директорию)
endif
# ifndef TARGET_DIR
#   $(error TARGET_DIR не указан. Используйте 'make install TARGET_DIR=./bin')
# endif
	@echo "Создание символических ссылок в директории bin"
	@mkdir -p $(TARGET_DIR)
	ln -sf $(CURDIR)/$(SWITCH_SIM)      $(TARGET_DIR)/$(SWITCH_SIM)
	ln -sf $(CURDIR)/$(CLI_TOOL)        $(TARGET_DIR)/$(CLI_TOOL)
	ln -sf $(CURDIR)/$(SAI_TOOL)        $(TARGET_DIR)/$(SAI_TOOL)
	ln -sf $(CURDIR)/$(NETWORK_SIM)     $(TARGET_DIR)/$(NETWORK_SIM)


# Очистка промежуточных файлов
.PHONY: clean
clean:
	rm -f $(OBJ_DIR_CORE)/*.o $(OBJ_DIR_CORE)/*/*.o $(OBJ_DIR_CORE)/*/*/*.o
	rm -f $(SWITCH_SIM) $(CLI_TOOL) $(SAI_TOOL) $(NETWORK_SIM)
	rmdir --ignore-fail-on-non-empty $(OBJ_DIR_CORE)/*/*/ $(OBJ_DIR_CORE)/*/ $(OBJ_DIR_CORE)/ $(OBJ_DIR_DEBUG)/
	@echo "Удаление символических ссылок из директории bin:"
	rm -f $(TARGET_DIR)/$(SWITCH_SIM)
	rm -f $(TARGET_DIR)/$(CLI_TOOL)
	rm -f $(TARGET_DIR)/$(SAI_TOOL)
	rm -f $(TARGET_DIR)/$(NETWORK_SIM)

# Тестирование
.PHONY: test
test:
	$(MAKE) -C tests

# Цель для запуска всех тестов
.PHONY: tests
tests: test
