/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This file contains a configurable cache class
 */

 #ifndef PIN_CACHE_H
 #define PIN_CACHE_H
 
 #include <string>
 
 #include "pin_util.H"
 #include "eaf.H"
 
 // Add a forward declaration in pin_cache.H so the template sees it:
 struct EAF;         // forward‐declare type
 extern EAF ul3_eaf; // filter lives elsewhere
 
 /*!
  *  @brief Checks if n is a power of 2.
  *  @returns true if n is power of 2
  */
 static inline bool IsPower2(UINT32 n) { return ((n & (n - 1)) == 0); }
 
 /*!
  *  @brief Computes floor(log2(n))
  *  Works by finding position of MSB set.
  *  @returns -1 if n == 0.
  */
 static inline INT32 FloorLog2(UINT32 n)
 {
     INT32 p = 0;
 
     if (n == 0)
         return -1;
 
     if (n & 0xffff0000)
     {
         p += 16;
         n >>= 16;
     }
     if (n & 0x0000ff00)
     {
         p += 8;
         n >>= 8;
     }
     if (n & 0x000000f0)
     {
         p += 4;
         n >>= 4;
     }
     if (n & 0x0000000c)
     {
         p += 2;
         n >>= 2;
     }
     if (n & 0x00000002)
     {
         p += 1;
     }
 
     return p;
 }
 
 /*!
  *  @brief Computes floor(log2(n))
  *  Works by finding position of MSB set.
  *  @returns -1 if n == 0.
  */
 static inline INT32 CeilLog2(UINT32 n) { return FloorLog2(n - 1) + 1; }
 
 /*!
  *  @brief Cache tag - self clearing on creation
  */
 class CACHE_TAG
 {
 private:
     ADDRINT _tag;
 
 public:
     CACHE_TAG(ADDRINT tag = 0) { _tag = tag; }
     bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
     operator ADDRINT() const { return _tag; }
 };
 
 class CACHE_COUNTER
 {
     private:
     UINT64 _counter;
 
   public:
     CACHE_COUNTER(UINT64 counter = 0) { _counter = counter; }
     bool operator==(const CACHE_COUNTER& right) const { return _counter == right._counter; }
     bool operator--() const { return _counter-1; }
     void decrement(){_counter -= 1;}
     UINT64 get_count(){return _counter;}
     operator UINT64() const { return _counter; }
 };
 
 
 /*!
  * Everything related to cache sets
  */
 namespace CACHE_SET
 {
     /*!
      *  @brief Cache set direct mapped
      */
     class DIRECT_MAPPED
     {
     private:
         CACHE_TAG _tag;
 
     public:
         DIRECT_MAPPED(UINT32 associativity = 1) { ASSERTX(associativity == 1); }
 
         VOID SetAssociativity(UINT32 associativity) { ASSERTX(associativity == 1); }
         UINT32 GetAssociativity(UINT32 associativity) { return 1; }
 
         UINT32 Find(CACHE_TAG tag) { return (_tag == tag); }
         VOID Replace(CACHE_TAG tag) { _tag = tag; }
         VOID Flush() { _tag = 0; }
     };
 
     /*!
      *  @brief Cache set with round robin replacement
      */
     template <UINT32 MAX_ASSOCIATIVITY = 4>
     class ROUND_ROBIN
     {
     private:
         CACHE_TAG _tags[MAX_ASSOCIATIVITY];
         UINT32 _tagsLastIndex;
         UINT32 _nextReplaceIndex;
 
     public:
         ROUND_ROBIN(UINT32 associativity = MAX_ASSOCIATIVITY) : _tagsLastIndex(associativity - 1)
         {
             ASSERTX(associativity <= MAX_ASSOCIATIVITY);
             _nextReplaceIndex = _tagsLastIndex;
 
             for (INT32 index = _tagsLastIndex; index >= 0; index--)
             {
                 _tags[index] = CACHE_TAG(0);
             }
         }
 
         VOID SetAssociativity(UINT32 associativity)
         {
             ASSERTX(associativity <= MAX_ASSOCIATIVITY);
             _tagsLastIndex = associativity - 1;
             _nextReplaceIndex = _tagsLastIndex;
         }
         UINT32 GetAssociativity(UINT32 associativity) { return _tagsLastIndex + 1; }
 
         UINT32 Find(CACHE_TAG tag)
         {
             bool result = true;
 
             for (INT32 index = _tagsLastIndex; index >= 0; index--)
             {
                 // this is an ugly micro-optimization, but it does cause a
                 // tighter assembly loop for ARM that way ...
                 if (_tags[index] == tag)
                     goto end;
             }
             result = false;
 
         end:
             return result;
         }
 
         VOID Replace(CACHE_TAG tag)
         {
             // g++ -O3 too dumb to do CSE on following lines?!
             const UINT32 index = _nextReplaceIndex;
 
             _tags[index] = tag;
             // condition typically faster than modulo
             _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
         }
         VOID Flush()
         {
             for (INT32 index = _tagsLastIndex; index >= 0; index--)
             {
                 _tags[index] = 0;
             }
             _nextReplaceIndex = _tagsLastIndex;
         }
     };
 
     // LEAST RECENTLY USED CACHE
 
     template <UINT32 MAX_ASSOCIATIVITY = 16>
     class LEAST_RECENTLY_USED
     {
     private:
         CACHE_COUNTER _counters[MAX_ASSOCIATIVITY];
         CACHE_TAG _tags[MAX_ASSOCIATIVITY];
 
         UINT32 _tagsLastIndex;
         UINT32 _nextReplaceIndex;
 
     public:
         LEAST_RECENTLY_USED(UINT32 associativity = MAX_ASSOCIATIVITY) : _tagsLastIndex(associativity - 1)
         {
             ASSERTX(associativity <= MAX_ASSOCIATIVITY);
             _nextReplaceIndex = _tagsLastIndex;
 
             // std::cout<<"LRU Cache Constructor called \n";
             // std::cout<<"MAX_ASSOCIATIVITY = "<<MAX_ASSOCIATIVITY<<"\n";
             // std::cout<<"_tagsLastIndex = "<<_tagsLastIndex<<"\n";
 
             // variable for initializing the counters
             ADDRINT counter_value = _tagsLastIndex;
             // std::cout << "Hello 1";
             for (INT32 index = _tagsLastIndex; index >= 0; index--)
             {
                 _tags[index] = CACHE_TAG(0);
                 // initialize the counters |0|1|2|3|
                 _counters[index] = CACHE_COUNTER(counter_value--);
             }
         }
 
         VOID SetAssociativity(UINT32 associativity)
         {
             ASSERTX(associativity <= MAX_ASSOCIATIVITY);
             _tagsLastIndex = associativity - 1;
             _nextReplaceIndex = _tagsLastIndex;
         }
         UINT32 GetAssociativity(UINT32 associativity) { return _tagsLastIndex + 1; }
 
         UINT32 Find(CACHE_TAG tag)
         {
             bool result = true;
             // std::cout<<"Inside the find function\n";
 
             for (UINT32 index = _tagsLastIndex; index >= 0 && index <= (_tagsLastIndex); index--)
             {
                 // this is an ugly micro-optimization, but it does cause a
                 // tighter assembly loop for ARM that way ...
                 // std::cout<<"Inside for loop before if "<<index<<"\n";
                 // if(index >= 0)
                 // {
 
                 if (_tags[index] == tag)
                 {
                     // std::cout<<"hit on index "<<index<<"\n";
                     // decrement the value of the other counters
                     // if(_counters[index] != MAX_ASSOCIATIVITY-1)
                     // {
                     for (UINT32 index2 = _tagsLastIndex; index2 >= 0 && index2 <= (_tagsLastIndex); index2--)
                     {
                         if (index2 != index && _counters[index2].get_count() > 0 && _counters[index2].get_count() > _counters[index].get_count())
                         {
                             _counters[index2].decrement();
                             // std::cout << "Hello 2";
                         }
                     }
                     // }
 
                     // maximize the counter of found index
                     _counters[index] = CACHE_COUNTER(_tagsLastIndex);
 
                     goto end;
                 }
                 // }
             }
             // std::cout<<"After the for loop\n";
             result = false;
 
         end:
             return result;
         }
 
         CACHE_TAG Replace(CACHE_TAG tag, UINT32 newage)
         {
             // Index to be replaced
             // const UINT32 index = _nextReplaceIndex;
 
             // determine the counter with the min count value
 
             // UINT32 min_counter = MAX_ASSOCIATIVITY;
             for (UINT32 index = _tagsLastIndex; index >= 0 && index <= (_tagsLastIndex); index--)
             {
                 if (_counters[index].get_count() == 0)
                 {
                     _nextReplaceIndex = index;
                     // min_counter = _counters[index];
                     // std::cout << "Hello 3";
                     break;
                 }
             }
 
             // copy of the victim tag
             CACHE_TAG old = _tags[_nextReplaceIndex];
 
             // If newage is 0, then it is a LOW-REUSE block
             if (newage != 0) {
                 // decrement the value of the other counters
                 for (UINT32 index = _tagsLastIndex; index >= 0 && index <= (_tagsLastIndex); index--)
                 {
                     if (index != _nextReplaceIndex && _counters[index].get_count() > 0)
                     {
                         _counters[index].decrement();
                         // std::cout << "Hello 4";
                     }
                 }
                 _counters[_nextReplaceIndex] = CACHE_COUNTER(_tagsLastIndex);
             }
 
             // Replace the tag
             _tags[_nextReplaceIndex] = tag;
 
             // std::cout<<_nextReplaceIndex<<" "<<_counters[_nextReplaceIndex].get_count()<<"\n";
             // std::cout<<_nextReplaceIndex<<" ";
             // condition typically faster than modulo
             // _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
             //  return -1;
             return old;
         }
         VOID Flush()
         {
             for (INT32 index = _tagsLastIndex; index >= 0; index--)
             {
                 _tags[index] = 0;
                 _counters[index] = CACHE_COUNTER(0);
                 // std::cout << "Hello 5";
             }
             _nextReplaceIndex = _tagsLastIndex;
         }
     };
 
 } // namespace CACHE_SET
 
 namespace CACHE_ALLOC
 {
     typedef enum
     {
         STORE_ALLOCATE,
         STORE_NO_ALLOCATE
     } STORE_ALLOCATION;
 }
 
 /*!
  *  @brief Generic cache base class; no allocate specialization, no cache set specialization
  */
 class CACHE_BASE
 {
 public:
     // types, constants
     typedef enum
     {
         ACCESS_TYPE_LOAD,
         ACCESS_TYPE_STORE,
         ACCESS_TYPE_NUM
     } ACCESS_TYPE;
 
 protected:
     static const UINT32 HIT_MISS_NUM = 2;
     CACHE_STATS _access[ACCESS_TYPE_NUM][HIT_MISS_NUM];
 
 private:
     // input params
     const std::string _name;
     const UINT32 _cacheSize;
     const UINT32 _lineSize;
     const UINT32 _associativity;
     UINT32 _numberOfFlushes;
     UINT32 _numberOfResets;
 
     // computed params
     const UINT32 _lineShift;
     const UINT32 _setIndexMask;
 
     CACHE_STATS SumAccess(bool hit) const
     {
         CACHE_STATS sum = 0;
 
         for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
         {
             sum += _access[accessType][hit];
         }
 
         return sum;
     }
 
 protected:
     UINT32 NumSets() const { return _setIndexMask + 1; }
 
 public:
     // constructors/destructors
     CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity);
 
     // accessors
     UINT32 CacheSize() const { return _cacheSize; }
     UINT32 LineSize() const { return _lineSize; }
     UINT32 Associativity() const { return _associativity; }
     //
     CACHE_STATS Hits(ACCESS_TYPE accessType) const { return _access[accessType][true]; }
     CACHE_STATS Misses(ACCESS_TYPE accessType) const { return _access[accessType][false]; }
     CACHE_STATS Accesses(ACCESS_TYPE accessType) const { return Hits(accessType) + Misses(accessType); }
     CACHE_STATS Hits() const { return SumAccess(true); }
     CACHE_STATS Misses() const { return SumAccess(false); }
     CACHE_STATS Accesses() const { return Hits() + Misses(); }
 
     CACHE_STATS Flushes() const { return _numberOfFlushes; }
     CACHE_STATS Resets() const { return _numberOfResets; }
 
     VOID SplitAddress(const ADDRINT addr, CACHE_TAG &tag, UINT32 &setIndex) const
     {
         tag = addr >> _lineShift;
         setIndex = tag & _setIndexMask;
     }
 
     VOID SplitAddress(const ADDRINT addr, CACHE_TAG &tag, UINT32 &setIndex, UINT32 &lineIndex) const
     {
         const UINT32 lineMask = _lineSize - 1;
         lineIndex = addr & lineMask;
         SplitAddress(addr, tag, setIndex);
     }
 
     VOID IncFlushCounter() { _numberOfFlushes += 1; }
 
     VOID IncResetCounter() { _numberOfResets += 1; }
 
     std::ostream &StatsLong(std::ostream &out) const;
 };
 
 CACHE_BASE::CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
     : _name(name), _cacheSize(cacheSize), _lineSize(lineSize), _associativity(associativity), _lineShift(FloorLog2(lineSize)),
       _setIndexMask((cacheSize / (associativity * lineSize)) - 1)
 {
     ASSERTX(IsPower2(_lineSize));
     ASSERTX(IsPower2(_setIndexMask + 1));
 
     for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
     {
         _access[accessType][false] = 0;
         _access[accessType][true] = 0;
     }
 }
 
 /*!
  *  @brief Stats output method
  */
 std::ostream &CACHE_BASE::StatsLong(std::ostream &out) const
 {
     const UINT32 headerWidth = 19;
     const UINT32 numberWidth = 10;
 
     out << _name << ":" << std::endl;
 
     for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
     {
         const ACCESS_TYPE accessType = ACCESS_TYPE(i);
 
         std::string type(accessType == ACCESS_TYPE_LOAD ? "Load" : "Store");
 
         out << StringString(type + " Hits:      ", headerWidth) << StringInt(Hits(accessType), numberWidth) << std::endl;
         out << StringString(type + " Misses:    ", headerWidth) << StringInt(Misses(accessType), numberWidth) << std::endl;
         out << StringString(type + " Accesses:  ", headerWidth) << StringInt(Accesses(accessType), numberWidth) << std::endl;
         out << StringString(type + " Miss Rate: ", headerWidth)
             << StringFlt(100.0 * Misses(accessType) / Accesses(accessType), 2, numberWidth - 1) << "%" << std::endl;
         out << std::endl;
     }
 
     out << StringString("Total Hits:      ", headerWidth, ' ') << StringInt(Hits(), numberWidth) << std::endl;
     out << StringString("Total Misses:    ", headerWidth, ' ') << StringInt(Misses(), numberWidth) << std::endl;
     out << StringString("Total Accesses:  ", headerWidth, ' ') << StringInt(Accesses(), numberWidth) << std::endl;
     out << StringString("Total Miss Rate: ", headerWidth, ' ') << StringFlt(100.0 * Misses() / Accesses(), 2, numberWidth - 1)
         << "%" << std::endl;
 
     out << StringString("Flushes:         ", headerWidth, ' ') << StringInt(Flushes(), numberWidth) << std::endl;
     out << StringString("Stat Resets:     ", headerWidth, ' ') << StringInt(Resets(), numberWidth) << std::endl;
 
     out << std::endl;
 
     return out;
 }
 
 /// ostream operator for CACHE_BASE
 std::ostream &operator<<(std::ostream &out, const CACHE_BASE &cacheBase) { return cacheBase.StatsLong(out); }
 
 /*!
  *  @brief Templated cache class with specific cache set allocation policies
  *
  *  All that remains to be done here is allocate and deallocate the right
  *  type of cache sets.
  */
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 class CACHE : public CACHE_BASE
 {
 private:
     SET _sets[MAX_SETS];
 
 public:
     // constructors/destructors
     CACHE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
         : CACHE_BASE(name, cacheSize, lineSize, associativity)
     {
         ASSERTX(NumSets() <= MAX_SETS);
 
         for (UINT32 i = 0; i < NumSets(); i++)
         {
             _sets[i].SetAssociativity(associativity);
         }
     }
 
     // modifiers
     /// Cache access from addr to addr+size-1
     bool Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
     /// Cache access at addr that does not span cache lines
     bool AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType);
     /* ---------- NEW: LLC version that uses an Evicted‑Address Filter ------ */
     bool UL3AccessEAF(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
 
     bool Access_LRU_L3(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
 
     void Flush();
     void ResetStats();
 };
 
 /*!
  *  @return true if all accessed cache lines hit
  */
 
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 bool CACHE<SET, MAX_SETS, STORE_ALLOCATION>::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
 {
     const ADDRINT highAddr = addr + size;
     bool allHit = true;
 
     const ADDRINT lineSize = LineSize();
     const ADDRINT notLineMask = ~(lineSize - 1);
     do
     {
         CACHE_TAG tag;
         UINT32 setIndex;
 
         SplitAddress(addr, tag, setIndex);
 
         SET &set = _sets[setIndex];
 
         bool localHit = set.Find(tag);
         allHit &= localHit;
 
         // on miss, loads always allocate, stores optionally
         if ((!localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
         {
             set.Replace(tag);
         }
 
         addr = (addr & notLineMask) + lineSize; // start of next cache line
     } while (addr < highAddr);
 
     _access[accessType][allHit]++;
 
     return allHit;
 }
 
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 bool CACHE<SET, MAX_SETS, STORE_ALLOCATION>::Access_LRU_L3(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
 {
     const ADDRINT highAddr = addr + size;
     bool allHit = true;
 
     const ADDRINT lineSize = LineSize();
     const ADDRINT notLineMask = ~(lineSize - 1);
     do
     {
         CACHE_TAG tag;
         UINT32 setIndex;
 
         SplitAddress(addr, tag, setIndex);
 
         SET &set = _sets[setIndex];
 
         bool localHit = set.Find(tag);
         allHit &= localHit;
 
         // on miss, loads always allocate, stores optionally
         if ((!localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
         {
             set.Replace(tag,1);
         }
 
         addr = (addr & notLineMask) + lineSize; // start of next cache line
     } while (addr < highAddr);
 
     _access[accessType][allHit]++;
 
     return allHit;
 }
 
 // Replace the plain call ul3.Access() with a dedicated wrapper so we can intercept the miss path:

 //24th April 2025: Avoid regressions on very LRU-friendly kernels (simple set-dueling “D-EAF’’)
 // ---- simple 64-set duel to decide whether we should bypass BIP ----
static UINT64 duelMissA = 0, duelMissB = 0;
static const UINT32 GROUP_MASK = 0x3F;      // 64 sets
static bool globalHighPriority = false;     // start with normal EAF

inline void update_duel(UINT32 setIdx, bool miss, bool tookHigh)
{
    // First 64 logical sets = group A, next 64 = group B
    if (setIdx & GROUP_MASK) {
        if (miss) (setIdx & (GROUP_MASK<<6) ? duelMissB : duelMissA)++;
        // every 1 M inserts re-evaluate
        if ((duelMissA + duelMissB) && (((duelMissA + duelMissB) & 0xFFFFF) == 0))
            globalHighPriority = (duelMissB < duelMissA);
    }
}


 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 bool CACHE<SET, MAX_SETS, STORE_ALLOCATION>::UL3AccessEAF(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
 {
     const ADDRINT highAddr = addr + size;
     bool allHit = true;
 
     const ADDRINT lineSize = LineSize();
     const ADDRINT notLineMask = ~(lineSize - 1);
     const UINT32 maxAge = Associativity() - 1;
     do
     {
         CACHE_TAG tag;
         UINT32 setIndex;
 
         SplitAddress(addr, tag, setIndex);
 
         SET &set = _sets[setIndex];
 
         bool localHit = set.Find(tag);
         allHit &= localHit;
 
         // on miss, loads always allocate, stores optionally
         if ((!localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
         {
 
             /* ---------- 1.  Look up the EAF before replacing the tag to check if it is HIGH-REUSE or LOW-REUSE ---------- */
             bool highReuse = ul3_eaf.test(tag);
 
            //  /* ---------- 2.  pick insertion age ---------- */
            //  UINT32 newAge;
            //  if (highReuse)
            //      newAge = maxAge; // MRU
            //  else
            //      newAge = ((rand() & 0x3F) == 0) ? maxAge // 1/64 MRU
            //                                      : 0;     // usually LRU

            // 24th April 2025: Updated Age selection:
            /* ---------- 2. pick insertion age ---------- */
             UINT32 newAge;

             if (globalHighPriority)
                 newAge = maxAge; // full MRU
             else
             {
                 if (highReuse)
                     newAge = maxAge; // MRU
                 else
                     newAge = ((rand() & 0x3F) == 0) ? maxAge : 0; // BIP
             }

             /* ---------- 3.  insert & get evicted tag ---- */
             CACHE_TAG victim = set.Replace(tag, newAge);

             // 24th April 2025 : After Replace() record duelling statistics
             update_duel(setIndex, !localHit, newAge == maxAge);

             /* ---------- 4.  update EAF ------------------ */
             ul3_eaf.insert(victim);
         }
 
         addr = (addr & notLineMask) + lineSize; // start of next cache line
     } while (addr < highAddr);
 
     _access[accessType][allHit]++;
 
     return allHit;
 }
 
 /*!
  *  @return true if accessed cache line hits
  */
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 bool CACHE<SET, MAX_SETS, STORE_ALLOCATION>::AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
 {
     CACHE_TAG tag;
     UINT32 setIndex;
 
     SplitAddress(addr, tag, setIndex);
 
     SET &set = _sets[setIndex];
 
     bool hit = set.Find(tag);
 
     // on miss, loads always allocate, stores optionally
     if ((!hit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
     {
         set.Replace(tag);
     }
 
     _access[accessType][hit]++;
 
     return hit;
 }
 /*!
  *  @return true if accessed cache line hits
  */
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 void CACHE<SET, MAX_SETS, STORE_ALLOCATION>::Flush()
 {
     for (INT32 index = NumSets(); index >= 0; index--)
     {
         SET &set = _sets[index];
         set.Flush();
     }
     IncFlushCounter();
 }
 
 template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
 void CACHE<SET, MAX_SETS, STORE_ALLOCATION>::ResetStats()
 {
     for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
     {
         _access[accessType][false] = 0;
         _access[accessType][true] = 0;
     }
     IncResetCounter();
 }
 
 // define shortcuts
 #define CACHE_DIRECT_MAPPED(MAX_SETS, ALLOCATION) CACHE<CACHE_SET::DIRECT_MAPPED, MAX_SETS, ALLOCATION>
 #define CACHE_ROUND_ROBIN(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
     CACHE<CACHE_SET::ROUND_ROBIN<MAX_ASSOCIATIVITY>, MAX_SETS, ALLOCATION>
 #define CACHE_LEAST_RECENTLY_USED(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
     CACHE<CACHE_SET::LEAST_RECENTLY_USED<MAX_ASSOCIATIVITY>, MAX_SETS, ALLOCATION>
 
 #endif // PIN_CACHE_H
 