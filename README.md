# SharedMemoryParallelisation
A simple code that compares the bandwidth using a shared memory MPI and point-to-point communication technique. It uses a task based parallel approach where ranks are coloured each representing a task. This is for now limited to two colours only (See CTaskDivider.hpp). 

In `main.cpp` you can specify how to distribute the ranks among the two colours. Now it is set to `initList = {size-1, 1}`, i.e. one rank for colour 1 and the rest ranks `size-1` are designated colour 0. 


Build by `mpic++ -std=c++17 -O2 main.cpp -a main.x`. 

Run by `mpirun -np X ./main.x` where `X` is the number of ranks. 


Program will write two files, namely the average communication time.
   1) `bandwidth_p2p.out`: Bandwidth Analysis for point-to-point communication
   2) `bandwidth_shm.out`: Bandwidth Analysis for Shared memory  communication
   
Files will have three columns:

   
   Average time among colour 1 ranks.                     Average time among colour 2 ranks.                              Message size.  
