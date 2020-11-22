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

template<class CommMethod>
void BandwidthTest( const std::unique_ptr<const CTaskDivider>& TaskDivider
                  , const std::vector<real>& MBbytesArray
                  , std::ofstream& fileopen
                  , const int nrepeats);

int main( int argc, char *argv[])
{
   const int ntasks   = 2;     //  max two for now.
   const int nrepeats = 10;    // We repeat the communication for each message to neglect noise.
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
      myfileshm << std::scientific << std::setprecision(4);
      myfilep2p << std::scientific << std::setprecision(4);
   }
   
// This initList will distribute the ranks to the two colors. 
// Below we asign all ranks-1 to color 0 and the last rank to color 1
   auto initList = {size-1, 1};

   auto TaskDivider = std::make_unique<const CTaskDivider>( ntasks, initList );

   auto mycolor    = TaskDivider->GetColor();


 //  If you want to test data transfer only the one way, i.e. color 0 only sends 
 //  data. 
   real dum = 1.;  
   if( mycolor == 1 )dum = 0.;


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

   if( root )printf("Testing P2P communication... \n" ); 

   BandwidthTest<CP2PComm<real>>( TaskDivider
                          , MBbytesArray
                          , myfilep2p
                          , nrepeats );

   if( root ) myfilep2p.close();               


   if( root )printf("Testing SHM communication... \n" ); 

   BandwidthTest<CSharedMemoryComm<real>>( TaskDivider
                          , MBbytesArray
                          , myfileshm
                          , nrepeats );

   if( root ) myfileshm.close();         



   
   MPI_Finalize();
   
   return 0;
}

template<typename CommMethod>
void BandwidthTest( const std::unique_ptr<const CTaskDivider>& TaskDivider
                  , const std::vector<real>& MBbytesArray
                  , std::ofstream& fileopen
                  , const int nrepeats)
{
   const static int tag  = 999999;// must be 6 digits to work with mpi intel
   int whoami;
   const int npartners = TaskDivider->GetNPartners(); 
   double taccess, tcomm;

   auto com_color  = TaskDivider->GetColComm();
   auto root_color = TaskDivider->GetColRoot();
   auto size_color = TaskDivider->GetColSize();

   CMPIWrapper::Comm_rank(MPI_COMM_WORLD, &whoami);

   fileopen 
   << "------------------------------------------------------------------------------------------------"
   << "\n";
   fileopen << "t_access [s]" << "  " << "t_comm [s]" << "    "
             << "t_tot [s]" << "   |  "  
             << "t_access [s]" << "  " << "t_comm [s]" << "    "<< "t_tot [s]"
             << "    " << " Size [Mb]"
             << "\n";
   fileopen 
   << "------------------------------------------------------------------------------------------------"
   << "\n";

   int root = 0;
   if( whoami == 0 )root=1;

   for( auto& mb:MBbytesArray )
   {
      if(root)printf("Testing message size %e Mb \n", mb); 
      int size = static_cast<int>(mb*1e6/((real)sizeof(real)));
      auto DataHalo = std::make_unique<CDataExchange<CommMethod,real>>
                     ( size, whoami, npartners , TaskDivider );

      taccess = 0.;
      tcomm   = 0.;
      for(int iRepeat = 0; iRepeat < nrepeats; iRepeat++)
      {
// set the clock and estimate the time
         double t1 = MPI_Wtime();
#pragma omp simd
         for (int i = 0; i < size; i++)
         {
            const real val = static_cast<real>(i)-static_cast<real>(iRepeat);
            DataHalo->SetBuffer(val*(whoami+1.0),i);
         }

         double t3 = MPI_Wtime();
         MPI_Barrier( MPI_COMM_WORLD);
         t3 = MPI_Wtime();

         taccess += t3-t1;

// Exchange data...
         DataHalo->ExchangeAllData(); 
         double t4 = MPI_Wtime();
         tcomm    += t4-t3;


#ifdef VALIDATION
         for(int ipart = 0; ipart<npartners; ipart++)
         {
            const int rank_ = TaskDivider->GetPartnerRank(ipart);
            for(int isize = 0; isize<DataHalo->GetPartnerSize(ipart); isize++)
            {
               const real valrecv = DataHalo->ReadPartnerBuffer(isize, ipart);

               const real val = static_cast<real>(isize)
                              - static_cast<real>(iRepeat);

               const real expecval = (rank_+1.0)*val;
               const real diff = std::abs(expecval-valrecv);
               if( diff>1e-15 )
               printf("Unexpected value at %d for partner %d. \
                        Diff = %e \n", isize, rank_, diff);
            }
         }
#endif
         double t2 = MPI_Wtime(); 
         taccess  += (t2-t4);
         MPI_Barrier(MPI_COMM_WORLD);

      }// repeats 
      
      taccess = taccess/((double)nrepeats); 
      tcomm   = tcomm/((double)nrepeats); 

      double taccess_avg;
      double tcomm_avg;
      CMPIWrapper::Reduce( &taccess
                         , &taccess_avg
                         , 1
                         , MPI_DOUBLE_PRECISION
                         , MPI_SUM
                         , 0
                         , com_color);



      CMPIWrapper::Reduce( &tcomm
                         , &tcomm_avg
                         , 1
                         , MPI_DOUBLE_PRECISION
                         , MPI_SUM
                         , 0
                         , com_color);

// Compute the average time accross all ranks
      if( whoami == root_color && !root )
      {
         taccess_avg = taccess_avg / (double)size_color;
         tcomm_avg  = tcomm_avg / (double)size_color;
         double send_[2]; 
         send_[0] = taccess_avg; send_[1] = tcomm_avg;
         CMPIWrapper::Send(  &send_
                           , 2
                           , MPI_DOUBLE_PRECISION
                           , 0
                           , tag
                           , MPI_COMM_WORLD);
      }
      else if( whoami == root_color && root )
      {
         double recv_[2];
         CMPIWrapper::Recv( &recv_
                           , 2
                           , MPI_DOUBLE_PRECISION
                           , MPI_ANY_SOURCE
                           , tag
                           , MPI_COMM_WORLD
                           , MPI_STATUS_IGNORE );
         
         taccess_avg = taccess_avg / size_color;
         tcomm_avg     = tcomm_avg / size_color;;

         fileopen<< taccess_avg 
                  << "    "       << tcomm_avg << "    "<<taccess_avg+tcomm_avg
                  << "  |  "       << recv_[0]
                  << "    "       << recv_[1] <<"    "<< recv_[0]+recv_[1]
                  << "    "       << size*sizeof(real)/1e6 
                  << std::endl;
      }
   }

}