#ifndef __ContainerRDMA__
#define __ContainerRDMA__

#include <mpi.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <cstring>

template <typename C>
class ContainerRDMA {

  public:
    
    ContainerRDMA( MPI_Aint size, size_t disp_unit, MPI_Comm comm );
    ContainerRDMA();
    void create( MPI_Aint size, size_t disp_unit, MPI_Comm comm );
    void create();
    void destroy();

    C & get();
    void put( const C & cont );
  
  private:
    
    C m_container;
    int64_t m_container_size;

    std::string serialize_win();
    void deserialize_win( const std::string ar );

    MPI_Comm      m_comm;
    MPI_Win       m_window;
    
    MPI_Info      m_info;
    MPI_Aint      m_size;
    size_t        m_disp_unit;
    void*         m_base;
    
    int m_comm_size;
    int m_comm_rank;
  
    const static size_t __m_meta_size = sizeof(int64_t);
    const static int __m_master = 0;
    constexpr static const double __m_memory_factor = 0.05; // factor additional memory for serialized map
    static auto const __m_flags = boost::archive::no_header | boost::archive::no_tracking;

};

#endif // __ContainerRDMA__

