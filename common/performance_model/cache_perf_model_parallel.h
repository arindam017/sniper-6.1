#ifndef __CACHE_PERF_MODEL_PARALLEL_H__ 
#define __CACHE_PERF_MODEL_PARALLEL_H__

#include "cache_perf_model.h"

class CachePerfModelParallel : public CachePerfModel
{
   private:
      bool m_enabled;

   public:
      CachePerfModelParallel(const ComponentLatency& cache_data_access_time,
            const ComponentLatency& cache_tags_access_time,const ComponentLatency& cache_data_write_time) :	//sn; added cache_data_write_time
         CachePerfModel(cache_data_access_time, cache_tags_access_time,cache_data_write_time),
         m_enabled(false)
      {}
      ~CachePerfModelParallel() {}

      void enable() { m_enabled = true; }
      void disable() { m_enabled = false; }
      bool isEnabled() { return m_enabled; }

      SubsecondTime getLatency(CacheAccess_t access)
      {
         if (!m_enabled)
            return SubsecondTime::Zero();

         switch(access)
         {
            case ACCESS_CACHE_TAGS:
               return m_cache_tags_access_time.getLatency();

            case ACCESS_CACHE_DATA:
            case ACCESS_CACHE_DATA_AND_TAGS:
               return m_cache_data_access_time.getLatency();

            /* As data_access_time is larger than tag_access_time */
            case ACCESS_CACHE_WRITEDATA_AND_TAGS:	//sn: this enum has been added in cache_perf_model.h
            {
               //printf("cycles = %s\n", itostr(m_cache_data_write_time.getLatency()).c_str());
               return m_cache_data_write_time.getLatency();
             }

            case WRITE_CACHE_DATA:					//sn: this enum has been added in cache_perf_model.h
               return m_cache_data_write_time.getLatency();

            default:
               return SubsecondTime::Zero();
         }
      }
 
};

#endif /* __CACHE_PERF_MODEL_PARALLEL_H__ */
