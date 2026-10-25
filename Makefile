CC = gcc

CFLAGS = -O \
        -fstack-protector-strong \
        -fstrict-flex-arrays=3 \
        -fno-strict-overflow \
        -fno-delete-null-pointer-checks \
        -fcf-protection=full \
        -Wall -Wextra -Wformat -Wformat-security \
        -fPIE

LDFLAGS = -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack

TARGET = NIXOS_DEPLOYMENT
TEST_TARGET = NIXOS_DEPLOYMENT_TEST

SRC = config_parser.c filesystem_operations.c config_paths.c compile_projects.c deploy_pkgs.c recreate_dir.c main.c
OBJ = $(SRC:.c=.o)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Testversion mit -D_TEST_BUILD
test: CFLAGS += -D_TEST_BUILD
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

# Aufräumen
clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)
