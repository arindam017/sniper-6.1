#ifndef CACHE_SET_NRU_H
#define CACHE_SET_NRU_H

#include "cache_set.h"

class CacheSetNRU : public CacheSet
{
   public:
      CacheSetNRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetNRU();

      UInt32 getReplacementIndex(CacheCntlr *cntlr, IntPtr eip, UInt32 set_index);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag, UInt32 set_index);

   private:
      UInt8* m_lru_bits;
      UInt8  m_num_bits_set;
      UInt8  m_replacement_pointer;
};

#endif /* CACHE_SET_NRU_H */
