#ifndef CACHE_SET_PHC_H
#define CACHE_SET_PHC_H

#include "cache_set.h"
#include "cache_set_lru.h"
/*
class CacheSetInfoLRU : public CacheSetInfo
{
   public:
      CacheSetInfoLRU(String name, String cfgname, core_id_t core_id, UInt32 associativity, UInt8 num_attempts);
      virtual ~CacheSetInfoLRU();
      void increment(UInt32 index)
      {
         LOG_ASSERT_ERROR(index < m_associativity, "Index(%d) >= Associativity(%d)", index, m_associativity);
         ++m_access[index];
      }
      void incrementAttempt(UInt8 attempt)
      {
         if (m_attempts)
            ++m_attempts[attempt];
         else
            LOG_ASSERT_ERROR(attempt == 0, "No place to store attempt# histogram but attempt != 0");
      }
   private:
      const UInt32 m_associativity;
      UInt64* m_access;
      UInt64* m_attempts;
};*/

class CacheSetPHC : public CacheSet
{
   public:
      CacheSetPHC(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize, CacheSetInfoLRU* set_info, UInt8 num_attempts);
      virtual ~CacheSetPHC();

      virtual UInt32 getReplacementIndex(CacheCntlr *cntlr, IntPtr eip, UInt32 set_index);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag, UInt32 set_index);
      void updateLoopBitPolicy(UInt32 index, UInt8 loopbit); //sn

   protected:
      const UInt8 m_num_attempts;
      UInt8* m_lru_bits;
      UInt16* m_TI;
      UInt8* m_cost;
      //UInt8* m_state;
      CacheSetInfoLRU* m_set_info;
      void moveToMRU(UInt32 accessed_index);
};

#endif /* CACHE_SET_LRU_H */
