# What is it?

A simple shared RDMA container with synchronized access. The intention is to provide C++ STL containers which are 
shared between the ranks of an MPI communicator. Consistency of the shared container is not guaranteed. The accesses
need to be synchronized appropriately.

# Example

```C
int main() {
  MPI_Init(NULL,NULL);
  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  std::set<int64_t> test_set;

  /*--------------------------------------------------------------*/ 
  /* Define shared set                                            */
  /*--------------------------------------------------------------*/
  ContainerRDMA< std::set<int64_t> > cont_set;
  
  /*--------------------------------------------------------------*/
  /* Create container with size 'SIZE' on MPI_COMM_WORLD          */
  /*--------------------------------------------------------------*/
  cont_set.create( SIZE, sizeof(std::set<int64_t>), MPI_COMM_WORLD );
  
  // Manipulate local copy of set
  for(int t = 0; t<60; t++) {
    if( rank == 0 ) {
      for(int i = 1; i<size; i++) {
        io_state_id_t state_new(t, i);
        io_state_id_t state_old(t-1, i);
        test_set.insert(to_ckpt_id(state_new));
        if( t>0 ) {
          test_set.erase(to_ckpt_id(state_old));  
        }
      }  
      /*--------------------------------------------------------------*/
      /* Update shared set by local copy                              */
      /*--------------------------------------------------------------*/
      cont_set.put(test_set);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    try {
      /*--------------------------------------------------------------*/
      /* Get copy of shared set                                       */
      /*--------------------------------------------------------------*/
      test_set = cont_set.get();
    } catch ( ... ) {
      std::cout << "exeption caught" << std::endl;
    }
    if( rank != 0 ) {
      io_state_id_t state_new(t, rank);
      io_state_id_t state_old(t-1, rank);
      bool exists_new = test_set.find(to_ckpt_id(state_new)) != test_set.end();
      std::cout << "cp{t:" << t << "|id:" << rank << "} exists? " << exists_new << std::endl;
      if (t>0) {
        bool exists_old = test_set.find(to_ckpt_id(state_old)) != test_set.end();
        std::cout << "cp{t:" << t-1 << "|id:" << rank << "} exists? " << exists_old << std::endl;
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
  /*--------------------------------------------------------------*/
  /* Free memory and destroy RDMA container                       */
  /*--------------------------------------------------------------*/
  cont_set.destroy();

  MPI_Finalize();
  return 0;
}
```
