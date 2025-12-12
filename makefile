PROJECT = my-redis

COMPILER = g++

BUILDDIR = ./build/
FLAGS = -std=c++17 -Wall -Wextra

my-redis: main.o
	$(COMPILER) $(BUILDDIR)*.o -o $(BUILDDIR)$(PROJECT)

main.o: main.cpp utils.o context.o
	$(COMPILER) $(FLAGS) -c main.cpp -o $(BUILDDIR)main.o

utils.o: ./utils/utils.*
	$(COMPILER) $(FLAGS) -c ./utils/utils.cpp -o $(BUILDDIR)utils.o

context.o: context.*
	$(COMPILER) $(FLAGS) -c context.cpp -o $(BUILDDIR)context.o

clean:
	@echo "| Cleaning.."
	rm -f $(BUILDDIR)*.o
	rm -f $(BUILDDIR)$(PROJECT)
	@echo "| Done cleaning!"

run:
	@echo " "
	@echo "| Building executable..."
	@echo " "
	make
	@echo " "
	@echo "| Running executable..."
	@echo " "
	$(BUILDDIR)$(PROJECT)

