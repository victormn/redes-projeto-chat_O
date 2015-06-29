all: 
	@g++ -o chat ChatO.cpp -g -std=c++11 -pthread

clean:
	@rm -f *.o chat
	@find -name "*~" | xargs rm -rf

memorycheck:
	@valgrind --track-origins=yes ./chat

run:
	@./chat