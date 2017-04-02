EXEC = mrdisc
CFLAGS = -Og -g

all: $(EXEC)

mrdisc: mrdisc.o

clean:
	$(RM) $(EXEC)

distclean: clean
	$(RM) *.o *~
