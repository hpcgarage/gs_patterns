#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <map>
#include <set>
#include <sstream>
#include "pin.H"

#define MAXBYTES (1LL<<37) //32GiB
#define MAXTHREADS (256)

//threads
bool isActive[MAXTHREADS] = {false};

//pe
bool doTrace = true;
bool stopTrace = false;
// bool isROI = false; // REMOVED: We trace everything now
INT32 pid;
INT32 nfuncs = 0;
INT64 totalbytes = 0;
UINT64 ROI_A = 0;
UINT64 ROI_B = 0;
UINT64 Icnt = 0;
UINT64 Mcnt = 0;
UINT64 gIcnt = 0;
UINT64 gMcnt_global = 0;

// Lock to ensure thread safety when writing to cout, the set, and the map
PIN_LOCK output_lock;

// Map to store counts per function
std::map<std::string, u_int64_t> gs_count_map_all;

// NEW: Map to store source locations per function
std::map<std::string, std::set<std::string> > gs_func_locations;

#define PADSIZE 56 // 64 byte line size: 64-8
#define NBUFS (1024)
INT32 numThreads = 0;


//FROM DR SOURCE
//DR trace
typedef uintptr_t addr_t;

struct _trace_entry_t {
  unsigned short type; 
  unsigned short size;
  addr_t addr; 
}  __attribute__((packed));
typedef struct _trace_entry_t trace_entry_t;

FILE * fptrace;
trace_entry_t * ptrace = NULL; 
trace_entry_t btrace[NBUFS+1];

// a running count of the instructions
class thread_data_t {
  public:
    thread_data_t() : _count(0) {}
    UINT64 _count;
    UINT8 _pad[PADSIZE];
};

// key for accessing TLS storage in the threads. initialized once in main()
static TLS_KEY tls_key = INVALID_TLS_KEY;

INT32 read_range_file(const char * roi_file) {

  FILE * roi_p = NULL;
   
  roi_p = fopen(roi_file, "r");
  if (roi_p == NULL) {
    printf("PIN -- ERROR: Could not open %s!\n", roi_file);
    return -1;
  }

  fscanf(roi_p, "%lu %lu\n", &ROI_A, &ROI_B);
   
  fclose(roi_p);

  return 0;
   
}

//string tools
int startswith(const char *a, const char *b) {
  if(strncmp(b, a, strlen(b)) == 0)
    return 1;
  return 0;
}

//set active thread 0 on PE 0 only, ignore rest
VOID set_active_first_pe_thread() {
   
  INT32 aPid;
  CHAR pid_name[1024];
  THREADID threadid = PIN_ThreadId();
  FILE * fppid;
  struct dirent *de;  // Pointer for directory entry
  DIR *dr = NULL;  

  //Only thread ONE is active
  if (threadid != 0) {
    sleep(16);
    return;
     
  } else {
    isActive[threadid] = true;
  }

  //check current dir for existing tag files
  dr = opendir(".");  
  if (dr == NULL) {
    printf("PIN -- Could not open current directory" );
    PIN_ExitProcess(1);
  }
   
  while ((de = readdir(dr)) != NULL) {
     
    if ( startswith(de->d_name, "tag.")) {
      printf("PIN -- ERROR tag files exist. Remove all tag.*.pid files and rerun.\n");
      PIN_ExitProcess(1);
    }
  }  
  closedir(dr);
   
  // Open tag file to determine pid
  pid = PIN_GetPid();
  sprintf(pid_name, "tag.%d.pid", pid);
  fppid = fopen(pid_name, "w");
  if (fppid == NULL) {
    printf("PIN -- ERROR: Could not open %s for writing!\n", pid_name);
    PIN_ExitProcess(1);
  }
  fclose(fppid);
  sleep(15);
   
  dr = opendir(".");  
  if (dr == NULL) {
    printf("PIN -- Could not open current directory" );
    PIN_ExitProcess(1);
  }
   
  while ((de = readdir(dr)) != NULL) {
     
    if ( startswith(de->d_name, "tag.")) {

      sscanf(de->d_name, "tag.%d.pid", &aPid);
      if (pid > aPid) {
    doTrace = false;
    break;
      }
    }
  }  
  closedir(dr);
   
  sleep(15);

  remove(pid_name);

  return;

}


VOID PeStart(VOID* v) {  

  THREADID threadid = PIN_ThreadId();
  set_active_first_pe_thread();

  if (!isActive[threadid])
    return;

  if (!doTrace)
    return;
}

VOID ThreadStart(THREADID threadid, CONTEXT* ctxt, INT32 flags, VOID* v) {

  if (threadid == 0)
    isActive[threadid] = true;
   
  numThreads++;
  thread_data_t* tdata = new thread_data_t;
  if (PIN_SetThreadData(tls_key, tdata, threadid) == FALSE) {
    printf("ERROR: PIN_SetThreadData failed\n");
    PIN_ExitProcess(1);
  }

  //Only thread 0
  if (!isActive[threadid])
    return;

  //Only PE 0
  if (!doTrace) {
    return;
  }
             
}

// This function is called when the thread exits
VOID ThreadFini(THREADID threadIndex, const CONTEXT* ctxt, INT32 code, VOID* v) {
   
   
  thread_data_t* tdata = static_cast< thread_data_t* >(PIN_GetThreadData(tls_key, threadIndex));

  delete tdata;
     
  THREADID threadid= PIN_ThreadId();
  if (!isActive[threadid])
    return;
   
  if (!doTrace)
    return;

  FILE * fp;
  fp = fopen("inscount.out", "w");
  fprintf(fp, "Count %lu\n", gIcnt);
  fclose(fp);
   
  printf("PIN --    TOTAL Instrs        %lu\n", gIcnt);
  printf("PIN --    TOTAL G/S MemInstrs     %lu\n", gMcnt_global);
  printf("PIN --    File            inscount.out\n");
  printf("PIN -- \n");
  
  // LOCK to read the global map safely
  PIN_GetLock(&output_lock, threadIndex + 1);
  
  printf("PIN -- G/S Instructions per Function and Location:\n");
  printf("PIN -- ===================================================\n");
  
  // Iterate over the count map
  for (std::map<std::string, u_int64_t>::iterator it = gs_count_map_all.begin(); it != gs_count_map_all.end(); ++it) {
    std::string funcName = it->first;
    u_int64_t count = it->second;

    printf("PIN -- Function: %-60s (Count: %lu)\n", funcName.c_str(), count);

    // Check if we have locations for this function
    if (gs_func_locations.count(funcName)) {
        std::set<std::string>& locs = gs_func_locations[funcName];
        for (std::set<std::string>::iterator loc_it = locs.begin(); loc_it != locs.end(); ++loc_it) {
            printf("PIN --     Source: %s\n", loc_it->c_str());
        }
    } else {
        printf("PIN --     Source: (No debug symbols found)\n");
    }
    printf("PIN -- \n");
  }
  
  PIN_ReleaseLock(&output_lock);
  
  printf("PIN -- \n");

}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, USIZE bsize, THREADID threadid) {
        
  if (!doTrace)
    return;
   
  if (!isActive[threadid])
    return;
   
  if (stopTrace)
    return;
   
  // REMOVED check for isROI
     
  Mcnt++;
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, USIZE bsize, THREADID threadid) {
     
  if (!doTrace)
    return;
     
  if (!isActive[threadid])
    return;
   
  if (stopTrace)
    return;  
   
  // REMOVED check for isROI
     
  Mcnt++;
}

// REMOVED StartROI and StopROI functions
// REMOVED Routine function

VOID RecordInstr(VOID * ip, USIZE bsize, THREADID threadid) {
     
  if (!isActive[threadid])
    return;
   
  gIcnt++;
       
  if (!doTrace)
    return;  
   
  if (stopTrace)
    return;
   
  // REMOVED check for isROI, assume we count everything now
  Icnt++;

}

// Modified to accept ADDRINT ip and look up everything internally
VOID RecordMemScattered(IMULTI_ELEMENT_OPERAND* memOpInfo, THREADID threadid, ADDRINT ip) {
   
  if (!doTrace)
    return;
   
  if (!isActive[threadid])
    return;
       
  if (stopTrace)
    return;

  // --- Step 1: Resolve Function Name and Source Location Safely ---
  // We use the PIN client lock to safely access symbol and debug info
  std::string funcName = "invalid";
  INT32 column = 0;
  INT32 line = 0;
  std::string file = "";

  PIN_LockClient();
  
  RTN rtn = RTN_FindByAddress(ip);
  if (RTN_Valid(rtn)) {
      funcName = RTN_Name(rtn); // This copies the string value, which is safe
  }
  
  // Corrected: PIN_GetSourceLocation returns VOID in your version.
  PIN_GetSourceLocation(ip, &column, &line, &file);
  
  PIN_UnlockClient();
  // ---------------------------------------------------------------

  // --- Step 2: Update Global Counts and Store Location (Protected by Lock) ---
  PIN_GetLock(&output_lock, threadid + 1);

  gMcnt_global++;
  gs_count_map_all[funcName]++;

  // Store location if valid
  if (!file.empty()) {
      std::stringstream ss;
      ss << file << ":" << line;
      std::string loc = ss.str();
      
      // Insert into the set of locations for this function
      gs_func_locations[funcName].insert(loc);
  }
   
  for (UINT32 j = 0; j < memOpInfo->NumOfElements(); j++) {
    Mcnt++;
  }

  PIN_ReleaseLock(&output_lock);
  // ---------------------------------------------------------------
}



// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v) {
   
  // Instruments memory accesses using a predicated call, i.e.
  // the instrumentation is called iff the instruction will actually be executed.
     
   
  UINT32 memOperands = INS_MemoryOperandCount(ins);
   
  USIZE Isize;
   
  Isize = INS_Size (ins);
   
  // REMOVED incorrect RTN_Name lookup here to avoid dangling pointer
   
  INS_InsertPredicatedCall(
               ins, IPOINT_BEFORE, (AFUNPTR)RecordInstr,
               IARG_INST_PTR,
               IARG_UINT32, Isize,
               //IARG_PTR, name,
               IARG_THREAD_ID,
               IARG_END);
   
  if (INS_HasScatteredMemoryAccess(ins) ) {
    
    if (INS_IsValidForIarg(ins, IARG_MULTI_ELEMENT_OPERAND)) {
    
        for (UINT32 op=0; op < INS_OperandCount(ins); op++) {
       
            if (INS_OperandIsMemory(ins, op) &&         // Skip register operands
                INS_OperandElementCount(ins, op) > 1) {  // Operand must have elements

          INS_InsertCall( ins, IPOINT_BEFORE, (AFUNPTR)RecordMemScattered,
                  IARG_MULTI_ELEMENT_OPERAND, op,
                  IARG_THREAD_ID,
                  // IARG_PTR, name, // REMOVED: Name is resolved inside analysis
                  IARG_INST_PTR,  // Passing the IP to look up everything
                  IARG_END);
            }
        }
    }     
    return;
  }
   
  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
    
    UINT32 Msize = INS_MemoryOperandSize(ins, memOp);
    
    if (INS_MemoryOperandIsWritten(ins, memOp)) {
      INS_InsertPredicatedCall(
               ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
               IARG_INST_PTR,
               IARG_MEMORYOP_EA, memOp,
               IARG_UINT32, Msize,
               //IARG_ADDRINT, name,
               IARG_THREAD_ID,
               IARG_END);
    }
    
    if (INS_MemoryOperandIsRead(ins, memOp)) {
      INS_InsertPredicatedCall(
               ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
               IARG_INST_PTR,
               IARG_MEMORYOP_EA, memOp,
               IARG_UINT32, Msize,
               //IARG_ADDRINT, name,
               IARG_THREAD_ID,
               IARG_END);
    }
    
  }
}


// Pin calls this function at the end
VOID Fini(INT32 code, VOID *v) {
  //*OutFile << "Total number of threads = " << numThreads << endl;
  //fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[]) {
   
  // Initialize symbol table code, needed for rtn instrumentation
  PIN_InitSymbols();
   
  // Usage
  if (PIN_Init(argc, argv))
    return Usage();

  // Initialize the lock for our print buffer
  PIN_InitLock(&output_lock);

  tls_key = PIN_CreateThreadDataKey(NULL);
  if (tls_key == INVALID_TLS_KEY) {
    printf("number of already allocated keys reached the MAX_CLIENT_TLS_KEYS limit\n");
    PIN_ExitProcess(1);
  }
   
  PeStart(NULL);
   
  // Register ThreadStart to be called when a thread starts.
  PIN_AddThreadStartFunction(ThreadStart, NULL);

  // Register Fini to be called when thread exits.
  PIN_AddThreadFiniFunction(ThreadFini, NULL);
   
  // Register Fini to be called when the application exits.
  PIN_AddFiniFunction(Fini, NULL);
    
  // Add instrument functions
  INS_AddInstrumentFunction(Instruction, 0);

  // REMOVED RTN_AddInstrumentFunction(Routine, 0); since we don't need to find markers anymore
   
  // Never returns
  PIN_StartProgram();
   
  return 0;
}
