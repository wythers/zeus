test: cache_test
	@./cache_test

cache_test:
	@g++ -std=c++20 -Wall -latomic -pthread -o cache_test cache_test.cpp

.PHONY: clean
clean:
	@rm cache_test