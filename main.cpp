#include <memory>
#include <iostream>
#include <fstream>
#include <iomanip>      // std::setprecision
#include <stdio.h>

#include "MPI_Cbindings.hpp"
#include "CDataExchange.hpp"
#include "CTaskDivider.hpp"
#include "CSharedMemoryComm.hpp"
#include "CP2PComm.hpp"

using namespace CMPIWrapper; 
using real = double;

int main( int argc, char *argv[])
{
   const int tag      = 13221345;
   const int ntasks   = 2;     //  max two for now.
   const int nrepeats = 10;    // We repeat the communication for each message to neglect noise.
/
   int whoami, size, ierr, root;
   std::vector<real> MBbytesArray;
   std::ofstream myfileshm;
   std::ofstream myfilep2p; 


   CMPIWrapper::Init(argc, argv);
   CMPIWrapper::Comm_rank(MPI_COMM_WORLD, &whoami);
   CMPIWrapper::Comm_size(MPI_COMM_WORLD, &size);
  
   root = 0;
   if( whoami == 0 )root = 1;

   if( root )
   {
      auto filep2p = "bandwidth_p2p_"+std::to_string(size)+".out";
      auto fileshm = "bandwidth_shm_"+std::to_string(size)+".out";
      myfileshm.open(fileshm);
      myfilep2p.open(filep2p);
      myfileshm << std::scientific << std::setprecision(8);
      myfilep2p << std::scientific << std::setprecision(8);
   }
   
// This initList will distribute the ranks to the two colors. 
// Below we asign all ranks-1 to color 0 and the last rank to color 1
   auto initList = {size-1, 1};

   auto TaskDivider = std::make_unique<const CTaskDivider>( ntasks, initList );

   auto mycolor    = TaskDivider->GetColor();
   auto com_color  = TaskDivider->GetColComm();
   auto root_color = TaskDivider->GetColRoot();
   auto size_color = TaskDivider->GetColSize();

 //  If you want to test data transfer only the one way, i.e. color 0 only sends 
 //  data. 
   real dum = 1.; 
/*   
   if( mycolor == 1 )dum = 0.;
*/

   MBbytesArray.push_back(0.1*dum);
   MBbytesArray.push_back(1.0*dum);
   MBbytesArray.push_back(10.0*dum);
   MBbytesArray.push_back(100.*dum);

   const int npartners = TaskDivider->GetNPartners(); 

   printf("Hello From Rank %d. Assigned to color: %d. No. Partners: %d\n",whoami, mycolor, npartners);
   CMPIWrapper::Barrier(MPI_COMM_WORLD);

#ifdef VALIDATION
   if(root)
   {
      printf("==========================\n");
      printf("Running with validation...\n");
      printf("==========================\n");
   }
#endif


   for( auto& mb:MBbytesArray )
   {
       
      int size = static_cast<int>(mb*1e6/((real)sizeof(real))*dum);
      if( root )printf("P2P Testing size: %.5e Mb\n", size*sizeof(real)/1e6 ); 

      auto DataHalo = std::make_unique<CDataExchange<CP2PComm<real>,real>>
                     ( size, whoami, npartners , TaskDivider );
#pragma omp simd
      for (int i = 0; i < size; i++)
      {
         const real val = static_cast<real>(i);
         DataHalo->SetBuffer(val*(whoami+1.0),i);
      }

      CMPIWrapper::Barrier(MPI_COMM_WORLD);

      double t1 = MPI_Wtime();
// set the clock and estimate the time
      for(int iRepeat = 0; iRepeat < nrepeats; iRepeat++)
      {
         DataHalo->ExchangeAllData(); 
      }
      double t2 = MPI_Wtime(); 
      const double telapsed = (t2-t1)/((double)nrepeats); 

#ifdef VALIDATION
      for(int ipart = 0; ipart<npartners; ipart++)
      {
         const int rank_ = TaskDivider->GetPartnerRank(ipart);
         for(int isize = 0; isize<DataHalo->GetPartnerSize(ipart); isize++)
         {
            const real valrecv = DataHalo->ReadPartnerBuffer(isize, ipart);
            const real val = static_cast<real>(isize);
            const real expecval = (rank_+1.0)*val;
            const real diff = std::abs(expecval-valrecv);
            if( diff>1e-15 )
            printf("Unexpected value at %d for partner %d. \
                     Diff = %e \n", isize, rank_, diff);
         }
      }
#endif

      double tot_elapsed;
      CMPIWrapper::Reduce( &telapsed
                         , &tot_elapsed
                         , 1
                         , MPI_DOUBLE_PRECISION
                         , MPI_SUM
                         , 0
                         , com_color);

// Compute the average time accross all ranks
      if( whoami == root_color && !root )
      {
         tot_elapsed = tot_elapsed / (double)size_color;
         CMPIWrapper::Send(  &tot_elapsed
                           , 1
                           , MPI_DOUBLE_PRECISION
                           , 0
                           , tag
                           , MPI_COMM_WORLD);
      }
      else if( whoami == root_color && root )
      {
         double tot_elapsed_1;
         CMPIWrapper::Recv(  &tot_elapsed_1
                           , 1
                           , MPI_DOUBLE_PRECISION
                           , MPI_ANY_SOURCE
                           , tag
                           , MPI_COMM_WORLD
                           , MPI_STATUS_IGNORE );
         
         tot_elapsed = tot_elapsed / size_color;

         myfilep2p<< tot_elapsed  << "    " 
                  << "    "       << tot_elapsed_1 
                  << "    "       << size*sizeof(real)/1e6 
                  << std::endl;
      }
   }

   if( root ) myfilep2p.close();               



// Do similar for shm communication. 
   for( auto& mb:MBbytesArray )
   {
       
      int size = static_cast<int>(mb*1e6/((real)sizeof(real))*dum);
      if( root )printf("SHM Testing size: %.5e Mb\n", size*sizeof(real)/1e6 ); 

      auto DataHalo = 
         std::make_unique<CDataExchange<CSharedMemoryComm<real>,real>>
            ( size, whoami, npartners, TaskDivider );


#pragma omp simd
      for (int i = 0; i < size; i++)
      {
         const real val = static_cast<real>(i);
         DataHalo->SetBuffer(val*(whoami+1.0),i);
      }



      CMPIWrapper::Barrier(MPI_COMM_WORLD);

      double t1 = MPI_Wtime();
// set the clock and estimate the time
      for(int iRepeat = 0; iRepeat < nrepeats; iRepeat++)
      {
         DataHalo->ExchangeAllData(); 
      }
      double t2 = MPI_Wtime(); 
      const double telapsed = (t2-t1)/((double)nrepeats); 

#ifdef VALIDATION
      for(int ipart = 0; ipart<npartners; ipart++)
      {
         const int rank_ = TaskDivider->GetPartnerRank(ipart);

         for(int isize = 0; isize<DataHalo->GetPartnerSize(ipart); isize++)
         {
            const real valrecv = DataHalo->ReadPartnerBuffer(isize, ipart);
            const real val = static_cast<real>(isize);
            const real expecval = (rank_+1.0)*val;
            const real diff = std::abs(expecval-valrecv);
            if( diff>1e-15 ){
               printf("Error in Rank %d. Unexpected value at index no. %d for partner %d.\n", whoami, isize, rank_);
               printf("Expected value = %e. Received value = %e\n\n", expecval, valrecv); 
            }
         }
      }

#endif


      double tot_elapsed;
      CMPIWrapper::Reduce( &telapsed
                         , &tot_elapsed
                         , 1
                         , MPI_DOUBLE_PRECISION
                         , MPI_SUM
                         , 0
                         , com_color);

      if( whoami == root_color && !root )
      {
         tot_elapsed = tot_elapsed / (double)size_color;
         CMPIWrapper::Send(  &tot_elapsed
                           , 1
                           , MPI_DOUBLE_PRECISION
                           , 0
                           , tag
                           , MPI_COMM_WORLD);
      }
      else if( whoami == root_color && root )
      {
         double tot_elapsed_1;
         CMPIWrapper::Recv(  &tot_elapsed_1
                           , 1
                           , MPI_DOUBLE_PRECISION
                           , MPI_ANY_SOURCE
                           , tag
                           , MPI_COMM_WORLD
                           , MPI_STATUS_IGNORE );
         
         tot_elapsed = tot_elapsed / size_color;

         myfileshm<< tot_elapsed  << "    " 
                  << "    "       << tot_elapsed_1 
                  << "    "       << size*sizeof(real)/1e6 
                  << std::endl;
      }
   }

   if( root ) myfileshm.close();     



   
   MPI_Finalize();
   
   return 0;
}
