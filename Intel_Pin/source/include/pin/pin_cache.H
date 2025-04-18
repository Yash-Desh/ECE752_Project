


// ########################################################################################################################################
// ########################################################################################################################################
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
 // #include "../tools/Memory/mycache.h"  // include full path

 // typedef UINT64 CACHE_STATS; // type of cache hit/miss counters
 
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
 
     if (n == 0) return -1;
 
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
     CACHE_TAG(ADDRINT tag = 0) {
        //std::cout<<"In cache_tag \n";
         _tag = tag; 
         //std::cout<<"exiting cache_tag \n";
        }
     bool operator==(const CACHE_TAG& right) const { return _tag == right._tag; }
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
     // void set_count()
     UINT64 get_count(){return _counter;}
     operator UINT64() const { return _counter; }
 };
 
 class CACHE_METADATA
 {
     private:
     CACHE_TAG _tag;
     UINT64 _counter;
     UINT32 _valid;
     UINT64 _FPTR;
 
   public:
     CACHE_METADATA(UINT64 counter = 0) 
     {
        std::cout<<"In cache Metadata \n";
         _tag = CACHE_TAG(0);
         _counter = counter; 
         _valid = 0;
         _FPTR = 0;
         std::cout<<"Exiting cache Metadata \n";
     }
 
     // bool operator==(const CACHE_METADATA& right) const { return _counter == right._counter; }
     // bool operator--() const { return _counter-1; }
 
     void decrement_count(){_counter -= 1;}
     UINT64 get_count(){return _counter;}
     void set_count(UINT64 counter){_counter = counter;}
 
     bool isValid(){return _valid;}
     void toggleValid(){_valid = ~_valid;}
 
     void set_FPTR(UINT64 FPTR){_FPTR = FPTR;}
     UINT64 get_FPTR(){return _FPTR;}
 
     CACHE_TAG get_tag(){return _tag;}
     void set_tag(CACHE_TAG tag){_tag = tag; }
 
     // operator UINT64() const { return _counter; }
 };
 
 class DATA_METADATA{
     private:
     UINT32 _valid;
     UINT64 _RPTR;
     UINT64 _reuse_counter;
 
     public:
     DATA_METADATA(){
         _valid = 0;
         _reuse_counter = 0;
         _RPTR = -1;
     }
     bool isValid(){return _valid;}
     void toggleValid(){_valid = ~_valid;}
 
     void set_RPTR(UINT64 FPTR){_RPTR = FPTR;}
     UINT64 get_RPTR(){return _RPTR;}
 
     UINT64 get_reuse_counter(){return _reuse_counter;}
     void set_reuse_counter(UINT64 reuse_counter){_reuse_counter = reuse_counter;}
     void increment_reuse_counter()
     {   
         if(_reuse_counter < 4)
             _reuse_counter++;
     }
     void decrement_reuse_counter()
     {
         if(_reuse_counter > 0)
             _reuse_counter--;
     }
 
 };
 
 // extern UINT64 PTR;
 // extern DATA_METADATA _data_store;
 
 
 class DATA 
 {
     public:
     DATA_METADATA *_data_store;
     UINT64 PTR;
     UINT64 data_store_size;
 
     DATA(UINT32 cacheSize, UINT32 lineSize)
     {
        data_store_size = cacheSize/lineSize;
         _data_store = new DATA_METADATA[data_store_size];
         PTR = 0;
     }
 
 };
 
 extern DATA data_array;
 
 
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
     UINT32 Replace(CACHE_TAG tag) { _tag = tag; return -1; }
     VOID Flush() { _tag = 0; }
 };
 
 /*!
  *  @brief Cache set with round robin replacement
  */
 template< UINT32 MAX_ASSOCIATIVITY = 4 > class ROUND_ROBIN
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
         _tagsLastIndex    = associativity - 1;
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
             if (_tags[index] == tag) goto end;
         }
         result = false;
 
     end:
         return result;
     }
 
     UINT32 Replace(CACHE_TAG tag)
     {
         // g++ -O3 too dumb to do CSE on following lines?!
         const UINT32 index = _nextReplaceIndex;
 
         _tags[index] = tag;
         // condition typically faster than modulo
         _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
         return -1;
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
 
 
 template < UINT32 MAX_ASSOCIATIVITY = 4 > class LEAST_RECENTLY_USED
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
         _tagsLastIndex    = associativity - 1;
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
                         for(UINT32 index2 = _tagsLastIndex; index2 >= 0 && index2 <= (_tagsLastIndex); index2--)
                         {
                             if(index2 != index && _counters[index2].get_count() > 0 && _counters[index2].get_count() > _counters[index].get_count())
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
 
     UINT32 Replace(CACHE_TAG tag)
     {
         // Index to be replaced
         //const UINT32 index = _nextReplaceIndex;
 
         // determine the counter with the min count value
 
         // UINT32 min_counter = MAX_ASSOCIATIVITY;
         for(UINT32 index = _tagsLastIndex; index >= 0 && index <= (_tagsLastIndex); index--)
         {
             if(_counters[index].get_count() ==  0)
             {
                 _nextReplaceIndex = index;
                 // min_counter = _counters[index];
                 // std::cout << "Hello 3";
                 break;
             }
         }
 
         // decrement the value of the other counters 
         for(UINT32 index = _tagsLastIndex; index >= 0 && index <= (_tagsLastIndex); index--)
         {
             if(index != _nextReplaceIndex && _counters[index].get_count() > 0)
             {
                 _counters[index].decrement();
                 // std::cout << "Hello 4";
             }
         }
 
         _tags[_nextReplaceIndex] = tag;
         _counters[_nextReplaceIndex] = CACHE_COUNTER(_tagsLastIndex);
         // std::cout<<_nextReplaceIndex<<" "<<_counters[_nextReplaceIndex].get_count()<<"\n";
         // std::cout<<_nextReplaceIndex<<" ";
         // condition typically faster than modulo
         // _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
         return -1;
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
 
     VOID SplitAddress(const ADDRINT addr, CACHE_TAG& tag, UINT32& setIndex) const
     {
         tag      = addr >> _lineShift;
         setIndex = tag & _setIndexMask;
     }
 
     VOID SplitAddress(const ADDRINT addr, CACHE_TAG& tag, UINT32& setIndex, UINT32& lineIndex) const
     {
         const UINT32 lineMask = _lineSize - 1;
         lineIndex             = addr & lineMask;
         SplitAddress(addr, tag, setIndex);
     }
 
     VOID IncFlushCounter() { _numberOfFlushes += 1; }
 
     VOID IncResetCounter() { _numberOfResets += 1; }
 
     std::ostream& StatsLong(std::ostream& out) const;
 };
 
 CACHE_BASE::CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
     : _name(name), _cacheSize(cacheSize), _lineSize(lineSize), _associativity(associativity), _lineShift(FloorLog2(lineSize)),
       _setIndexMask((cacheSize / (associativity * lineSize)) - 1)
 {
    std::cout<<"Entering the Base cache constructor\n";
     ASSERTX(IsPower2(_lineSize));
     ASSERTX(IsPower2(_setIndexMask + 1));
 
     for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
     {
         _access[accessType][false] = 0;
         _access[accessType][true]  = 0;
     }
     std::cout<<"Exiting the Base cache constructor\n";
 }
 
 /*!
  *  @brief Stats output method
  */
 std::ostream& CACHE_BASE::StatsLong(std::ostream& out) const
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
 std::ostream& operator<<(std::ostream& out, const CACHE_BASE& cacheBase) { return cacheBase.StatsLong(out); }
 
 /*!
  *  @brief Templated cache class with specific cache set allocation policies
  *
  *  All that remains to be done here is allocate and deallocate the right
  *  type of cache sets.
  */
 template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION > class CACHE : public CACHE_BASE
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
     void Flush();
     void ResetStats();
 };
 
 /*!
  *  @return true if all accessed cache lines hit
  */
 
 template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION >
 bool CACHE< SET, MAX_SETS, STORE_ALLOCATION >::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
 {
     const ADDRINT highAddr = addr + size;
     bool allHit            = true;
 
     const ADDRINT lineSize    = LineSize();
     const ADDRINT notLineMask = ~(lineSize - 1);
     do
     {
         CACHE_TAG tag;
         UINT32 setIndex;
 
         SplitAddress(addr, tag, setIndex);
 
         SET& set = _sets[setIndex];
 
         bool localHit = set.Find(tag);
         allHit &= localHit;
 
         // on miss, loads always allocate, stores optionally
         if ((!localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
         {
             set.Replace(tag);
         }
 
         addr = (addr & notLineMask) + lineSize; // start of next cache line
     }
     while (addr < highAddr);
 
     _access[accessType][allHit]++;
 
     return allHit;
 }
 
 /*!
  *  @return true if accessed cache line hits
  */
 template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION >
 bool CACHE< SET, MAX_SETS, STORE_ALLOCATION >::AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
 {
     CACHE_TAG tag;
     UINT32 setIndex;
 
     SplitAddress(addr, tag, setIndex);
 
     SET& set = _sets[setIndex];
 
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
 template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION > void CACHE< SET, MAX_SETS, STORE_ALLOCATION >::Flush()
 {
     for (INT32 index = NumSets(); index >= 0; index--)
     {
         SET& set = _sets[index];
         set.Flush();
     }
     IncFlushCounter();
 }
 
 template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION > void CACHE< SET, MAX_SETS, STORE_ALLOCATION >::ResetStats()
 {
     for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
     {
         _access[accessType][false] = 0;
         _access[accessType][true]  = 0;
     }
     IncResetCounter();
 }
 
 
//  template < UINT32 MAX_ASSOCIATIVITY = 4 > class VARIABLE_WAY
//  {
//      private:
//      CACHE_TAG _tags[MAX_ASSOCIATIVITY];
//      CACHE_METADATA _metadatas[MAX_ASSOCIATIVITY];
 
//      //UINT32 _tagsLastIndex; 
//      UINT32 _nextReplaceIndex;
//      UINT32 max_associativity;
//      UINT32 current_associativity;
//      UINT32 user_associativity;
//      UINT32 TDR = 1;
 
//    public:
//    VARIABLE_WAY(UINT32 associativity = MAX_ASSOCIATIVITY) : user_associativity(associativity - 1)
//      {
//          ASSERTX(associativity <= MAX_ASSOCIATIVITY);
//          //_nextReplaceIndex = _tagsLastIndex;
//          current_associativity = 0;        
//          max_associativity = TDR*user_associativity;    // --> this could be parameterized as the TDR
//          // std::cout<<"LRU Cache Constructor called \n";
//          // std::cout<<"MAX_ASSOCIATIVITY = "<<MAX_ASSOCIATIVITY<<"\n";
//          // std::cout<<"_tagsLastIndex = "<<_tagsLastIndex<<"\n";
 
//          // variable for initializing the counters
//          ADDRINT counter_value = user_associativity;
//          // std::cout << "Hello 1";
//          for (INT32 index = user_associativity; index >= 0; index--)
//          {
//              _tags[index] = CACHE_TAG(0);
//              // initialize the counters |0|1|2|3|
//              _metadatas[index] = CACHE_METADATA(counter_value--);
             
//          }
//      }
 
//      VOID SetAssociativity(UINT32 associativity)
//      {
//          ASSERTX(associativity <= MAX_ASSOCIATIVITY);
//          //_tagsLastIndex    = associativity - 1;
//          //_nextReplaceIndex = _tagsLastIndex;
//          user_associativity = associativity - 1;
//          current_associativity = 0;
//          max_associativity = TDR*associativity;    // --> this could be parameterized as the TDR
//      }
//      UINT32 GetAssociativity(UINT32 associativity) { return _tagsLastIndex + 1; }
 
//      UINT32 Find(CACHE_TAG tag)
//      {
//          bool result = true;
//          std::cout<<"Inside the find function\n"<<"current_associativity = "<< current_associativity<<"\n";
 
//          for (UINT32 index = (current_associativity-1); index >= 0 && index <= (current_associativity-1); index--)
//          {
//              // this is an ugly micro-optimization, but it does cause a
//              // tighter assembly loop for ARM that way ...
//              std::cout<<"Inside for loop before if "<<index<<"\n";
//              // if(index >= 0)
//              // {
 
//                  if (_tags[index] == tag) 
//                  {
//                      std::cout<<"hit on index "<<index<<"\n";
//                      // decrement the value of the other counters 
//                      // if(_counters[index] != MAX_ASSOCIATIVITY-1)
//                      // {
//                          for(UINT32 index2 = (current_associativity-1); index2 >= 0 && index2 <= (current_associativity-1); index2--)
//                          {
//                              if(index2 != index && _metadatas[index2].get_count() > 0 && _metadatas[index2].get_count() > _metadatas[index].get_count())
//                              {
//                                  _metadatas[index2].decrement_count();
//                                  // std::cout << "Hello 2";
//                              }
//                          }
//                      // }
 
//                      // maximize the counter of found index
//                      _metadatas[index].set_count(current_associativity-1);
//                      data_array._data_store[_metadatas[index].get_FPTR()].increment_reuse_counter();
                     
//                      goto end;
//                  }
//              // }
//          }
//          std::cout<<"After the for loop\n";
//          result = false;
 
//      end:
//          return result;
//      }
 
//      UINT32 Replace(CACHE_TAG tag)
//      {
//          // std::cout<<"Inside the Replace() function\n";
 
//          // ------------------------ calculate the number of valid ways ------------------------
//          // UINT32 num_valid_ways = 0;
//          // for (UINT32 index = (max_associativity-1); index >= 0 && index <= (max_associativity-1); index--)
//          // {
//          //     if(_metadatas[index].isValid())
//          //     {
//          //         num_valid_ways++;
//          //     }
//          // }
 
 
//          // ------------------------ case-1: At least 1 invalid way in the target set ------------------------
//          if(current_associativity != max_associativity)
//          {
//              //I think victim index is just current associativity
//              // UINT32 victim_index = 0; //find who is invalid?
//              // while(!_metadatas[index].isValid())
//              // {
//              //         victim_index++;
//              // }
//              UINT32 victim_index = current_associativity;
//              _tags[victim_index] = tag;
//              _metadatas[victim_index].toggleValid();
//              _metadatas[victim_index].set_count(current_associativity)
//              current_associativity++;                //demand based growth in associativity
             
//              while(_data_store[PTR]._reuse_counter != 0)
//              {
//                  _data_store[PTR].decrement_reuse_counter();
//                  PTR++;
//                  if(PTR == (sizeof(_data_store)/sizeof(DATA_METADATA) -1)) //give a variable name
//                      PTR = 0
//              }
//              _metadatas[victim_index].set_FPTR(PTR) //Map newly added tag store to its data block
//              UINT64 data_victim_rptr = _data_store[PTR].get_RPTR();
//              PTR++; //Give this new block some time to be resused. 
 
//              //Need to map the RPTR back to this tag
//              //Will get set index from the address
//              if()
//              return data_victim_rptr;
 
 
//          }
 
//          // ------------------------ case-2: All the ways are valid in the target set ------------------------
//          else{
//              for(UINT32 index = (max_associativity-1); index >= 0 && index <= (max_associativity-1); index--)
//              {
//                  if(_metadatas[index].get_count() ==  0)
//                  {
//                      _nextReplaceIndex = index;
//                      // min_counter = _counters[index];
//                      // std::cout << "Hello 3";
//                      break;
//                  }
//              }
 
//              // decrement the value of the other counters 
//              for(UINT32 index = (max_associativity-1); index >= 0 && index <= (max_associativity-1); index--)
//              {
//                  if(index != _nextReplaceIndex && _metadatas[index].get_count() > 0)
//                  {
//                      _metadatas[index].decrement_count();
//                      // std::cout << "Hello 4";
//                  }
//              }
 
//              _tags[_nextReplaceIndex] = tag;
//              _metadatas[_nextReplaceIndex].set_count(max_associativity-1);
//              _metadatas[_nextReplaceIndex].toggleValid();
//              data_array._data_store[_metadatas[_nextReplaceIndex].get_FPTR()].set_reuse_counter(0);
//              data_array.PTR = _metadatas[_nextReplaceIndex].get_FPTR()+1;
 
//              return -1; //necessitated by modularity
//          }
//      }
//      VOID Flush()
//      {
//          // for (INT32 index = _tagsLastIndex; index >= 0; index--)
//          // {
//          //     _tags[index] = 0;
//          //     _counters[index] = CACHE_COUNTER(0);
//          //     // std::cout << "Hello 5";
//          // }
//          // _nextReplaceIndex = _tagsLastIndex;
//      }
//  };
 
 
 
 class SET_METADATA{
     public:
         UINT32 max_associativity;
         UINT32 current_associativity;
         UINT32 user_associativity;
         UINT32 TDR;
 
         SET_METADATA(UINT32 _user_associativity = 0)
         {
             //std::cout<<"In Set Metdata \n";
             user_associativity = _user_associativity;
             current_associativity = 0;      
             TDR = 1;  
             max_associativity = TDR*_user_associativity;
             
             //std::cout<<"Exiting Set Metdata \n";
         }
         void SetAssociativity(UINT32 associativity)
         {
             //ASSERTX(associativity <= MAX_ASSOCIATIVITY);
             user_associativity = associativity;
             current_associativity = 0;        
             max_associativity = TDR*user_associativity;
         }
 
 };
 
//  template< UINT32 MAX_ASSOCIATIVITY, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION> class VWAY_CACHE : public CACHE_BASE
class VWAY_CACHE : public CACHE_BASE
{
   private:
    //  CACHE_METADATA tag_stores[MAX_SETS][MAX_ASSOCIATIVITY];
    // //  CACHE_METADATA tag_stores[MAX_SETS];
    //  SET_METADATA set_data[MAX_SETS];
    //  UINT32 victim_index;

    CACHE_METADATA ** tag_stores;
    SET_METADATA* set_data;
    UINT32 victim_index;
    UINT32 STORE_ALLOCATION;
 
   public:
     // constructors/destructors
     VWAY_CACHE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
         : CACHE_BASE(name, cacheSize, lineSize, associativity)
     {
        std::cout<<"Entering the VWAY constructor\n";
        //  ASSERTX(NumSets() <= MAX_SETS);

        UINT32 num_sets = cacheSize/ (lineSize* associativity);
        set_data = new SET_METADATA[num_sets];
        tag_stores = new CACHE_METADATA * [num_sets];
        std::cout<<"Before the first for loop in the VWAY constructor\n";
        for(UINT32 i=0; i<num_sets; i++)
        {
            std::cout<<"inside first for loop in the VWAY constructor\n";
            tag_stores[i] = new (std::nothrow) CACHE_METADATA[(set_data[i].TDR)*associativity];
            if (!tag_stores[i]) {
                std::cerr << "Allocation failed" << std::endl;
                // Handle gracefully
            }
        }


        STORE_ALLOCATION = CACHE_ALLOC::STORE_ALLOCATE;

        std::cout<<"Midway in the VWAY constructor\n";

         for(UINT32 index = 0; index < num_sets; index++ )
         {
             set_data[index].SetAssociativity(associativity);
             for(UINT32 index2 = set_data[index].max_associativity; index2 >= 0; index2--) ///Check loop
             {
                 tag_stores[index][index2].set_count(index2);
             }
         }
         std::cout<<"Exiting the VWAY constructor\n";
     }
 
     // modifiers
     /// Cache access from addr to addr+size-1
     bool Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType);
     /// Cache access at addr that does not span cache lines
     bool AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType);
    //  void Flush();
    //  void ResetStats();
 
     UINT32 Find(CACHE_TAG tag, UINT32 set_index)
     {
         bool result = true;
         //std::cout<<"Inside the find function\n"<<"current_associativity = "<< current_associativity<<"\n";
 
         for (UINT32 index = (set_data[set_index].current_associativity-1); index >= 0 && index <= (set_data[set_index].current_associativity-1); index--)
         {
             // this is an ugly micro-optimization, but it does cause a
             // tighter assembly loop for ARM that way ...
             std::cout<<"Inside for loop before if "<<index<<"\n";
             // if(index >= 0)
             // {
 
                 if (tag_stores[set_index][index].get_tag() == tag) 
                 {
                     std::cout<<"hit on index "<<index<<"\n";
                     // decrement the value of the other counters 
                     // if(_counters[index] != MAX_ASSOCIATIVITY-1)
                     // {
                         for(UINT32 index2 = (set_data[set_index].current_associativity-1); index2 >= 0 && index2 <= (set_data[set_index].current_associativity-1); index2--)
                         {
                             if(index2 != index && tag_stores[set_index][index2].get_count() > 0 && tag_stores[set_index][index2].get_count() > tag_stores[set_index][index2].get_count())
                             {
                                 tag_stores[set_index][index2].decrement_count();
                                 // std::cout << "Hello 2";
                             }
                         }
                     // }
 
                     // maximize the counter of found index
                     tag_stores[set_index][index].set_count(set_data[set_index].current_associativity-1);
                     data_array._data_store[tag_stores[set_index][index].get_FPTR()].increment_reuse_counter();
                     
                     goto end;
                 }
             // }
         }
         std::cout<<"After the for loop\n";
         result = false;
 
     end:
         return result;
     }
 
     VOID Replace(CACHE_TAG tag, UINT32 set_index)
     {
         // std::cout<<"Inside the Replace() function\n";
 
         // ------------------------ calculate the number of valid ways ------------------------
         // UINT32 num_valid_ways = 0;
         // for (UINT32 index = (max_associativity-1); index >= 0 && index <= (max_associativity-1); index--)
         // {
         //     if(_metadatas[index].isValid())
         //     {
         //         num_valid_ways++;
         //     }
         // }
 
 
         // ------------------------ case-1: At least 1 invalid way in the target set ------------------------
         if(set_data[set_index].current_associativity != set_data[set_index].max_associativity)
         {
             //I think victim index is just current associativity
             // UINT32 victim_index = 0; //find who is invalid?
             // while(!_metadatas[index].isValid())
             // {
             //         victim_index++;
             // }
             victim_index = set_data[set_index].current_associativity;
             tag_stores[set_index][victim_index].set_tag(tag);
             tag_stores[set_index][victim_index].toggleValid();
             tag_stores[set_index][victim_index].set_count(set_data[set_index].current_associativity);
             set_data[set_index].current_associativity++;                //demand based growth in associativity
             
             while(data_array._data_store[data_array.PTR].get_reuse_counter() != 0)
             {
                data_array._data_store[data_array.PTR].decrement_reuse_counter();
                data_array.PTR++;
                 if(data_array.PTR == (data_array.data_store_size-1)) //give a variable name
                    data_array.PTR = 0;
             }
             tag_stores[set_index][victim_index].set_FPTR(data_array.PTR); //Map newly added tag store to its data block
             UINT64 data_victim_rptr = data_array._data_store[data_array.PTR].get_RPTR();
 
             UINT64 new_rptr = set_index * set_data[set_index].max_associativity + victim_index;
             data_array._data_store[data_array.PTR].set_RPTR(new_rptr);
 
         
 
             data_array.PTR++; //Give this new block some time to be resused. 
             //STEP1 : Invalidate tag victim
             if(data_victim_rptr != UINT64(-1))
             {
                 UINT32 victim_set_index = data_victim_rptr/set_data[set_index].max_associativity;
                 UINT32 victim_way = data_victim_rptr%set_data[set_index].max_associativity;
                 tag_stores[victim_set_index][victim_way].toggleValid();
             }
 
             //Need to map the RPTR back to this tag
             //Will get set index from the address
             // if()
             // return data_victim_rptr;
 
 
         }
 
         // ------------------------ case-2: All the ways are valid in the target set ------------------------
         else{
             for(UINT32 index = (set_data[set_index].max_associativity-1); index >= 0 && index <= (set_data[set_index].max_associativity-1); index--)
             {
                 if(tag_stores[set_index][index].get_count() ==  0)
                 {
                    victim_index = index;
                     // min_counter = _counters[index];
                     // std::cout << "Hello 3";
                     break;
                 }
             }
 
             // decrement the value of the other counters 
             for(UINT32 index = set_data[set_index].max_associativity-1; index >= 0 && index <= (set_data[set_index].max_associativity-1); index--)
             {
                 if(index != victim_index && tag_stores[set_index][index].get_count() > 0)
                 {
                    tag_stores[set_index][index].decrement_count();
                     // std::cout << "Hello 4";
                 }
             }
 
             tag_stores[set_index][victim_index].set_tag(tag);
             tag_stores[set_index][victim_index].set_count(set_data[set_index].max_associativity-1);
             tag_stores[set_index][victim_index].toggleValid();
             data_array._data_store[tag_stores[set_index][victim_index].get_FPTR()].set_reuse_counter(0);
             data_array.PTR = tag_stores[set_index][victim_index].get_FPTR()+1;
 
            //  return -1; //necessitated by modularity
         }
     }
 };
 
 /*!
  *  @return true if all accessed cache lines hit
  */
 
//   template< UINT32 MAX_ASSOCIATIVITY, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
//  bool VWAY_CACHE< MAX_ASSOCIATIVITY, MAX_SETS, STORE_ALLOCATION >::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
 
bool VWAY_CACHE::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType)
{
     const ADDRINT highAddr = addr + size;
     bool allHit            = true;
 
     const ADDRINT lineSize    = LineSize();
     const ADDRINT notLineMask = ~(lineSize - 1);
     do
     {
         CACHE_TAG tag;
         UINT32 setIndex;
 
         SplitAddress(addr, tag, setIndex);
 
         //SET& set = _sets[setIndex];
 
         bool localHit = Find(tag, setIndex);
         allHit &= localHit;
 
         // on miss, loads always allocate, stores optionally
         if ((!localHit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
         {
             Replace(tag, setIndex);
         }
 
         addr = (addr & notLineMask) + lineSize; // start of next cache line
     }
     while (addr < highAddr);
 
     _access[accessType][allHit]++;
 
     return allHit;
 }
 
 /*!
  *  @return true if accessed cache line hits
  */
//  template< UINT32 MAX_ASSOCIATIVITY, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION>
//  bool VWAY_CACHE< MAX_ASSOCIATIVITY, MAX_SETS, STORE_ALLOCATION >::AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
 
bool VWAY_CACHE::AccessSingleLine(ADDRINT addr, ACCESS_TYPE accessType)
{
     CACHE_TAG tag;
     UINT32 setIndex;
 
     SplitAddress(addr, tag, setIndex);
 
     //SET& set = _sets[setIndex];
 
     bool hit = Find(tag, setIndex);
 
     // on miss, loads always allocate, stores optionally
     if ((!hit) && (accessType == ACCESS_TYPE_LOAD || STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE))
     {
         Replace(tag, setIndex);
         
     }
 
     _access[accessType][hit]++;
 
     return hit;
 }
 /*!
  *  @return true if accessed cache line hits
  */
//  template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION > void CACHE< SET, MAX_SETS, STORE_ALLOCATION >::Flush()
//  {
//      for (INT32 index = NumSets(); index >= 0; index--)
//      {
//         //  SET& set = _sets[index];
//         //  set.Flush();
//      }
//      IncFlushCounter();
//  }
 
//  template< class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION > void CACHE< SET, MAX_SETS, STORE_ALLOCATION >::ResetStats()
//  {
//      for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
//      {
//          _access[accessType][false] = 0;
//          _access[accessType][true]  = 0;
//      }
//      IncResetCounter();
//  }
 // extern UL2::CACHE ul2;
 
 // define shortcuts
 #define CACHE_DIRECT_MAPPED(MAX_SETS, ALLOCATION) CACHE< CACHE_SET::DIRECT_MAPPED, MAX_SETS, ALLOCATION >
 #define CACHE_ROUND_ROBIN(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
     CACHE< CACHE_SET::ROUND_ROBIN< MAX_ASSOCIATIVITY >, MAX_SETS, ALLOCATION >
 #define CACHE_LEAST_RECENTLY_USED(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
     CACHE< CACHE_SET::LEAST_RECENTLY_USED< MAX_ASSOCIATIVITY >, MAX_SETS, ALLOCATION >
/*  
#define CACHE_VARIABLE_WAY(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
      CACHE< CACHE_SET::VARIABLE_WAY< MAX_ASSOCIATIVITY >, MAX_SETS, ALLOCATION >
*/
#define CACHE_VARIABLE_WAY(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION) \
     VWAY_CACHE< MAX_ASSOCIATIVITY, MAX_SETS, ALLOCATION >
 
 #endif // PIN_CACHE_H
 