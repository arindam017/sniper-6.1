#ifndef CACHE_SET_ROUND_ROBIN_H
#define CACHE_SET_ROUND_ROBIN_H

#include "cache_set.h"

class CacheSetRoundRobin : public CacheSet
{
   public:
      CacheSetRoundRobin(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetRoundRobin();

      UInt32 getReplacementIndex(CacheCntlr *cntlr, IntPtr eip, UInt32 set_index);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag, UInt32 set_index);

   private:
      UInt32 m_replacement_index;
};

#endif /* CACHE_SET_ROUND_ROBIN_H */
