EXEC = mrdd
CFLAGS = -Og -g

all: $(EXEC)

mrdd: mrdd.o

clean:
	$(RM) $(EXEC)

distclean: clean
	$(RM) *.o *~
