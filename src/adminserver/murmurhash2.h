//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef MURMURHASH2_H_
#define MURMURHASH2_H_

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#include "stdint.h"

class CMurmurHash2A
{
public:
  void Begin (uint32_t seed = 0 );

  void Add(const unsigned char *data, int len);
  
  uint32_t End (void);
  
private:
  static const uint32_t m = 0x5bd1e995;
  static const int r = 24;

  void MixTail(const unsigned char *&data, int &len);
  
  uint32_t m_hash;
  uint32_t m_tail;
  uint32_t m_count;
  uint32_t m_size;
};

uint32_t MurmurHash2        ( const void * key, int len, uint32_t seed );
uint64_t MurmurHash64A      ( const void * key, int len, uint64_t seed );
uint64_t MurmurHash64B      ( const void * key, int len, uint64_t seed );
uint32_t MurmurHash2A       ( const void * key, int len, uint32_t seed );
uint32_t MurmurHashNeutral2 ( const void * key, int len, uint32_t seed );
uint32_t MurmurHashAligned2 ( const void * key, int len, uint32_t seed );

//-----------------------------------------------------------------------------

#endif // _MURMURHASH2_H_

