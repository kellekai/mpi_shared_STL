#include "ContainerRDMA.h"
    
template <typename C>
ContainerRDMA<C>::ContainerRDMA( MPI_Aint size, size_t disp_unit, MPI_Comm comm ) 
  : m_size(size), m_disp_unit(disp_unit), m_comm(comm), m_info(MPI_INFO_NULL), m_container{}  
{
  MPI_Comm_rank( m_comm, &m_comm_rank ); 
  MPI_Comm_size( m_comm, &m_comm_size ); 
}

  template <typename C>
ContainerRDMA<C>::ContainerRDMA() 
  : m_size(0), m_disp_unit(1), m_comm(MPI_COMM_SELF), m_info(MPI_INFO_NULL), m_container{}  
{
}

template <typename C>
C & ContainerRDMA<C>::get() {
  MPI_Win_lock( MPI_LOCK_EXCLUSIVE, __m_master, 0, m_window );
  MPI_Get( &m_container_size, __m_meta_size, MPI_CHAR, __m_master, 0, 
      __m_meta_size, MPI_CHAR, m_window);
  MPI_Win_flush(__m_master, m_window);
  char* buffer = new char[m_container_size];
  assert( buffer != NULL && "could not allocate memory" );
  MPI_Get( buffer, m_container_size, 
      MPI_CHAR, __m_master, __m_meta_size, m_container_size, 
      MPI_CHAR, m_window); 
  MPI_Win_unlock( __m_master, m_window );
  std::string buffer_str(buffer, buffer+m_container_size);
  deserialize_win( buffer_str );
  delete buffer;
  return m_container;
}

template <typename C>
void ContainerRDMA<C>::put( const C & cont ) {
  m_container = cont;
  std::string buffer = serialize_win();
  m_container_size = buffer.size();
  MPI_Win_lock( MPI_LOCK_EXCLUSIVE, __m_master, 0, m_window );
  MPI_Put( &m_container_size, __m_meta_size, MPI_CHAR, __m_master, 0,    
      __m_meta_size, MPI_CHAR, m_window); 
  MPI_Put( buffer.data(), m_container_size, 
      MPI_CHAR, __m_master, __m_meta_size, m_container_size, 
      MPI_CHAR, m_window); 
  MPI_Win_unlock( __m_master, m_window );
}

template <typename C>
void ContainerRDMA<C>::create( MPI_Aint size, size_t disp_unit, MPI_Comm comm ) 
{
  m_comm = comm;
  m_size = size;
  m_disp_unit = disp_unit;
  MPI_Comm_rank( m_comm, &m_comm_rank ); 
  MPI_Comm_size( m_comm, &m_comm_size );
  if(m_comm_rank == __m_master) {  
    MPI_Aint size = m_size*m_disp_unit;
    MPI_Aint extended_size = static_cast<MPI_Aint>((__m_memory_factor+1.0F)*size); 
    MPI_Alloc_mem( extended_size, MPI_INFO_NULL, &m_base );
    MPI_Win_create( m_base, extended_size, 1, m_info, m_comm, &m_window ); 
    // put empty map in window
//    m_container[1] = 100;
//    m_container[2] = 200;
//    m_container[3] = 300;
//    m_container[4] = 400;
    std::string buffer = serialize_win();
    m_container_size = buffer.size();
    MPI_Win_lock( MPI_LOCK_EXCLUSIVE, __m_master, 0, m_window );
    MPI_Put( &m_container_size, __m_meta_size, MPI_CHAR, __m_master, 0,    
        __m_meta_size, MPI_CHAR, m_window); 
    MPI_Put( buffer.data(), m_container_size, 
        MPI_CHAR, __m_master, __m_meta_size, m_container_size, 
        MPI_CHAR, m_window); 
    MPI_Win_unlock( __m_master, m_window );
  } else {
    MPI_Win_create( nullptr, 0, 1, m_info, m_comm, &m_window ); 
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

template <typename C>
void ContainerRDMA<C>::create() 
{
  if(m_comm_rank == __m_master) {  
    MPI_Aint size = m_size*m_disp_unit;
    MPI_Aint extended_size = static_cast<MPI_Aint>((__m_memory_factor+1.0F)*size); 
    MPI_Alloc_mem( extended_size, MPI_INFO_NULL, &m_base );
    MPI_Win_create( m_base, extended_size, 1, m_info, m_comm, &m_window ); 
    // put empty map in window
//    m_container[1] = 100;
//    m_container[2] = 200;
//    m_container[3] = 300;
//    m_container[4] = 400;
    std::string buffer = serialize_win();
    m_container_size = buffer.size();
    MPI_Win_lock( MPI_LOCK_EXCLUSIVE, __m_master, 0, m_window );
    MPI_Put( &m_container_size, __m_meta_size, MPI_CHAR, __m_master, 0,    
        __m_meta_size, MPI_CHAR, m_window); 
    MPI_Put( buffer.data(), m_container_size, 
        MPI_CHAR, __m_master, __m_meta_size, m_container_size, 
        MPI_CHAR, m_window); 
    MPI_Win_unlock( __m_master, m_window );
  } else {
    MPI_Win_create( nullptr, 0, 1, m_info, m_comm, &m_window ); 
  }
  MPI_Barrier(MPI_COMM_WORLD);
}

template <typename C>
void ContainerRDMA<C>::destroy() 
{
  MPI_Barrier(MPI_COMM_WORLD);
  if( m_comm_rank == 0 ) {
    MPI_Free_mem( m_base );
  }
  MPI_Win_free( &m_window );
}

template <typename C>
std::string ContainerRDMA<C>::serialize_win() {
  std::ostringstream oss;
  {
    boost::archive::binary_oarchive oa(oss, __m_flags);
    oa << m_container;
  }
  return oss.str();
}

template <typename C>
void ContainerRDMA<C>::deserialize_win( const std::string ar ) {
  std::istringstream iss(ar);
  {
    boost::archive::binary_iarchive ia(iss, __m_flags);
    ia >> m_container;
  }
}

template class ContainerRDMA<std::set<int64_t>>;
template class ContainerRDMA<std::map<int,int64_t>>;
template class ContainerRDMA<std::map<int,int>>;
template class ContainerRDMA<std::vector<double>>;

