
RME_OBJ := rme.o
RME_BIN = rme.a

RME_DEPS := $(RME_OBJ:%.o=%.dep)
RME_DEPS := $(filter %.dep,$(RME_DEPS))

CFLAGS = -Wall
CPPFLAGS = -I . -I ../testkern

all: $(RME_BIN)

clean:
	$(RM) $(RME_BIN) $(RME_OBJ) $(RME_DEPS)

$(RME_BIN): $(RME_OBJ)
	$(AR) cur $(RME_BIN) $(RME_OBJ)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS) -MD -MT $@ -MF $*.dep

-include $(RME_DEPS)
