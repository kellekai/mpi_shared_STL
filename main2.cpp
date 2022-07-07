#include <mpi.h>
#include "ContainerRDMA.h"
#include <set>

const MPI_Aint SIZE = 1024*256;

typedef int64_t io_id_t;

struct io_state_id_t {
  io_state_id_t( io_id_t _t, io_id_t _id ) : t(_t), id(_id), param(0) {}
  io_state_id_t( io_id_t _t, io_id_t _id, int p ) : t(_t), id(_id), param(p) {}
  io_state_id_t() : t(0), id(0), param(0) {}
	io_id_t t;
  io_id_t id;
  int param;
};

inline io_state_id_t to_state_id(const int64_t ckpt_id) {
  int64_t mask_param  = 0xFF;
  int64_t mask_t      = 0xFFFFFF;
  int64_t mask_id     = 0xFFFFFFFF;
  int64_t id          = ckpt_id & mask_id;
  int64_t t           = (ckpt_id >> 32) & mask_t;
  int64_t param       = (ckpt_id >> 56) & mask_param;
  return { t, id, param };
}

inline int64_t to_ckpt_id(io_state_id_t state_id) {
  int64_t ckpt_id = state_id.param;
  ckpt_id = (ckpt_id << 24) | state_id.t;
  ckpt_id = (ckpt_id << 32) | state_id.id;
  return ckpt_id;
}


int main() {
  MPI_Init(NULL,NULL);
  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  std::set<int64_t> test_set;

  ContainerRDMA< std::set<int64_t> > cont_set;
  cont_set.create( SIZE, sizeof(std::set<int64_t>), MPI_COMM_WORLD );
  
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
      cont_set.put(test_set);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    try {
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
  
  cont_set.destroy();

  MPI_Finalize();
  return 0;
}


