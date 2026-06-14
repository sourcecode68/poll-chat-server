client: client.o
	$(CXX) $(CXXFLAGS) -o $@ $^
server: server.o
	$(CXX) $(CXXFLAGS) -o $@ $^

client.o: client.cpp
	$(CXX) $(CXXFLAGS) -c $<
server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean run_server run_client
clean:
	rm -f client server *.o
run_server: server
	./server
run_client: client
	./client 127.0.0.1
