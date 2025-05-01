#include <iostream>
#include <fstream>
#include <unordered_map>
#include "pin.H"
#include <fstream>
#include <ctime>
#include "eaf.H"
extern EAF ul3_eaf; 


typedef UINT64 CACHE_STATS; // type of cache hit/miss counters

#include "pin_cache.H"

/* ------------------------------------------------------ */
/* 1.  NEW GLOBAL STATISTICS                              */
/* ------------------------------------------------------ */
static UINT64 totalAccesses   = 0;        // ADDED – every L1/L2/L3 access
static UINT64 eafAccesses     = 0;        // ADDED – calls that query the EAF
static UINT64 instCount       = 0;        // ADDED – dynamic instructions retired
static UINT64 cycleCount      = 0;        // ADDED – very simple “cycle” model

/* ------------------------------------------------------ */
/* 2.  VERY ROUGH LATENCY MODEL (tweak as you like)       */
/* ------------------------------------------------------ */
const UINT32 LAT_IL1 = 2;     // 1 cycle hit
const UINT32 LAT_DL1 = 3;
const UINT32 LAT_EAF = 2; //2 cycles (1 cycle for parallel hash & bit read + 1 cycle to merge with L3 insertion logic)
const UINT32 LAT_UL2_HIT = 10; // ± real µ-arch
const UINT32 LAT_UL2_MISS = 40;  // miss ⇒ go to DRAM (or UL3 in your flow)
/* ------------------------------------------------------ */



namespace IL1
{
// 1st level instruction cache: 32 kB, 32 B lines, 32-way associative
const UINT32 cacheSize                         = 16 * KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 2;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

//  typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
typedef CACHE_LEAST_RECENTLY_USED(max_sets, associativity, allocation) CACHE;
} // namespace IL1
static IL1::CACHE il1("L1 Instruction Cache", IL1::cacheSize, IL1::lineSize, IL1::associativity);

namespace DL1
{
// 1st level data cache: 32 kB, 32 B lines, 32-way associative
const UINT32 cacheSize                         = 16 * KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 2;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets          = cacheSize / (lineSize * associativity);
const UINT32 max_associativity = associativity;

//  typedef CACHE_ROUND_ROBIN(max_sets, max_associativity, allocation) CACHE;
typedef CACHE_LEAST_RECENTLY_USED(max_sets, associativity, allocation) CACHE;
} // namespace DL1
static DL1::CACHE dl1("L1 Data Cache", DL1::cacheSize, DL1::lineSize, DL1::associativity);

namespace UL2
{
// 2nd level unified cache: 2 MB, 64 B lines, direct mapped
const UINT32 cacheSize                         = 256* KILO;
const UINT32 lineSize                          = 64;
const UINT32 associativity                     = 8;
const CACHE_ALLOC::STORE_ALLOCATION allocation = CACHE_ALLOC::STORE_ALLOCATE;

const UINT32 max_sets = cacheSize / (lineSize * associativity);

// typedef CACHE_DIRECT_MAPPED(max_sets, allocation) CACHE;
// typedef CACHE_ROUND_ROBIN(max_sets, associativity, allocation) CACHE;
typedef CACHE_LEAST_RECENTLY_USED(max_sets, associativity, allocation) CACHE;
// typedef CACHE_VARIABLE_WAY(max_sets, associativity, allocation) CACHE;

} // namespace UL2
// VWAY_EAF_CACHE ul2("L2 Unified Cache", UL2::cacheSize, UL2::lineSize, UL2::associativity);
static UL2::CACHE ul2("L2 Unified Cache", UL2::cacheSize, UL2::lineSize, UL2::associativity);

EAF ul3_eaf( UL2::cacheSize / UL2::lineSize , /*alpha=*/8 );  

//  // Define Data-Store
DATA data_array (UL2::cacheSize, UL2::lineSize);

static VOID Fini(int code, VOID* v)
{
    /*std::ofstream out ("mycache.out");
    out << "PIN:MEMLATENCIES 1.0. 0x0\n";

    out << "#\n# L1 Instruction Cache stats\n";
    out << "# IL1 Cache Size           : "<<IL1::cacheSize<<"\n";
    out << "# IL1 Line Size            : "<<IL1::lineSize<<"\n";
    out << "# IL1 Associativity        : "<<IL1::associativity<<"\n";
    out << "# IL1 Replacement Policy   : LRU\n";

    out << "#\n# L1 Data Cache stats\n";
    out << "# DL1 Cache Size           : "<<DL1::cacheSize<<"\n";
    out << "# DL1 Line Size            : "<<DL1::lineSize<<"\n";
    out << "# DL1 Associativity        : "<<DL1::associativity<<"\n";
    out << "# DL1 Replacement Policy   : LRU\n";

    out << "#\n# L2 Unified Cache stats\n";
    out << "# UL2 Cache Size           : "<<UL2::cacheSize<<"\n";
    out << "# UL2 Line Size            : "<<UL2::lineSize<<"\n";
    out << "# UL2 Associativity        : "<<UL2::associativity<<"\n";
    out << "# UL2 Replacement Policy   : LRU\n";

    out <<"\n####################################################################\n";
    out << il1;
    out << dl1;
    out << ul2;*/
    //static VOID Fini(int code, VOID* v)

   // Generate timestamped filename with BASELRU prefix
   time_t now = time(nullptr);
   char filename[80];  // Increased size to accommodate the prefix
   strftime(filename, sizeof(filename), "eaf_lru_CPI-%Y-%m-%d_%H-%M-%S.out", localtime(&now));

   // Open file and dump stats
   std::ofstream out(filename);
   if (!out) return;
   out << il1;
   out << dl1;
   out << ul2;
   //out << ul3;

    // EXTRA:
    out << "\n################  EXTRA STATS  ################\n";
    out << "Total cache-related accesses      : " << totalAccesses << '\n';   // ADDED
    out << "EAF filter accesses         : " << eafAccesses   << '\n';   // ADDED
    out << "Dynamic instructions retired     : " << instCount      << '\n';   // ADDED
    out << "Approximate cycles               : " << cycleCount     << '\n';   // ADDED
    if (instCount)
        out << "Approximate CPI                 : "
            << std::fixed << std::setprecision(2)
            << static_cast<double>(cycleCount) / instCount << '\n';           // ADDED

}

   //  out << ul3;


/* ------------------------------------------------------ */
/* 3.  BUMP TOTAL-ACCESS COUNTER EARLIEST IN THE STACK    */
/* ------------------------------------------------------ */

static VOID Ul2Access(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
   // second level unified cache
   //  const BOOL ul2Hit = ul2.Access(addr, size, accessType);
   //std::cout<<"Ul2Access Called on address = "<<addr<<"\n"<<std::flush;
//    ul2.Access(addr, size, accessType);
//    ul2.UL3AccessEAF(addr, size, accessType);
   //  // third level unified cache
   //  if (!ul2Hit) ul3.Access(addr, size, accessType);


//NEW
totalAccesses++;                          // ADDED
eafAccesses++;                            // ADDED – the call always consults EAF

const BOOL hit = ul2.UL3AccessEAF(addr, size, accessType);

/* crude timing model */
cycleCount += hit ? LAT_UL2_HIT : LAT_UL2_MISS;      // ADDED

/* 2) pay the EAF probe latency **only** on a miss */
if (!hit)
cycleCount += LAT_EAF;  // NEW

}


/* ------------------------------------------------------ */
/* 4.  FRONT-END & BACK-END INSTRUMENTATION               */
/* ------------------------------------------------------ */

static VOID InsRef(ADDRINT addr)
{

    instCount++;                              // ADDED
    cycleCount += LAT_IL1;                    // ADDED   (I-cache assumed hit)

    const UINT32 size                        = 1; // assuming access does not cross cache lines
    const CACHE_BASE::ACCESS_TYPE accessType = CACHE_BASE::ACCESS_TYPE_LOAD;

   //  // ITLB
   //  itlb.AccessSingleLine(addr, accessType);

    // first level I-cache
    const BOOL il1Hit = il1.AccessSingleLine(addr, accessType);

    // second level unified Cache
    if (!il1Hit) Ul2Access(addr, size, accessType);
}

static VOID MemRefMulti(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
   //  // DTLB
   //  dtlb.AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD);

    // first level D-cache
    const BOOL dl1Hit = dl1.Access(addr, size, accessType);

    // second level unified Cache
    if (!dl1Hit) Ul2Access(addr, size, accessType);
}

static VOID MemRefSingle(ADDRINT addr, UINT32 size, CACHE_BASE::ACCESS_TYPE accessType)
{
   //  // DTLB
   //  dtlb.AccessSingleLine(addr, CACHE_BASE::ACCESS_TYPE_LOAD);

    // first level D-cache

    totalAccesses++;                          // ADDED

    const BOOL dl1Hit = dl1.AccessSingleLine(addr, accessType);

    cycleCount += dl1Hit ? LAT_DL1 : 0;       // ADDED  (latency if hit; miss handled in Ul2Access)

    // second level unified Cache
    if (!dl1Hit) Ul2Access(addr, size, accessType);
}

static VOID Instruction(INS ins, VOID* v)
{
    // all instruction fetches access I-cache
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InsRef, IARG_INST_PTR, IARG_END);

    if (!INS_IsStandardMemop(ins)) return;
    if (INS_MemoryOperandCount(ins) == 0) return;
    ;

    UINT32 readSize = 0, writeSize = 0;
    UINT32 readOperandCount = 0, writeOperandCount = 0;

    for (UINT32 opIdx = 0; opIdx < INS_MemoryOperandCount(ins); opIdx++)
    {
        if (INS_MemoryOperandIsRead(ins, opIdx))
        {
            readSize = INS_MemoryOperandSize(ins, opIdx);
            readOperandCount++;
            break;
        }
        if (INS_MemoryOperandIsWritten(ins, opIdx))
        {
            writeSize = INS_MemoryOperandSize(ins, opIdx);
            writeOperandCount++;
            break;
        }
    }

    if (readOperandCount > 0)
    {
        const AFUNPTR countFun = (readSize <= 4 ? (AFUNPTR)MemRefSingle : (AFUNPTR)MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, countFun, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_UINT32,
                                 CACHE_BASE::ACCESS_TYPE_LOAD, IARG_END);
    }

    if (writeOperandCount > 0)
    {
        const AFUNPTR countFun = (writeSize <= 4 ? (AFUNPTR)MemRefSingle : (AFUNPTR)MemRefMulti);

        // only predicated-on memory instructions access D-cache
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, countFun, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_UINT32,
                                 CACHE_BASE::ACCESS_TYPE_STORE, IARG_END);
    }
}

extern int main(int argc, char* argv[])
{
    PIN_Init(argc, argv);

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0; // make compiler happy
}
