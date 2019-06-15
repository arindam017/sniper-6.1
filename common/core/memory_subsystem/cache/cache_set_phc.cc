#include "cache_set_phc.h"
#include "log.h"
#include "stats.h"

// Implements LRU replacement, optionally augmented with Query-Based Selection [Jaleel et al., MICRO'10]
UInt8 cost_threshold = 20;
UInt8 Ew=24;
UInt8 Er=-1;
UInt16 predictor_table_length=256;
UInt8 state_threshold=2;
UInt8 state_max=3;
UInt8 SRAM_ways=4;
//UInt32 counter=0;



static UInt8 m_state[256] = {0};


CacheSetPHC::CacheSetPHC(
      CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize, CacheSetInfoLRU* set_info, UInt8 num_attempts)
   : CacheSet(cache_type, associativity, blocksize)
   , m_num_attempts(num_attempts)
   , m_set_info(set_info)
{ 

   //printf("set number %d \n", counter);
   //counter++;

   m_lru_bits = new UInt8[m_associativity];

   for (UInt32 i = 0; i < m_associativity; i++)
      m_lru_bits[i] = i;

   m_TI = new UInt16[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
      m_TI[i] = 0; 

   m_cost = new UInt8[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      //printf("i is %d \n",i);
      m_cost[i] = 0; 
   }

   //if(counter==0)
   //{
   //   counter++;
      //printf("predictor table is initialised \n");
/*   m_state = new UInt8[256];
      for (UInt16 i = 0; i < 256; i++)
      {
         //printf("i is %d \n",i);
         m_state[i] = 0;   
      }
      //printf("predictor table initialisation complete \n");
   //}
 */ 
   


   
   //printf("bbbbb \n");

}

CacheSetPHC::~CacheSetPHC()
{
   delete [] m_lru_bits;
   delete [] m_TI;
   delete [] m_cost;
   //delete [] m_state;
}

//m_state = new UInt8[predictor_table_length];
//UInt8 m_state[predictor_table_length];




UInt32
CacheSetPHC::getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag, IntPtr eip)
{

   UInt16 eip_truncated=eip%256;  //truncating eip to last 12bits

   //printf("////////// \ngetReplacementIndex for L3 called and new truncated eip is %d. its state is %d \n", eip_truncated, m_state[eip_truncated]);   //ns

   // First try to find an invalid block
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (!m_cache_block_info_array[i]->isValid())
      {
         // Mark our newly-inserted line as most-recently used
         //printf("invalid block is found and its way is %d. Current truncated eip of block to be evicted is %d \n", i, m_TI[i]);   //ns
         
         //////////////////////////////////////////////////////
         //printf("current cost is \n");
         for (UInt32 i2 = 0; i2 < m_associativity; i2++)
         {
            //printf("%d  ",m_cost[i2]);  //ns
         }


         if((m_cost[i]>cost_threshold) && (m_state[m_TI[i]]<state_max)) //state should be incremented
         {
            //printf("\n state of eip %d is %d and should be incremented \n", eip_truncated, m_state[m_TI[i]]);
            m_state[m_TI[i]]++;
         }
         else if((m_cost[i]<cost_threshold) && (m_state[m_TI[i]]>0))
         {
            //printf("\n state of eip %d is %d and should be decremented \n", eip_truncated, m_state[m_TI[i]]);
            m_state[m_TI[i]]--;
         }
         else 
            //printf("\n state of eip %d is not changed \n", m_TI[i]);

         //printf("\n new state of eip %d is %d \n",eip_truncated, m_state[m_TI[i]] );


         m_TI[i]=eip_truncated;
         m_cost[i]=24;

         //printf("cost after modification is \n");
         for (UInt32 i2 = 0; i2 < m_associativity; i2++)
         {
            //printf("%d  ",m_cost[i2]);  //ns
         }
         //printf("\n");

         //////////////////////////////////////////////////////

         moveToMRU(i);
         return i;
      }
   }

   // Make m_num_attemps attempts at evicting the block at LRU position
   for(UInt8 attempt = 0; attempt < m_num_attempts; ++attempt)
   {
      UInt32 index = 0;
      UInt8 max_bits = 0;

      ////////////////////////[ARINDAM]///////////////////////////////////////////////////

      if(m_state[eip_truncated]<state_threshold)   //TI is cold. select victim from STTRAM
      {

         //printf("invalid block is not found and state of new eip(%d) is %d. TI is cold (STTRAM)\n", eip_truncated, m_state[eip_truncated]);   //ns
         
         for (UInt32 i = SRAM_ways; i < m_associativity; i++)
         {
            if (m_lru_bits[i] > max_bits && isValidReplacement(i))
            {
               index = i;
               max_bits = m_lru_bits[i];
            }
         }

         //printf("way number %d is selected for eviction and current truncated eip of block to be evicted is %d \n", index, m_TI[index]);  //ns

      }

      else  //TI is hot. select victim from SRAM
      {


         //printf("invalid block not found and state of %d is %d. TI is hot (SRAM)\n", eip_truncated, m_state[eip_truncated]);   //ns
         
         for (UInt32 i = 0; i < SRAM_ways; i++)
         {
            if (m_lru_bits[i] > max_bits && isValidReplacement(i))
            {
               index = i;
               max_bits = m_lru_bits[i];
            }
         }


         //printf("way number %d is selected for eviction and current truncated eip of block to be evicted is %d \n", index, m_TI[index]);  //ns
      }


      /////////////////////////////////////////////////////////////////////////////////////

      LOG_ASSERT_ERROR(index < m_associativity, "Error Finding LRU bits");

      bool qbs_reject = false;
      if (attempt < m_num_attempts - 1)
      {
         LOG_ASSERT_ERROR(cntlr != NULL, "CacheCntlr == NULL, QBS can only be used when cntlr is passed in");
         qbs_reject = cntlr->isInLowerLevelCache(m_cache_block_info_array[index]);
      }
      if (qbs_reject)
      {
         // Block is contained in lower-level cache, and we have more tries remaining.
         // Move this block to MRU and try again
         moveToMRU(index);
         cntlr->incrementQBSLookupCost();
         continue;
      }
      else
      {
         // Mark our newly-inserted line as most-recently used

         //////////////////////////////////////////////////////

         //printf("current cost is \n");
         for (UInt32 i5 = 0; i5 < m_associativity; i5++)
         {
            //printf("%d ",m_cost[i5]);  //ns
         }

         if((m_cost[index]>cost_threshold) && (m_state[m_TI[index]]<state_max)) //state should be incremented
         {
            //printf("\n state of eip %d is %d and should be incremented \n", m_TI[index], m_state[m_TI[index]]);
            m_state[m_TI[index]]++;
         }
         else if((m_cost[index]<cost_threshold) && (m_state[m_TI[index]]>0))
         {
            //printf("\n state of eip %d is %d and should be decremented \n", m_TI[index], m_state[m_TI[index]]);
            m_state[m_TI[index]]--;
         }
         else
            //printf("\n state of eip %d is not changed \n", m_TI[index]);


         //printf("\n new state of eip %d is %d \n",m_TI[index], m_state[m_TI[index]] );


         m_TI[index]=eip_truncated;
         m_cost[index]=24;

         //printf("cost after modification is \n");
         for (UInt32 i2 = 0; i2 < m_associativity; i2++)
         {
            //printf("%d ",m_cost[i2]);  //ns
         }
         //printf("\n");


         //////////////////////////////////////////////////////

         moveToMRU(index);
         m_set_info->incrementAttempt(attempt);
         return index;
      }
   }



   LOG_PRINT_ERROR("Should not reach here");   
}

void
CacheSetPHC::updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag)
{
   //printf("\\\\\\\\ \n updateReplacementIndex for L3 is called and accessed_index is %d and write flag is %d \n", accessed_index, write_flag);   //ns
   //printf("cost will be modified here and cost is \n");   //ns

   for(UInt32 i=0; i<m_associativity; i++)
   {
   //   printf("%d ", m_cost[i]);   //ns
   }
   //printf(" \n");   //ns
 


   m_set_info->increment(m_lru_bits[accessed_index]);
   moveToMRU(accessed_index);

   if(write_flag==1)
   {
      m_cost[accessed_index]=m_cost[accessed_index]+24;  //cost modification
   //   printf("new cost is %d \n \\\\\\\\ \n", m_cost[accessed_index]);   //ns
   }
   else if(write_flag==0)
   {
      m_cost[accessed_index]=m_cost[accessed_index]-1;
   //   printf("new cost is %d \n \\\\\\\\ \n", m_cost[accessed_index]);   //n
   }
   else
      printf("error: value of write_flag is %d \n", write_flag);
}

void
CacheSetPHC::moveToMRU(UInt32 accessed_index)
{
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_lru_bits[i] < m_lru_bits[accessed_index])
         m_lru_bits[i] ++;
   }
   m_lru_bits[accessed_index] = 0;
}

////////////created by Arindam//////////////////sn
void
CacheSetPHC::updateLoopBitPolicy(UInt32 index, UInt8 loopbit)
{
  
}
//////////////////////////////////////////////////

/*
CacheSetInfoLRU::CacheSetInfoLRU(String name, String cfgname, core_id_t core_id, UInt32 associativity, UInt8 num_attempts)
   : m_associativity(associativity)
   , m_attempts(NULL)
{
   m_access = new UInt64[m_associativity];
   for(UInt32 i = 0; i < m_associativity; ++i)
   {
      m_access[i] = 0;
      registerStatsMetric(name, core_id, String("access-mru-")+itostr(i), &m_access[i]);
   }

   if (num_attempts > 1)
   {
      m_attempts = new UInt64[num_attempts];
      for(UInt32 i = 0; i < num_attempts; ++i)
      {
         m_attempts[i] = 0;
         registerStatsMetric(name, core_id, String("qbs-attempt-")+itostr(i), &m_attempts[i]);
      }
   }
};

CacheSetInfoLRU::~CacheSetInfoLRU()
{
   delete [] m_access;
   if (m_attempts)
      delete [] m_attempts;
}*/
