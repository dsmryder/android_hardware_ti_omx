/* ====================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 * (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
 *
 * Use of this software is controlled by the terms and conditions found
 * in the license agreement under which this software has been supplied.
 * ==================================================================== */


/** OMX_Core.h
 *  The OMX_Core header file contains the definitions used for PV core
 */

#ifndef OMX_TI_Core_h
#define OMX_TI_Core_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header shall include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully
 */

#include <OMX_Core.h>
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_Init(void);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_Deinit(void);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent);
OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole (
	OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames);
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput);
OMX_API OMX_ERRORTYPE   OMX_APIENTRY  TIOMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
