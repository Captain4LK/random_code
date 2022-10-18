rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CC = gcc
LD = gcc

SOURCES += $(call rwildcard,./,*.c)

LIBS = -lpthread -lwiringPi -lm

CPPFLAGS = -MD -MP -Wno-write-strings -O3 -Wall -Wextra -std=c99 -pedantic

plotter: $(SOURCES:%.c=%.o)
	$(LD) -o $@ $^ $(LIBS)

clean:
	rm $(SOURCES:%.c=%.o)

-include $(SOURCES:%.c=%.d)

