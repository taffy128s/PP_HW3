CC = mpicc
CXX = mpicxx
CFLAGS = -O3 -std=gnu99
CXXFLAGS = -O3 -std=gnu++11

EXE1 = APSP_Pthread
EXE2 = APSP_MPI_sync
EXE3 = APSP_MPI_async
EXE4 = APSP_Hybrid

.PHONY: all
all:
	$(CXX) $(EXE1).cc $(CXXFLAGS) -o $(EXE1)
	$(CXX) $(EXE2).cc $(CXXFLAGS) -o $(EXE2)
	$(CXX) $(EXE3).cc $(CXXFLAGS) -o $(EXE3)
	$(CXX) $(EXE4).cc $(CXXFLAGS) -o $(EXE4)
  
.PHONY: clean
clean: 
	rm -f $(EXE1) $(EXE2) $(EXE3) $(EXE4)
