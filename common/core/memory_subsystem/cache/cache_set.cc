#include "cache_set.h"
#include "cache_set_lru.h"
#include "cache_set_lru_new.h"
#include "cache_set_phc.h"
#include "cache_set_lru_l3.h"
#include "cache_set_lru_l2.h"
#include "cache_set_mru.h"
#include "cache_set_nmru.h"
#include "cache_set_nru.h"
#include "cache_set_plru.h"
#include "cache_set_random.h"
#include "cache_set_round_robin.h"
#include "cache_set_srrip.h"
#include "cache_base.h"
#include "log.h"
#include "simulator.h"
#include "config.h"
#include "config.hpp"


CacheSet::CacheSet(CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize):
      m_associativity(associativity), m_blocksize(blocksize)
{
   m_cache_block_info_array = new CacheBlockInfo*[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      m_cache_block_info_array[i] = CacheBlockInfo::create(cache_type);
   }

   if (Sim()->getFaultinjectionManager())
   {
      m_blocks = new char[m_associativity * m_blocksize];
      memset(m_blocks, 0x00, m_associativity * m_blocksize);
   } else {
      m_blocks = NULL;
   }
}

CacheSet::~CacheSet()
{
   for (UInt32 i = 0; i < m_associativity; i++)
      delete m_cache_block_info_array[i];
   delete [] m_cache_block_info_array;
   delete [] m_blocks;
}

void
CacheSet::read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes, bool update_replacement)
{
   assert(offset + bytes <= m_blocksize);
   //assert((out_buff == NULL) == (bytes == 0));

   if (out_buff != NULL && m_blocks != NULL)
      memcpy((void*) out_buff, &m_blocks[line_index * m_blocksize + offset], bytes);


   if (update_replacement)
   {
      //printf("updateReplacementIndex called for read hit \n"); //n
      updateReplacementIndex(line_index,0);
   }
}

void
CacheSet::write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes, bool update_replacement)
{
   
   assert(offset + bytes <= m_blocksize);
   //assert((in_buff == NULL) == (bytes == 0));

   if (in_buff != NULL && m_blocks != NULL)
      memcpy(&m_blocks[line_index * m_blocksize + offset], (void*) in_buff, bytes);


   if (update_replacement)
   {
      //printf("updateReplacementIndex called for write hit \n"); //n
      updateReplacementIndex(line_index,1);
   }
}

CacheBlockInfo*
CacheSet::find(IntPtr tag, UInt32* line_index)
{
   for (SInt32 index = m_associativity-1; index >= 0; index--)
   {
      if (m_cache_block_info_array[index]->getTag() == tag)
      {
         if (line_index != NULL)
            *line_index = index;
         return (m_cache_block_info_array[index]);
      }
   }
   return NULL;
}

bool
CacheSet::invalidate(IntPtr& tag)
{
   for (SInt32 index = m_associativity-1; index >= 0; index--)
   {
      if (m_cache_block_info_array[index]->getTag() == tag)
      {
         m_cache_block_info_array[index]->invalidate();
         return true;
      }
   }
   return false;
}

/////////////////created by arindam///////////////////////////
void
CacheSet::insert2(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff, CacheCntlr *cntlr, UInt8 write_flag, IntPtr eip) //sn insert2 function is insert with additional argument
{
   // This replacement strategy does not take into account the fact that
   // cache blocks can be voluntarily flushed or invalidated due to another write request
   //printf("getReplacementIndex called \n"); //n
   //printf("flag is %d inside insert2 \n", write_flag);   //sn
   //printf("insert2 is called and eip is %" PRIxPTR "\n", eip); //sn
   const UInt32 index = getReplacementIndex(cntlr, write_flag, eip);
   
   assert(index < m_associativity);

   assert(eviction != NULL);

   if (m_cache_block_info_array[index]->isValid())
   {
      *eviction = true;
      // FIXME: This is a hack. I dont know if this is the best way to do
      evict_block_info->clone(m_cache_block_info_array[index]);   //m_cache_block_info_array[index] will be evicted. so it is cloned in evict_block_info [ARINDAM]
      if (evict_buff != NULL && m_blocks != NULL)
         memcpy((void*) evict_buff, &m_blocks[index * m_blocksize], m_blocksize);
   }
   else
   {
      *eviction = false;
   }

   // FIXME: This is a hack. I dont know if this is the best way to do
   m_cache_block_info_array[index]->clone(cache_block_info);   //insertion occurs here mainly. cache_block_info is copied into m_cache_block_info_array[index] [ARINDAM]

   if (fill_buff != NULL && m_blocks != NULL)
      memcpy(&m_blocks[index * m_blocksize], (void*) fill_buff, m_blocksize);
}

//////////////////////////////////////////////////////////////



void
CacheSet::insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff, CacheCntlr *cntlr)
{
   // This replacement strategy does not take into account the fact that
   // cache blocks can be voluntarily flushed or invalidated due to another write request
   //printf("getReplacementIndex called \n"); //n
   const UInt32 index = getReplacementIndex(cntlr, 100, 0);
   
   assert(index < m_associativity);

   assert(eviction != NULL);

   if (m_cache_block_info_array[index]->isValid())
   {
      *eviction = true;
      // FIXME: This is a hack. I dont know if this is the best way to do
      evict_block_info->clone(m_cache_block_info_array[index]);   //m_cache_block_info_array[index] will be evicted. so it is cloned in evict_block_info [ARINDAM]
      if (evict_buff != NULL && m_blocks != NULL)
         memcpy((void*) evict_buff, &m_blocks[index * m_blocksize], m_blocksize);
   }
   else
   {
      *eviction = false;
   }

   // FIXME: This is a hack. I dont know if this is the best way to do
   m_cache_block_info_array[index]->clone(cache_block_info);   //insertion occurs here mainly. cache_block_info is copied into m_cache_block_info_array[index] [ARINDAM]

   if (fill_buff != NULL && m_blocks != NULL)
      memcpy(&m_blocks[index * m_blocksize], (void*) fill_buff, m_blocksize);
}



///////////////created by Arindam/////////////////////sn
void
CacheSet::updateLoopBitSet(IntPtr tag, UInt8 loopbit)
{
   UInt32 index=0;
   UInt32 temp_index=0;
   int count=0;

   for(UInt32 i=0;i<m_associativity;i++)
   {
      if(m_cache_block_info_array[i]->getTag()==tag)
      {
         //printf("tag matches at index = %d and tag is  %" PRIxPTR , i,tag);      //sn
         temp_index=i;
         count=1;
      }
   }
   /*
   printf("\n******************\n");     //sn

   for(UInt32 j=0;j<m_associativity;j++)
   {
      printf("  %" PRIxPTR , m_cache_block_info_array[j]->getTag());  //sn
   }

    printf("\n******************\n");     //sn
   */ 

   if(count==1)
   {
      count=0;
      index=temp_index;
      //printf("tag match occurs, index is %d, loopbit is %d (printed in updateLoopBitSet)\n", index,loopbit);  //sn
      updateLoopBitPolicy(index, loopbit);
   }
}
//////////////////////////////////////////////////////


char*
CacheSet::getDataPtr(UInt32 line_index, UInt32 offset)
{
   return &m_blocks[line_index * m_blocksize + offset];
}

CacheSet*
CacheSet::createCacheSet(String cfgname, core_id_t core_id,
      String replacement_policy,
      CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize, CacheSetInfo* set_info)
{
   CacheBase::ReplacementPolicy policy = parsePolicyType(replacement_policy);
   switch(policy)
   {
      case CacheBase::ROUND_ROBIN:
         return new CacheSetRoundRobin(cache_type, associativity, blocksize);

      case CacheBase::LRU:
      case CacheBase::LRU_QBS:
         return new CacheSetLRU(cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::LRU_NEW:
         return new CacheSetLRUNEW(cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::PHC:
         return new CacheSetPHC(cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::LRU_L3:
         return new CacheSetLRUL3(cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::LRU_L2:
         return new CacheSetLRUL2(cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::NRU:
         return new CacheSetNRU(cache_type, associativity, blocksize);

      case CacheBase::MRU:
         return new CacheSetMRU(cache_type, associativity, blocksize);

      case CacheBase::NMRU:
         return new CacheSetNMRU(cache_type, associativity, blocksize);

      case CacheBase::PLRU:
         return new CacheSetPLRU(cache_type, associativity, blocksize);

      case CacheBase::SRRIP:
      case CacheBase::SRRIP_QBS:
         return new CacheSetSRRIP(cfgname, core_id, cache_type, associativity, blocksize, dynamic_cast<CacheSetInfoLRU*>(set_info), getNumQBSAttempts(policy, cfgname, core_id));

      case CacheBase::RANDOM:
         return new CacheSetRandom(cache_type, associativity, blocksize);

      default:
         LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy: %i",
               policy);
         break;
   }

   return (CacheSet*) NULL;
}

CacheSetInfo*
CacheSet::createCacheSetInfo(String name, String cfgname, core_id_t core_id, String replacement_policy, UInt32 associativity)
{
   CacheBase::ReplacementPolicy policy = parsePolicyType(replacement_policy);
   switch(policy)
   {
      case CacheBase::LRU:
      case CacheBase::LRU_QBS:
      case CacheBase::LRU_NEW:
      case CacheBase::LRU_L3:
      case CacheBase::LRU_L2:
      case CacheBase::PHC:
      case CacheBase::SRRIP:
      case CacheBase::SRRIP_QBS:
         return new CacheSetInfoLRU(name, cfgname, core_id, associativity, getNumQBSAttempts(policy, cfgname, core_id));
      default:
         return NULL;
   }
}

UInt8
CacheSet::getNumQBSAttempts(CacheBase::ReplacementPolicy policy, String cfgname, core_id_t core_id)
{
   switch(policy)
   {
      case CacheBase::LRU_QBS:
      case CacheBase::SRRIP_QBS:
         return Sim()->getCfg()->getIntArray(cfgname + "/qbs/attempts", core_id);
      default:
         return 1;
   }
}

CacheBase::ReplacementPolicy
CacheSet::parsePolicyType(String policy)
{
   if (policy == "round_robin")
      return CacheBase::ROUND_ROBIN;
   if (policy == "lru")
      return CacheBase::LRU;
   if (policy == "lrunew")
      return CacheBase::LRU_NEW;
   if (policy == "phc")
      return CacheBase::PHC;
   if (policy == "lrul3")
      return CacheBase::LRU_L3;
   if (policy == "lrul2")
      return CacheBase::LRU_L2;
   if (policy == "lru_qbs")
      return CacheBase::LRU_QBS;
   if (policy == "nru")
      return CacheBase::NRU;
   if (policy == "mru")
      return CacheBase::MRU;
   if (policy == "nmru")
      return CacheBase::NMRU;
   if (policy == "plru")
      return CacheBase::PLRU;
   if (policy == "srrip")
      return CacheBase::SRRIP;
   if (policy == "srrip_qbs")
      return CacheBase::SRRIP_QBS;
   if (policy == "random")
      return CacheBase::RANDOM;

   LOG_PRINT_ERROR("Unknown replacement policy %s", policy.c_str());
}

bool CacheSet::isValidReplacement(UInt32 index)
{
   if (m_cache_block_info_array[index]->getCState() == CacheState::SHARED_UPGRADING)
   {
      return false;
   }
   else
   {
      return true;
   }
}

UInt32
CacheSet::getBlockIndexForGivenTag(IntPtr tagToFind)  //sn copied from anushree
{
    SInt32 blockIndex = -1;
    IntPtr tagInSet;
    
    for (SInt32 index = m_associativity - 1; index >= 0; index--)
    {
        tagInSet = m_cache_block_info_array[index]->getTag();
        if (tagInSet == tagToFind)
        {
            blockIndex = index;
            //printf("Tag1:0x%lx Tag2:0x%lx\n", tagInSet, tagToFind);
            break;
        }
    }

    assert(blockIndex != -1);
    return blockIndex;
}

