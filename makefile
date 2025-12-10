PROJECT = my-redis

COMPILER = g++

BUILDDIR = ./build/
FLAGS = -Wall -Wextra

my-redis: main.o
	$(COMPILER) $(BUILDDIR)*.o -o $(BUILDDIR)$(PROJECT)


main.o: main.cpp
	$(COMPILER) $(FLAGS) -c main.cpp -o $(BUILDDIR)main.o

clean:
	@echo "Cleaning.."
	rm -f $(BUILDDIR)*.o
	rm -f $(BUILDDIR)$(PROJECT)
	@echo "Done cleaning!"

run:
	make
	$(BUILDDIR)$(PROJECT)
