#ifndef CTaskDivider_HPP
#define CTaskDivider_HPP

#include <assert.h>
/* For now this only works for two tasks */

class CTaskDivider{

   private:
   const int nTasks;
   std::vector<int> nRanksPerTask, partners; 
   int mycolor;
   MPI_Comm comm_color;
   int root_color[1], size_comm_color;

   public:
   CTaskDivider( ) = delete;
   CTaskDivider( const int nTasks_, std::vector<int>&& nRanksPerTask_ ) // this is r-value since no type deduction.
   : nTasks(nTasks_), nRanksPerTask(std::move(nRanksPerTask_))   // move needs to be applied both places! - at caller inside callee or if provided as r-value
   { 
     int nRanks;
     int whoami;
     CMPIWrapper::Comm_size(MPI_COMM_WORLD, &nRanks);
     CMPIWrapper::Comm_rank(MPI_COMM_WORLD, &whoami);

     int sum_of_ranks = 0;

     if( nRanksPerTask[0] == -1 )
     {
        nRanksPerTask[0] = nRanks - nRanksPerTask[1];
     }

     if( nRanksPerTask[1] == -1 )
     {
        nRanksPerTask[1] = nRanks - nRanksPerTask[0];
     }

     for( auto& n:nRanksPerTask )
        sum_of_ranks +=n; 

     if( sum_of_ranks != nRanks )CMPIWrapper::Abort(MPI_COMM_WORLD
                                 , "Dividing tasks to ranks do not add up!");

     mycolor = 0;
     if( whoami >= (nRanks-nRanksPerTask[1]) )mycolor = 1;
     
     partners.resize(GetNPartners());
     
     int start_id = 0;
     int npartners =0;
     if( mycolor == 1 )
     { 
        start_id   = 0;
        npartners  = nRanksPerTask[0]; 
     }
     else if( mycolor == 0 )
     {
       start_id = nRanksPerTask[0];
       npartners  = nRanksPerTask[1]; 
     }

      for( int i = 0; i < npartners; i++ ){
         partners[i]=(start_id+i); 
      }

// create a mpi communicator based on mycolor;

      CMPIWrapper::Comm_split(MPI_COMM_WORLD, mycolor, 0, &comm_color);

      MPI_Group color_group;
      MPI_Group world_group;
      CMPIWrapper::Comm_group(comm_color, &color_group);
      CMPIWrapper::Comm_group(MPI_COMM_WORLD, &world_group);
      
      int local_root[1];
      local_root[0] = 0;
      root_color[0] = -1;
      CMPIWrapper::Group_translate_ranks( color_group, 1, local_root
                                        , world_group, root_color);
      CMPIWrapper::Comm_size(comm_color, &size_comm_color);
      return;
   };

   int GetNPartners( void )const
   {
     if( mycolor == 0 )return nRanksPerTask[1];
     if( mycolor == 1 )return nRanksPerTask[0];
     return -1;
   };

   const std::vector<int>& GetAllPartnerRanks( void )const
   {
     return partners;
   };

   inline int GetPartnerRank( const int ipart )const
   {
     return partners[ipart];
   };

   int GetColor( void )const
   {
      return mycolor;
   }

   int GetnTasks(void )const
   {
      return nTasks;
   }
   int GetColRoot( void )const 
   {
      return root_color[0];
   }
   int GetColSize( void )const 
   {
      return size_comm_color;
   }
   const MPI_Comm& GetColComm( void )const
   {
      return comm_color;
   } 
   
};

#endif
