#ifndef OMX_PROXY_COMMON_LOG
#define OMX_PROXY_COMMON_LOG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>


#include "OMX_IVCommon.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Core.h"
#include "OMX_TI_Index.h"
#include "OMX_TI_IVCommon.h"
#include "OMX_Component.h"
//Uncoment this to enable the logger
//#define DUMP_LOG_ENABLED

/**
*   types
*/
typedef struct _mask_to_func_t {
    unsigned int mask;
    const char * funcName;
}maskToFunc_t;

typedef struct _func_log_t{
    void * handle;
    unsigned long startTime;
    unsigned int enabledFuncMask;
    maskToFunc_t *funcLUT;
    const char * fileName;
    FILE * file;
}funcLog_t;

typedef enum {
    OMX_log_Event_Handling = 0x7F000000,     /// PROXY_EventHandler
    OMX_log_Empty_Buffer,        /// PROXY_EmptyBufferDone
    OMX_log_Fill_Buffer,        /// PROXY_FillBufferDone
    OMX_log_Empty_This_Buff,    /// PROXY_EmptyThisBuffer
    OMX_log_Fill_This_Buffer,   /// PROXY_FillThisBuffer
    OMX_log_Allocate_Buffer,    /// PROXY_AllocateBuffer
    OMX_log_Use_Buffer,         /// PROXY_UseBuffer
    OMX_log_Free_Buffer,        /// PROXY_FreeBuffer
//     OMX_log_CommandStateSet,
//     OMX_log_CommandFlush,
//     OMX_log_CommandPortDisable,
//     OMX_log_CommandPortEnable,
//     OMX_log_CommandMarkBuffer,
    OMX_log_Send_Command,       /// PROXY_SendCommand
    OMX_log_Get_State,          /// PROXY_GetState
    OMX_log_Callback_set,       /// PROXY_SetCallbacks
    OMX_log_Un_Map_Buffer_Ducati, /// RPC_MapBuffer_Ducati i UnMap
    OMX_log_Meta_Data_Host,         /// RPC_MapMetaData_Host i UnMap
    OMX_log_Image_video_Lines,      /// RPC_UTIL_GetNumLines
    OMX_log_max = 0XFFFFFFFF

} log_index_extendtion_t;

void log_init(void* handle, funcLog_t* L, maskToFunc_t* lut );
void log_RegisterEnterFn( void * handle, funcLog_t* L, unsigned int mask,int fn, int param_0, unsigned int param_1, int param_2, int param_3, int param_4);
void log_RegisterExitFn( void * handle, funcLog_t* L, unsigned int mask );
void log_dumpLog( funcLog_t * log);
void log_deinit(void* handle,  funcLog_t* L);
void dumpToBuffer(OMX_U32, void*, void*, void*, FILE*, int);

#ifdef DUMP_LOG_ENABLED
  #define LOG_INIT( H , L, LUT ) { L.fileName = #L; log_init( H, &L, LUT);}
  #define LOG_ENTER2( H, L, BIT_RAISED, P0, P1 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, 0,0,0 );
  #define LOG_ENTER3( H, L, BIT_RAISED, P0, P1, P2 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, (int)P2,0,0 );
  #define LOG_ENTER4( H, L, BIT_RAISED, P0, P1, P2, P3 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, (int)P2, (int)P3,0 );
  #define LOG_EXIT( H, L, BIT_RAISED ) log_RegisterExitFn( H, &L, 1 << BIT_RAISED );
  #define LOG_DEINIT( H, L) log_deinit( H, &L);
#else
  #define LOG_INIT( H , L, LUT )
  #define LOG_ENTER2( H, L, BIT_RAISED, P0, P1 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, 0,0,0 );
  #define LOG_ENTER3( H, L, BIT_RAISED, P0, P1, P2 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, (int)P2,0,0 );
  #define LOG_ENTER4( H, L, BIT_RAISED, P0, P1, P2, P3 ) log_RegisterEnterFn( H, &L, 1 << BIT_RAISED,BIT_RAISED, (int)P0, (unsigned int)P1, (int)P2, (int)P3,0 );
  #define LOG_EXIT( H, L, BIT_RAISED )
  #define LOG_DEINIT( H, L)
#endif /// DUMP_LOG_ENABLED

#endif /// OMX_PROXY_COMMON_LOG
                                                  
