EXEC = mrdd

all: $(EXEC)

mrdd: mrdd.o

clean:
	$(RM) $(EXEC)

distclean: clean
	$(RM) *.o *~
