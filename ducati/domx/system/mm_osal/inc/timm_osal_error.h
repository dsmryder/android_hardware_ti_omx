/* ====================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 * (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
 *
 * Use of this software is controlled by the terms and conditions found
 * in the license agreement under which this software has been supplied.
 * ==================================================================== */
/*
*  @file timm_timm_osal_error.h
*  The osal header file defines the error codes
*  @path
*
*/
/* -------------------------------------------------------------------------- */
/* =========================================================================
 *!
 *! Revision History
 *! ===================================
 *! 0.1: Created the first draft version, ksrini@ti.com
 * ========================================================================= */

#ifndef _TIMM_OSAL_ERROR_H_
#define _TIMM_OSAL_ERROR_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#include "timm_osal_types.h"

/** A unique ID for each component*/
	typedef TIMM_OSAL_U16 TIMM_OSAL_COMPID;

/** TIMM_OSAL_ERROR is a 32 bits unsigned integer.
 *   Each error code can be broken up into three fields as given below:
 * - Type of error (2 bits): NO_ERROR: 00, WARNING: 01, FATAL_ERROR: 10
 * - Component ID (14 bits): A unique ID which indicates which of the component generated the error
 * - Error ID (16 bits): The specific error generated by a component
 */
	typedef TIMM_OSAL_U32 TIMM_OSAL_ERRORTYPE;

#define TIMM_OSAL_OK  0
#define TIMM_OSAL_WAR 1
#define TIMM_OSAL_ERR 2


/* Macro to process TIMM_OSAL_ERROR */

/** This macro tests if the provided M4OSA_ERR is a warning or not*/
#define TIMM_OSAL_IS_WARNING(error)   ((((error)>>30) == TIMM_OSAL_WAR) ? 1:0)

/** This macro tests if the provided M4OSA_ERR is a fatal error or not*/
#define TIMM_OSAL_IS_ERROR(error)   ((((error)>>30) == TIMM_OSAL_ERR) ? 1:0)

/** This macro returns an error code accroding to the 3 provided fields:
  * @arg Type: (IN) [TIMM_OSAL_U32] Type of error to put in the error code
  * @arg compID: (IN) [TIMM_OSAL_U32] CompID to put in the error code
  * @arg errorID: (IN) [TIMM_OSAL_U32] ErrorID to put in the error code*/
#define TIMM_OSAL_ERR_CREATE(type, compID, errorID)\
   (((type)<<30)+(((compID)&0x003FFF)<<16)+((errorID)&0x00FFFF))

/** This macro extracts the 3 fields from the error:
  * @arg error: (IN) [TIMM_OSAL_ERRORTYPE] Error code
  * @arg type: (OUT) [TIMM_OSAL_U32] Type of error in the error code
  * @arg compID: (OUT) [TIMM_OSAL_U32] CompID to put in the error code
  * @arg errorID: (OUT) [TIMM_OSAL_U32] ErrorID to put in the error code*/
#define TIMM_OSAL_ERR_SPLIT(error, type, compID, errorID)\
   { type=(TIMM_OSAL_U32)((error)>>30);\
     compID=(TIMM_OSAL_U32)(((error)>>16)&0x003FFF);\
     (TIMM_OSAL_U32)(errorID=(error)&0x00FFFF); }

/* Component IDs */
#define TIMM_OSAL_COMP_GENERAL      0x00
#define TIMM_OSAL_COMP_MEMORY       0x01
#define TIMM_OSAL_COMP_PIPES        0x02
#define TIMM_OSAL_COMP_EVENTS       0x03
#define TIMM_OSAL_COMP_SEMAPHORES   0x04
#define TIMM_OSAL_COMP_TASK         0x05

/* Definition of common error codes */
/** there is no error*/
#define TIMM_OSAL_ERR_NONE                 ((TIMM_OSAL_ERRORTYPE) 0x00000000)


/** There is no more memory available*/
#define TIMM_OSAL_ERR_ALLOC                ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000001))
#define TIMM_OSAL_ERR_OUT_OF_RESOURCE      ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000002))

/** Time out */
#define TIMM_OSAL_WAR_TIMEOUT              ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_WAR,TIMM_OSAL_COMP_GENERAL,0x000003))
#define TIMM_OSAL_ERR_PARAMETER            ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000004))
#define TIMM_OSAL_ERR_NOT_READY            ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000005))
#define TIMM_OSAL_ERR_OMX                  ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000006))
#define TIMM_OSAL_ERR_PIPE_FULL            ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000007))
#define TIMM_OSAL_ERR_PIPE_EMPTY           ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000008))
#define TIMM_OSAL_ERR_PIPE_DELETED         ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000009))
#define TIMM_OSAL_ERR_PIPE_RESET           ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000A))
#define TIMM_OSAL_ERR_GROUP_DELETED        ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000B))
#define TIMM_OSAL_ERR_UNKNOWN              ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000C))


#define TIMM_OSAL_ERR_SEM_CREATE_FAILED    ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_SEMAPHORE,0x000001))

/*Added during Linux Porting*/
#define TIMM_OSAL_ERR_NO_PERMISSIONS		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000D))
#define TIMM_OSAL_ERR_RESOURCE_EXISTS		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000E))
#define TIMM_OSAL_ERR_RESOURCE_REMOVED		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x00000F))
#define TIMM_OSAL_ERR_SYSTEM_LIMIT_EXCEEDED	((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000010))
#define TIMM_OSAL_ERR_NOT_SUPPORTED		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000011))
#define TIMM_OSAL_ERR_SIGNAL_CAUGHT		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000012))
#define TIMM_OSAL_ERR_TIMEOUT              ((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_GENERAL,0x000013))



#define TIMM_OSAL_COMP_MSG_Q        0x06
#define TIMM_OSAL_ERR_MSG_SIZE_MISMATCH		((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_MSG_Q,0x000001))
#define TIMM_OSAL_ERR_MSG_TYPE_NOT_FOUND	((TIMM_OSAL_ERRORTYPE) TIMM_OSAL_ERR_CREATE(TIMM_OSAL_ERR,TIMM_OSAL_COMP_MSG_Q,0x000002))


/*
#define GOTO_EXIT_IF(_Cond,_ErrorCode) { \
    if ((_Cond)) { \
        status = _ErrorCode; \
        printf ("Error :: %s : %s : %d :: Exiting because : %s\n", \
                __FILE__, __FUNCTION__, __LINE__, #_Cond); \
        goto EXIT; \
    } \
}
*/

#define SWITCH_CASE(_Case, _ErrCode, _ErrMsg)\
	case _Case:\
		TIMM_OSAL_Error(_ErrMsg);\
		bReturnStatus = _ErrCode;\
		break;

#define SWITCH_DEFAULT_CASE(_ErrCode, _ErrMsg )\
	default:\
		TIMM_OSAL_Error(_ErrMsg);\
		bReturnStatus = _ErrCode;\
		break;



#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /*_TIMM_OSAL_ERROR_H_*/
