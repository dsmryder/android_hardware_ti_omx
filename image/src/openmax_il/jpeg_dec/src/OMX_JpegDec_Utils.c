
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */

/**
* @file OMX_JpegDec_Utils.c
*
* This file implements OMX Component for JPEG decoder
*
* @patth $(CSLPATH)\jpeg_dec\src\OMX_JpegDec_Utils.c
*
* @rev 0.2
*/


/****************************************************************
 *  INCLUDE FILES
*****************************************************************/

/* -----------System and Platform Files ------------------------*/

#ifdef UNDER_CE
    #include <windows.h>
    #include <oaf_osal.h>
    #include <omx_core.h>
    #include <stdlib.h>
#else
    #include <wchar.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dlfcn.h>
    #include <malloc.h>
    #include <memory.h>
    #include <fcntl.h>
    #include <sched.h>
#endif


    #include <dbapi.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <pthread.h>

/*--------------------- Program Header Files ----------------------------*/

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_Image.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_Other.h>
#include "OMX_JpegDec_Utils.h"
#include <usn.h>

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif

#define JPEGDEC_TIMEOUT 10

#ifdef UNDER_CE
    HINSTANCE g_hLcmlDllHandle = NULL;
#endif

OMX_ERRORTYPE LCML_CallbackJpegDec(TUsnCodecEvent event,
                                   void * args [10]);


/*------------------------- Function Implementation ------------------*/

/* ========================================================================== */
/**
 * @fn GetLCMLHandleJpegDec - Implements the functionality to get LCML handle
 * @param pComponent - components private structure
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE GetLCMLHandleJpegDec(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifndef UNDER_CE
    OMX_HANDLETYPE LCML_pHandle;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    jpegdec_fpo fpGetHandle;
    void *handle = NULL;
    char *error = NULL;

    handle = dlopen("libLCML.so", RTLD_LAZY);
    if (!handle) {
        if ((error = dlerror()) != NULL) {
            fputs(error, stderr);
        }
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    fpGetHandle = dlsym(handle, "GetHandle");

    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        eError = OMX_ErrorInvalidComponent;
        goto EXIT;
    }

    /*calling gethandle and passing phandle to be filled */
    eError = (*fpGetHandle)(&LCML_pHandle);
    if (eError != OMX_ErrorNone) {
        eError = OMX_ErrorUndefined;
        JPEGDEC_DPRINT("eError != OMX_ErrorNone... in (*fpGetHandle)(&LCML_pHandle);\n");
        goto EXIT;
    }

    pComponentPrivate->pDllHandle = handle;
    pComponentPrivate->pLCML = (void *)LCML_pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pComponentPrivate;

EXIT:
#else
    typedef OMX_ERRORTYPE (*LPFNDLLFUNC1)(OMX_HANDLETYPE);
    LPFNDLLFUNC1 fpGetHandle1;
    OMX_HANDLETYPE LCML_pHandle = NULL;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    g_hLcmlDllHandle = LoadLibraryEx(TEXT("OAF_BML.dll"), NULL, 0);
    
    if (g_hLcmlDllHandle == NULL) 
    {
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    fpGetHandle1 = (LPFNDLLFUNC1)GetProcAddress(g_hLcmlDllHandle,TEXT("GetHandle"));
    if (!fpGetHandle1) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        eError = OMX_ErrorComponentNotFound;
        goto EXIT;
    }

    eError = fpGetHandle1(&LCML_pHandle);
    if (eError != OMX_ErrorNone) {
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
        eError = OMX_ErrorUndefined;
        LCML_pHandle = NULL;
        goto EXIT;
    }

    (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML = (LCML_DSP_INTERFACE *)LCML_pHandle;
    pComponentPrivate->pLCML->pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pComponentPrivate;

EXIT:
#endif
    return eError;

}   /* End of GetLCMLHandle */

/* ========================================================================== */
/**
 * @fn DisablePortJpegDec - Implements the functionality to disable the ports
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE DisablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                 OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CHECK_PARAM(pComponentPrivate);

    JPEGDEC_DPRINT("In DisablePortJpegDec %d\n", nParam1);

    if (pComponentPrivate->nCurState == OMX_StateExecuting || pComponentPrivate->nCurState == OMX_StatePause) {
        if ((nParam1 == 0) || (nParam1 == 1) || (nParam1 == -1)) {
            eError = HandleInternalFlush(pComponentPrivate, nParam1);
        }
    }

EXIT:
    return eError;
}   /* End of DisablePort */



/* ========================================================================== */
/**
 * @fn EnablePortJpegDec - Implements the functionality to enable the ports
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE EnablePortJpegDec(JPEGDEC_COMPONENT_PRIVATE* pComponentPrivate,
                                OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CHECK_PARAM(pComponentPrivate);

    while (1) {
        if ((nParam1 == 0x0) &&
            ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
             (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_INPUT_PORT,
                                                   NULL);
            break;
        }
        else if ((nParam1 == 0x1) &&
                 ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                  (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_OUTPUT_PORT,
                                                   NULL);
            break;
        }
        else if ((nParam1 == -1) &&
                ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                 ((pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated) &&
                  (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated)))) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_INPUT_PORT,
                                                   NULL);

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandPortEnable,
                                                   JPEGDEC_OUTPUT_PORT,
                                                   NULL);
            break;

        }
        else {
                JPEGDEC_WAIT_PORT_POPULATION(pComponentPrivate);
        }
    }
    JPEGDEC_DPRINT("Exiting EnablePortJpegDec(), Ports are enabled if no error\n");
EXIT:
    return eError;
}   /* End of EnablePort */


/* ========================================================================== */
/**
 * @fn Start_ComponentThreadJpegDec - Implements the functionality to start
 *  the component thread. Creates data pipes, commmand pipes and initializes
 *  Component thread.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Start_ComponentThreadJpegDec(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
#ifdef UNDER_CE
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__schedparam.__sched_priority = OMX_IMAGE_DECODER_THREAD_PRIORITY;
#endif

    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* create the pipe used to maintain free output buffers*/
    eError = pipe (pComponentPrivate->nFree_outBuf_Q);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to maintain filled input buffers*/
    eError = pipe (pComponentPrivate->nFilled_inpBuf_Q);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->nCmdPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* create the pipe used to send commands to the thread */
    eError = pipe (pComponentPrivate->nCmdDataPipe);
    if (eError) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    JPEGDEC_DPRINT ("JPEG Start_ComponentThread\n");
    /* Create the Component Thread */
#ifdef UNDER_CE
    eError = pthread_create (&(pComponentPrivate->pComponentThread), &attr, OMX_JpegDec_Thread, pComponent);
#else
    eError = pthread_create (&(pComponentPrivate->pComponentThread), NULL, OMX_JpegDec_Thread, pComponent);
#endif

    if (eError || !pComponentPrivate->pComponentThread) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF,
                       pComponentPrivate->pComponentThread,
                       PERF_FOURS("JPDT"));
#endif

EXIT:
    return eError;
}   /* End of Start_ComponentThreadJpegDec */



/* ========================================================================== */
/**
 * @fn Free_ComponentResourcesJpegDec - Implements the functionality to de-init
 *  the component thread. close component thread, Command pipe, data pipe &
 *  LCML pipe.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Free_ComponentResourcesJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError        = OMX_ErrorNone;
    OMX_ERRORTYPE threadError   = OMX_ErrorNone;
    OMX_ERRORTYPE eErr          = OMX_ErrorNone;
    int pthreadError = 0, nRet = 0;
    OMX_U8 nCount = 0;
    OMX_COMMANDTYPE eCmd = OMX_CustomCommandStopThread;

    if (!pComponentPrivate) {
        JPEGDEC_DPRINT("pComponentPrivate is NULL.\n");
        goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

    JPEGDEC_DPRINT ("Inside Free_ComponentResourcesJpegDec \n");
    /* should clean up in case of crash - implement*/



    if (pComponentPrivate->nIsLCMLActive ==1) {
        JPEGDEC_DPRINT ("EMMCodecControlDestroy inside Free_ComponentResourcesJpegDec\n");
        LCML_ControlCodec(((LCML_DSP_INTERFACE*)pComponentPrivate->pLCML)->pCodecinterfacehandle,EMMCodecControlDestroy,NULL);
        pComponentPrivate->nIsLCMLActive = 0;
#ifdef UNDER_CE
        FreeLibrary(g_hLcmlDllHandle);
        g_hLcmlDllHandle = NULL;
#else
        dlclose(pComponentPrivate->pDllHandle);
        pComponentPrivate->pDllHandle = NULL;
#endif
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        eCmd, 0, PERF_ModuleComponent);
#endif

    JPEGDEC_DPRINT("Freeing resources\n");

    nRet = write(pComponentPrivate->nCmdPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
        JPEGDEC_DPRINT("Error while writing into nCmdPipe\n");
        eError = OMX_ErrorHardware;
    }

    nRet = write(pComponentPrivate->nCmdDataPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
                     JPEGDEC_DPRINT("Error while writing into nCmdDataPipe\n");
        eError = OMX_ErrorHardware;
    }

    pthreadError = pthread_join(pComponentPrivate->pComponentThread, (void*)&threadError);
    if (0 != pthreadError)    {
        eError = OMX_ErrorHardware;
        JPEGDEC_DPRINT ("Error while closing Component Thread\n");
    }

    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        JPEGDEC_DPRINT("OMX_ErrorInsufficientResources\n");
        eError = OMX_ErrorInsufficientResources;
        JPEGDEC_DPRINT ("Error while closing Component Thread\n");
    }

    /* close the data pipe handles*/
    eErr = close(pComponentPrivate->nFree_outBuf_Q[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        JPEGDEC_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFilled_inpBuf_Q[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        JPEGDEC_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFree_outBuf_Q[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        JPEGDEC_DPRINT ("Error while closing data pipe\n");
    }

    eErr = close(pComponentPrivate->nFilled_inpBuf_Q[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
       JPEGDEC_DPRINT ("Error while closing data pipe\n");
    }

    /*Close the command pipe handles*/
    eErr = close(pComponentPrivate->nCmdPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
      JPEGDEC_DPRINT ("Error while closing cmd pipe\n");
    }

    eErr = close(pComponentPrivate->nCmdPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
      JPEGDEC_DPRINT ("Error while closing cmd pipe\n");
    }

    /*Close the command data pipe handles*/
    eErr = close(pComponentPrivate->nCmdDataPipe[0]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
      JPEGDEC_DPRINT ("Error while closing cmd pipe\n");
    }

    eErr = close(pComponentPrivate->nCmdDataPipe[1]);
    if (0 != eErr && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
      JPEGDEC_DPRINT ("Error while closing cmd pipe\n");
    }

    /* Free Resources */
#ifdef KHRONOS_1_1

    if (pComponentPrivate->pAudioPortType)
        {
            free(pComponentPrivate->pAudioPortType);
            pComponentPrivate->pAudioPortType = NULL;
        }

    if (pComponentPrivate->pVideoPortType)
        {
            free(pComponentPrivate->pVideoPortType);
            pComponentPrivate->pVideoPortType = NULL;
        }
    if (pComponentPrivate->pOtherPortType)
        {
            free(pComponentPrivate->pOtherPortType);
            pComponentPrivate->pOtherPortType = NULL;
        }
    if (pComponentPrivate->pCompRole)
        {
            free(pComponentPrivate->pCompRole);
            pComponentPrivate->pCompRole = NULL;
        }
    if (pComponentPrivate->pQuantTable)
        {
            free(pComponentPrivate->pQuantTable);
            pComponentPrivate->pQuantTable = NULL;
        }
    if (pComponentPrivate->pHuffmanTable)
        {
            free(pComponentPrivate->pHuffmanTable);
            pComponentPrivate->pHuffmanTable = NULL;
        }

#endif 

    if(pComponentPrivate->cComponentName){
        free(pComponentPrivate->cComponentName);
        pComponentPrivate->cComponentName = NULL;
    }
    
    for (nCount = 0; nCount < NUM_OF_BUFFERS; nCount++) {
        if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount]) {
            free(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount]);
            pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount] = NULL;
        }
    }
    for (nCount = 0; nCount < NUM_OF_BUFFERS; nCount++) {
        if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount]) {
            free(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount]);
            pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount] = NULL;
        }
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier) {
        free(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pParamBufSupplier = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier) {
        free(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef)   {
        free (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef) {
        free (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat)    {
        free (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortFormat = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat)   {
        free (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortFormat = NULL;
    }

    if (pComponentPrivate->pPriorityMgmt) {
        free (pComponentPrivate->pPriorityMgmt);
        pComponentPrivate->pPriorityMgmt = NULL;
    }

    if (pComponentPrivate->pPortParamType) {
        free (pComponentPrivate->pPortParamType);
        pComponentPrivate->pPortParamType = NULL;
    }

    if (pComponentPrivate->pScalePrivate) {
        free(pComponentPrivate->pScalePrivate);
        pComponentPrivate->pScalePrivate = NULL;
    }

    if(pComponentPrivate->pSectionDecode) {
        free(pComponentPrivate->pSectionDecode);
        pComponentPrivate->pSectionDecode = NULL;
    }

    if(pComponentPrivate->pSubRegionDecode) {
        free(pComponentPrivate->pSubRegionDecode);
        pComponentPrivate->pSubRegionDecode = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]) {
        free(pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]);
        pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT] = NULL;
    }

    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT])    {
        free(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]);
        pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT] = NULL;
    }

     if (pthread_mutex_destroy(&(pComponentPrivate->mJpegDecMutex)) != 0){
        perror("Error with pthread_mutex_destroy");
    }

    if(pthread_cond_destroy(&(pComponentPrivate->sStop_cond)) != 0){
        perror("Error with pthread_cond_desroy");
    }


#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);
    PERF_Done(pComponentPrivate->pPERF);
#endif

    if (pComponentPrivate) {
        free(pComponentPrivate);
        pComponentPrivate = NULL;
    }

EXIT:
  JPEGDEC_DPRINT ("Exiting Successfully After Freeing All Resources Errror %x, \n", eError);
    return eError;
}   /* End of Free_ComponentResourcesJpegDec */



/* ========================================================================== */
/**
 * @fn Fill_LCMLInitParamsJpegDec - This function fills the create phase
 *  parameters used by DSP
 * @param lcml_dsp    handle for this instance of the LCML
 * @param arr[]       array with the parameters
 * @param pComponent  handle for this instance of component
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE Fill_LCMLInitParamsJpegDec(LCML_DSP *lcml_dsp,
                                                OMX_U16 arr[],
                                                OMX_HANDLETYPE pComponent)
{

    OMX_ERRORTYPE eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle  =  NULL;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate    = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut       = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn        = NULL;
    OMX_U16 nScaleFactor;


    OMX_CHECK_PARAM(pComponent);
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDefIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef;
    pPortDefOut = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef;

    lcml_dsp->In_BufInfo.DataTrMethod = DMM_METHOD;
    lcml_dsp->Out_BufInfo.DataTrMethod = DMM_METHOD;

    lcml_dsp->NodeInfo.nNumOfDLLs = OMX_JPEGDEC_NUM_DLLS;
    lcml_dsp->NodeInfo.AllUUIDs[0].uuid = (struct DSP_UUID *)&JPEGDSOCKET_TI_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[0].DllName,JPEG_DEC_NODE_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[0].eDllType = DLL_NODEOBJECT;

    lcml_dsp->NodeInfo.AllUUIDs[1].uuid = (struct DSP_UUID *)&JPEGDSOCKET_TI_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[1].DllName,JPEG_DEC_NODE_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[1].eDllType = DLL_DEPENDENT;

    lcml_dsp->NodeInfo.AllUUIDs[2].uuid =(struct DSP_UUID *) &USN_UUID;
    strcpy ((char *)lcml_dsp->NodeInfo.AllUUIDs[2].DllName,USN_DLL);
    lcml_dsp->NodeInfo.AllUUIDs[2].eDllType = DLL_DEPENDENT;

    lcml_dsp->DeviceInfo.TypeofDevice = 0;
    lcml_dsp->SegID = 0;
    lcml_dsp->Timeout = -1;
    lcml_dsp->Alignment = 0;
    lcml_dsp->Priority = 5;


    switch(pComponentPrivate->pScalePrivate->xWidth){
        case (0):
            nScaleFactor = 100;
            break;
        case (1):
            nScaleFactor = 50;
            break;
        case (2):
            nScaleFactor = 25;
            break;
        case (3):
            nScaleFactor = 13; /*12.5*/
            break;
        case (4):
            nScaleFactor = 200;
            break;
        case (5):
            nScaleFactor = 400;
            break;
        case (6):
            nScaleFactor = 800;
            break;
        default:
            nScaleFactor = 100;
            break;
    }

    if (pComponentPrivate->nProgressive == 1) {
        if (pPortDefIn->format.image.nFrameHeight <= 144 &&
            pPortDefIn->format.image.nFrameWidth<= 176) {
            lcml_dsp->ProfileID = 0;
        }
        else if (pPortDefIn->format.image.nFrameHeight <= 288 &&
            pPortDefIn->format.image.nFrameWidth<= 352) {
            lcml_dsp->ProfileID = 1;
        }
        else if (pPortDefIn->format.image.nFrameHeight <= 480 &&
            pPortDefIn->format.image.nFrameWidth <= 640) {
            lcml_dsp->ProfileID = 2;
        }
        else if (pPortDefIn->format.image.nFrameHeight<= 1024 &&
            pPortDefIn->format.image.nFrameWidth <= 1280) {
            lcml_dsp->ProfileID = 4;
        }
        else if (pPortDefIn->format.image.nFrameHeight <= 1200 &&
            pPortDefIn->format.image.nFrameWidth<= 1920) {
            lcml_dsp->ProfileID = 5;
        }
        else if (pPortDefIn->format.image.nFrameHeight<= 1536 &&
            pPortDefIn->format.image.nFrameWidth<= 2048) {
            lcml_dsp->ProfileID = 6;
        }
        else if (pPortDefIn->format.image.nFrameHeight<= 1600 &&
            pPortDefIn->format.image.nFrameWidth<= 2560) {
            lcml_dsp->ProfileID = 7;
        }
        else if (pPortDefIn->format.image.nFrameHeight <= 2048 &&
            pPortDefIn->format.image.nFrameWidth<= 2560) {
            lcml_dsp->ProfileID = 8;
        }
        else if (pPortDefIn->format.image.nFrameHeight <= 2048 &&
            pPortDefIn->format.image.nFrameWidth<= 3200) {
            lcml_dsp->ProfileID = 9;
        }
        else {
            lcml_dsp->ProfileID = 3;
        }
    }
    else if (pComponentPrivate->nProgressive == 0) {
        lcml_dsp->ProfileID = -1;
    }
    
    pComponentPrivate->nProfileID = lcml_dsp->ProfileID;

    /*filling create phase params*/
    arr[0] = JPGDEC_SNTEST_STRMCNT;
    arr[1] = JPGDEC_SNTEST_INSTRMID;
    arr[2] = 0;
    arr[3] = JPGDEC_SNTEST_INBUFCNT;
    arr[4] = JPGDEC_SNTEST_OUTSTRMID;
    arr[5] = 0;
    arr[6] = JPGDEC_SNTEST_OUTBUFCNT;

    if (pComponentPrivate->nProgressive == 1) {
        JPEGDEC_DPRINT("JPEGdec:: nProgressive IMAGE");
        arr[7] = pComponentPrivate->sMaxResolution.nHeight;
        arr[8] = pComponentPrivate->sMaxResolution.nWidth;
        arr[9] = JPGDEC_SNTEST_PROG_FLAG;
    }
    else {
		printf("****** Max Width %d Max Height %d\n",(int)pComponentPrivate->sMaxResolution.nWidth,(int)pComponentPrivate->sMaxResolution.nHeight);

        arr[7] = pComponentPrivate->sMaxResolution.nHeight;
        arr[8] = pComponentPrivate->sMaxResolution.nWidth;
        arr[9] = 0;
    }

    if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_FormatCbYCrY) {
        arr[10] = 4;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format16bitRGB565) {
        arr[10] = 9;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format24bitRGB888) {
        arr[10] = 10;
    }
    else if (pPortDefOut->format.image.eColorFormat == OMX_COLOR_Format32bitARGB8888) {
        arr[10] = 11;
    }
    else { /*Set DEFAULT (Original) color format*/
        arr[10] = 1;
    }
    /*arr[11] doesn't need to be filled*/
    
    if(pComponentPrivate->pSectionDecode->bSectionsInput){ /*Slide decoding enable*/
        arr[12] = 1;
    }
    else{
        arr[12] = 0;
    }

    if(pComponentPrivate->pSectionDecode->bSectionsOutput){ /*Slide decoding enable*/
        arr[13] = 1;
    }
    else{
        arr[13] = 0;
    }

    arr[14] = END_OF_CR_PHASE_ARGS;

    lcml_dsp->pCrPhArgs = arr;

    JPEGDEC_DPRINT("Image Width\t= %d\n", arr[8]);
    JPEGDEC_DPRINT("Image Height\t= %d\n", arr[7]);
    JPEGDEC_DPRINT("Progressive\t= %d\n", arr[9]);
    JPEGDEC_DPRINT("Color Format\t= %d\n", arr[10]);
    JPEGDEC_DPRINT("Section Decode(Input)\t= %d\n", arr[12]);
    JPEGDEC_DPRINT("Section Decode(Output)\t= %d\n", arr[13]);
    
EXIT:
    return eError;

}   /* End of Fill_LCMLInitParamsJpegDec */



/*-------------------------------------------------------------------*/
/**
  *  HandleInternalFlush() This function return request to USN to return all buffers
  * via USN 
  * *  
  *
  * @param pComponentPrivate
  * @param nParam1
  *
  * @retval OMX_NoError              Success, ready to roll
  *
  *
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleInternalFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 aParam[4];
    LCML_DSP_INTERFACE *pLCML = NULL;
    OMX_U8 nCount = 0;
#ifdef UNDER_CE
    OMX_U32 nTimeout = 0;
#endif
    OMX_CHECK_PARAM(pComponentPrivate);

    if ( nParam1 == 0x0 || nParam1 == -1 ) {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 0;
        aParam[2] = 0;
        pLCML = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;

        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLCML)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
        nTimeout = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeout++ > 200000) {
                JPEGDEC_DPRINT("Flush input Timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

            if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                    JPEGDEC_DPRINT("disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                    nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                } 
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                JPEGDEC_DPRINT("return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                             pComponentPrivate->pHandle->pApplicationPrivate,
                                             pBuffPrivate->pBufferHdr);
            }
        }

        pComponentPrivate->bFlushComplete = OMX_FALSE;
    }
    if ( nParam1 == 0x1 || nParam1 == -1 ) {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 1;
        aParam[2] = 0;
        pLCML = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLCML)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
        nTimeout = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeout++ > 200000) {
                JPEGDEC_DPRINT("Flush output Timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }
        
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

            if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
            int nRet;
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                  PERF_ModuleLLMM);
#endif

            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                JPEGDEC_DPRINT("discard %p from InDir\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->nOutPortOut ++;
                 nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
              } 
#if 0  /* since we don't have this queue anymore, there is nothing to flush.  Buffers are handled immediately */
            else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
                 JPEGDEC_DPRINT("disgard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                 nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
              } 
#endif            
              JPEGDEC_DPRINT("return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
              pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
              pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                             pComponentPrivate->pHandle->pApplicationPrivate,
                                             pBuffPrivate->pBufferHdr);
            }
        }
        
        pComponentPrivate->bFlushComplete = OMX_FALSE;
    }

EXIT:
    return eError;

}



/* ========================================================================== */
/**
 * @fn HandleCommandFlush - Implements the functionality to send flush command.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - paramerer specifying the port type (Index port)
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_U32 HandleCommandFlush(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                           OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 nCount = 0;
    OMX_COMPONENTTYPE* pHandle = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    OMX_U32 aParam[3];
    OMX_U8 nBuffersIn = 0;
    OMX_U8 nBuffersOut = 0;
#ifdef UNDER_CE
    OMX_U32 nTimeOut = 0;
#endif

    OMX_CHECK_PARAM(pComponentPrivate);
    pHandle = pComponentPrivate->pHandle;
    
    nBuffersIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual;
    nBuffersOut = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual;

    if ( nParam1 == JPEGDEC_INPUT_PORT || nParam1 == -1 )   {

        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 0;
        aParam[2] = 0;
        pLcmlHandle = (LCML_DSP_INTERFACE*)(pComponentPrivate->pLCML);
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        JPEGDEC_DPRINT("pLcmlHandle %p\n", pLcmlHandle);
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
        JPEGDEC_DPRINT("eError %x\n", eError); 
        if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
    nTimeOut = 0;
#endif
    while(pComponentPrivate->bFlushComplete == OMX_FALSE){
#ifdef UNDER_CE
        sched_yield();
         if (nTimeOut++ > 200000){
            perror("Flushing Input port timeout Error\n");
            break;
        }
#else
        JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
    }
    pComponentPrivate->bFlushComplete = OMX_FALSE;

        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                   JPEGDEC_DPRINT("disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                  } 
                  pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
          JPEGDEC_DPRINT("return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                  pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
              }
        }

        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                                OMX_EventCmdComplete,
                                                OMX_CommandFlush,
                                                JPEGDEC_INPUT_PORT, 
                                                NULL);
        }

    
    if ( nParam1 == JPEGDEC_OUTPUT_PORT|| nParam1 == -1 ){
        /* return all output buffers */
        aParam[0] = USN_STRMCMD_FLUSH;
        aParam[1] = 1;
        aParam[2] = 0;
        pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
        pComponentPrivate->bFlushComplete = OMX_FALSE;
        JPEGDEC_DPRINT("pLcmlHandle %p\n", pLcmlHandle);
        eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,EMMCodecControlStrmCtrl, (void*)aParam);
         JPEGDEC_DPRINT("eError %x\n", eError); 
       if (eError != OMX_ErrorNone) {
            goto EXIT;
        }
#ifdef UNDER_CE
        nTimeOut = 0;
#endif
        while (pComponentPrivate->bFlushComplete == OMX_FALSE) {
#ifdef UNDER_CE
            sched_yield();
            if (nTimeOut++ > 200000) {
                perror("Flushing Input port timeout Error\n");
                break;
            }
#else
            JPEGDEC_WAIT_FLUSH(pComponentPrivate);
#endif
        }

        pComponentPrivate->bFlushComplete = OMX_FALSE;

        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
            JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
            int nRet;
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                  PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                  PERF_ModuleLLMM);
#endif

            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                JPEGDEC_DPRINT("disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                pComponentPrivate->nOutPortOut ++;
                 nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
            }

#if 0  /* since we don't have this queue anymore, there is nothing to flush.  Buffers are handled immediately */
            else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
                 JPEGDEC_DPRINT("disgard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                 nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
            }
#endif            
            JPEGDEC_DPRINT("return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                         pComponentPrivate->pHandle->pApplicationPrivate,
                                         pBuffPrivate->pBufferHdr);
            }
        }

        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                       OMX_EventCmdComplete,
                                       OMX_CommandFlush,
                                       JPEGDEC_OUTPUT_PORT,
                                       NULL);
    }
EXIT:
    JPEGDEC_DPRINT ("Exiting HandleCommand nFlush Function\n");
    return eError;

}   /* End of HandleCommandFlush */



/* ========================================================================== */
/**
 * @fn HandleCommandJpegDec - andle State type commands. Depending on the
 *  State Command received it executes the corresponding code.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_U32 HandleCommandJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate,
                             OMX_U32 nParam1)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefOut = NULL;
    OMX_HANDLETYPE pLcmlHandle = NULL;
    LCML_DSP *lcml_dsp;
    OMX_U16 arr[100];
    LCML_CALLBACKTYPE cb;
    OMX_U8 nCount = 0;
    int    nBufToReturn;
#ifdef RESOURCE_MANAGER_ENABLED
    OMX_U16 nMHzRM = 0;
    OMX_U32 lImageResolution = 0;
#endif


    OMX_CHECK_PARAM(pComponentPrivate);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pPortDefIn = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef;
    pPortDefOut= pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef;


    switch ((OMX_STATETYPE)(nParam1))
    {
    case OMX_StateIdle:
        JPEGDEC_DPRINT("JPEG HandleCommand: Cmd Idle \n");
        JPEGDEC_DPRINT("CURRENT STATE IS %d\n",pComponentPrivate->nCurState);
        if (pComponentPrivate->nCurState == OMX_StateIdle) {
            eError = OMX_ErrorSameState;
            break;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateLoaded) ||
                    (pComponentPrivate->nCurState == OMX_StateWaitForResources)) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                           PERF_BoundaryStart | PERF_BoundarySetup);
#endif

            JPEGDEC_DPRINT("Transition state from loaded to idle\n");

#ifdef RESOURCE_MANAGER_ENABLED /* Resource Manager Proxy Calls */
            pComponentPrivate->rmproxyCallback.RMPROXY_Callback = (void *)ResourceManagerCallback;
            lImageResolution = pPortDefIn->format.image.nFrameWidth * pPortDefIn->format.image.nFrameHeight;
            OMX_GET_RM_VALUE(lImageResolution, nMHzRM);
            JPEGDEC_DPRINT("Value sent to RM = %d\n", nMHzRM);
            if (pComponentPrivate->nCurState != OMX_StateWaitForResources) {

                eError = RMProxy_NewSendCommand(pHandle, RMProxy_RequestResource, OMX_JPEG_Decoder_COMPONENT, nMHzRM, 3456, &(pComponentPrivate->rmproxyCallback));

                if (eError != OMX_ErrorNone) {
                    /* resource is not available, need set state to OMX_StateWaitForResources*/
                    JPEGDEC_DPRINT("Resource is not available\n");
                    eError = OMX_ErrorInsufficientResources;
                    break;
                }
            }
#endif

            if ((pPortDefIn->bEnabled == OMX_TRUE) &&
                (pPortDefOut->bEnabled == OMX_TRUE)) {

                while (1) {
                    if ((pPortDefIn->bPopulated) && (pPortDefOut->bPopulated)) {
                        break;
                    }
                    else {
                        JPEGDEC_WAIT_PORT_POPULATION(pComponentPrivate);

                    }
                }
                if(eError != OMX_ErrorNone){
                    perror("Port population time out\n");
                    goto EXIT;
                }
            }

            eError =  GetLCMLHandleJpegDec(pHandle);
            if (eError != OMX_ErrorNone) {
                perror("GetLCMLHandle failed...\n");
                goto EXIT;
            }

            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            lcml_dsp = (((LCML_DSP_INTERFACE*)pLcmlHandle)->dspCodec);

            JPEGDEC_DPRINT("Fill_LCMLInitParams in JPEG\n");
            Fill_LCMLInitParamsJpegDec(lcml_dsp,arr, pHandle);

            cb.LCML_Callback = (void *) LCML_CallbackJpegDec;

            if (pComponentPrivate->nIsLCMLActive == 1) {
                JPEGDEC_DPRINT("nIsLCMLActive is active\n");
            }
            /*calling initMMCodec to init codec with details filled earlier */
            eError = LCML_InitMMCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, NULL, &pLcmlHandle, NULL, &cb);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("InitMMCodec failed...\n");
                goto EXIT;
            }
            else {
                pComponentPrivate->nIsLCMLActive = 1;
            }
            JPEGDEC_DPRINT("LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle %p\n" , ((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle);
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlUsnEos, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("Enable EOS at LCML failed...\n");
                goto EXIT;
            }
            /* need check the resource with RM */

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySetup);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateIdle,  3456, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("Resources not available Loaded ->Idle\n");
                break;
            }
#endif
            pComponentPrivate->nCurState = OMX_StateIdle;

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
            break;
            JPEGDEC_DPRINT("JPEGDEC: State has been Set to Idle\n");
        }
        else if ((pComponentPrivate->nCurState == OMX_StateExecuting) ||
                 (pComponentPrivate->nCurState == OMX_StatePause)) {
/*            if (pComponentPrivate->bPreempted == 1){
                    eError = OMX_ErrorResourcesPreempted;
            }
*/            
            nCount = 0;
            pComponentPrivate->ExeToIdleFlag = 0;
            JPEGDEC_DPRINT("OMX_StateIdle->OMX_StateExecuting-THE CODEC IS STOPPING!!!\n");
            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, MMCodecControlStop, NULL);
#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

        JPEGDEC_DPRINT("before stop lock\n");
        pthread_mutex_lock(&pComponentPrivate->mJpegDecMutex);
        while ((pComponentPrivate->ExeToIdleFlag & JPEGD_DSPSTOP) == 0) {
            pthread_cond_wait(&pComponentPrivate->sStop_cond, &pComponentPrivate->mJpegDecMutex);
        }
        pthread_mutex_unlock(&pComponentPrivate->mJpegDecMutex);


        nBufToReturn = 0;
        if ((pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent != NULL)  &&
                (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pParamBufSupplier->eBufferSupplier == OMX_BufferSupplyInput)) {
                for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual ; nCount++) {
                   JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];
                   JPEGDEC_DPRINT("Jpeg Returning buffers to Display\n");

                   if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                       OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                       int nRet;

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif

                       if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                          JPEGDEC_DPRINT("disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                          nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                       } 

#if 0  /* since we don't have this queue anymore, there is nothing to discard.  Buffers are handled immediately */
                       else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
                          JPEGDEC_DPRINT("discard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                          nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                       } 
#endif                       
                       JPEGDEC_DPRINT("return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
                       pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

                       eError = OMX_EmptyThisBuffer(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent,
                                    (OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount]->pBufferHdr);
                    }
                 }
        }
        else { /* output port is not tunneled */
            for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[1]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                     JPEGDEC_DPRINT("discard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     pComponentPrivate->nOutPortOut ++;
                     nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                }
#if 0  /* since we don't have this queue anymore, there is nothing to discard.  Buffers are handled immediately */                
                else if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_OUT) {
                     JPEGDEC_DPRINT("discard %p from OutDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                }
#endif                
                JPEGDEC_DPRINT("return output buffer %p in idle (%d)\n", pBuffPrivate->pBufferHdr, pBuffPrivate->eBufferOwner);
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
               }
            }
        }

        JPEGDEC_DPRINT("all buffers are returned\n");

        nBufToReturn = 0;
        for (nCount = 0; nCount < pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->nBufferCountActual; nCount++) {
              JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pBufferPrivate[nCount];

              if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
                OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
                int nRet;
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), pBuffer),
                                      PREF(((OMX_BUFFERHEADERTYPE*) pComponentPrivate->pCompPort[0]->pBufferPrivate[nCount]->pBufferHdr), nFilledLen),
                                      PERF_ModuleLLMM);
#endif
        
                  nBufToReturn ++;
                  if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_COMPONENT_IN) {
                     JPEGDEC_DPRINT("disgard %p from InDir\n", pBuffPrivate->pBufferHdr);
                     nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &pBuffHead, sizeof(pBuffHead));
                  } 
                  pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
                  JPEGDEC_DPRINT("return input buffer %p in idle\n", pBuffPrivate->pBufferHdr);
                  pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffPrivate->pBufferHdr);
               }
        }

#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateIdle, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("Resources not available Executing ->Idle\n");
                pComponentPrivate->nCurState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->nCurState,
                                                       NULL);
                eError = OMX_ErrorNone;
                break;
            }
#endif
         pComponentPrivate->nCurState = OMX_StateIdle;
         JPEGDEC_DPRINT("current state is %d\n", pComponentPrivate->nCurState);
         pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   OMX_StateIdle,
                                                   NULL);
         pComponentPrivate->ExeToIdleFlag = 0;
         JPEGDEC_DPRINT("JPEG-DEC in idle\n");
        }
        else { /* This means, it is invalid state from application */
            JPEGDEC_DPRINT("Error: Invalid State Given by Application\n");
            eError = OMX_ErrorInvalidState;
        }
        break;

    case OMX_StateExecuting:
        JPEGDEC_DPRINT("HandleCommand: Cmd Executing \n");
        if (pComponentPrivate->nCurState == OMX_StateExecuting) {
            eError = OMX_ErrorSameState;
        }
        else if (pComponentPrivate->nCurState == OMX_StateIdle ||
                  pComponentPrivate->nCurState == OMX_StatePause) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif

            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlStart, NULL);

            JPEGDEC_DPRINT("eError is %x\n", eError);
#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateExecuting, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("Resources not available\n");
                pComponentPrivate->nCurState = OMX_StateWaitForResources;
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                       pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       pComponentPrivate->nCurState,
                                                       NULL);
                eError = OMX_ErrorNone;
                break;
            }
#endif

            pComponentPrivate->nCurState = OMX_StateExecuting;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
         JPEGDEC_DPRINT("JPEG-DEC in OMX_StateExecuting\n");
        }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;


    case OMX_StatePause:
        JPEGDEC_DPRINT("HandleCommand: Cmd Pause\n");
        if (pComponentPrivate->nCurState == OMX_StatePause) {
            eError = OMX_ErrorSameState;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateIdle) ||
                 (pComponentPrivate->nCurState == OMX_StateExecuting)) {
            pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
            eError = LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlPause, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("Error during EMMCodecControlPause.. error is %d.\n", eError);
                break;
            }

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

            pComponentPrivate->nCurState = OMX_StatePause;
        }
        else {
            JPEGDEC_DPRINT ("Error: Invalid State Given by Application\n");
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateInvalid:
        JPEGDEC_DPRINT("HandleCommand: Cmd OMX_StateInvalid::\n");
        if (pComponentPrivate->nCurState == OMX_StateInvalid) {
            eError = OMX_ErrorSameState;
            break;
        }
        if (pComponentPrivate->nCurState == OMX_StateExecuting 
                || pComponentPrivate->nCurState == OMX_StatePause){
            JPEGDEC_DPRINT("HandleInternalFlush\n\n");
            eError = HandleInternalFlush(pComponentPrivate, OMX_ALL); /*OMX_ALL = -1 OpenMax 1.1*/
            if(eError != OMX_ErrorNone){
                JPEGDEC_DPRINT("eError from HandleInternalFlush = %x\n", eError);
                eError = OMX_ErrorNone; /* Clean error, already sending the component to Invalid state*/
            }
        }
        JPEGDEC_DPRINT("OMX_StateInvalid\n\n");
        pComponentPrivate->nCurState = OMX_StateInvalid;

        if(pComponentPrivate->nToState == OMX_StateInvalid){ /*if the IL client call directly send to invalid state*/
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                   OMX_EventCmdComplete, 
                                   OMX_CommandStateSet, 
                                   pComponentPrivate->nCurState, 
                                   NULL);
        }
        else{ /*When the component go to invalid state by it self*/
            eError = OMX_ErrorInvalidState;
        }
        break;

    case OMX_StateLoaded:
            JPEGDEC_DPRINT("go to loaded state\n");
       if (pComponentPrivate->nCurState == OMX_StateLoaded) {
            eError = OMX_ErrorSameState;
        }
        else if ((pComponentPrivate->nCurState == OMX_StateIdle) ||
                 (pComponentPrivate->nCurState == OMX_StateWaitForResources)) {

#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#ifdef RESOURCE_MANAGER_ENABLED
            if (pComponentPrivate->nCurState == OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_CancelWaitForResource, OMX_JPEG_Decoder_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
                    JPEGDEC_DPRINT("CancelWaitForResource Failed\n");
                    break;
                }
            }
            
#endif

            /* Ports have to be unpopulated before transition completes */
            while (1) {
                if ((!pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT]->pPortDef->bPopulated) &&
                        (!pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pPortDef->bPopulated)) {
                    break;
                }
                else {
                    JPEGDEC_WAIT_PORT_UNPOPULATION(pComponentPrivate);
                }
            }
            if (eError != OMX_ErrorNone){ /*Verify if UnPopulation compleate*/
                goto EXIT;
            }

#ifdef RESOURCE_MANAGER_ENABLED
            if (pComponentPrivate->nCurState != OMX_StateWaitForResources) {
                eError= RMProxy_NewSendCommand(pHandle,  RMProxy_FreeResource, OMX_JPEG_Decoder_COMPONENT, 0, 3456, NULL);
                if (eError != OMX_ErrorNone) {
                    JPEGDEC_DPRINT("Cannot Free Resources\n");
                    break;
                }
            }
#endif

            if ((pComponentPrivate->pLCML != NULL) &&
                (pComponentPrivate->nIsLCMLActive == 1)) {
                pLcmlHandle =(LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;
                LCML_ControlCodec(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle, EMMCodecControlDestroy, NULL);
                pComponentPrivate->pLCML = NULL;
                pComponentPrivate->nIsLCMLActive = 0;
#ifdef UNDER_CE
                FreeLibrary(g_hLcmlDllHandle);
                g_hLcmlDllHandle = NULL;
#else
                dlclose(pComponentPrivate->pDllHandle);
                pComponentPrivate->pDllHandle = NULL;
#endif
            }


#ifdef __PERF_INSTRUMENTATION__
            PERF_Boundary(pComponentPrivate->pPERFcomp,
                          PERF_BoundaryComplete | PERF_BoundaryCleanup);
#endif

            /*Restart Buffer counting*/
            pComponentPrivate->nInPortIn = 0;
            pComponentPrivate->nOutPortOut = 0;

            pComponentPrivate->nCurState = OMX_StateLoaded;            
            
            if ((pComponentPrivate->nCurState == OMX_StateIdle) &&
                 (pComponentPrivate->bPreempted == 1 )){
                pComponentPrivate->bPreempted = 0;
                eError = OMX_ErrorResourcesLost;
            }
            else {
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventCmdComplete,
                                                       OMX_CommandStateSet,
                                                       OMX_StateLoaded,
                                                       NULL);
            }
       }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateWaitForResources:

        if (pComponentPrivate->nCurState == OMX_StateWaitForResources) {
            eError = OMX_ErrorSameState;
        }
        else if (pComponentPrivate->nCurState == OMX_StateLoaded) {
            
#ifdef RESOURCE_MANAGER_ENABLED
            eError= RMProxy_NewSendCommand(pHandle, RMProxy_StateSet, OMX_JPEG_Decoder_COMPONENT, OMX_StateWaitForResources, 3456, NULL);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT("RMProxy_NewSendCommand(OMX_StateWaitForResources) failed\n");
                break;
            }
#endif
            
            pComponentPrivate->nCurState = OMX_StateWaitForResources;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventCmdComplete,
                                                   OMX_CommandStateSet,
                                                   pComponentPrivate->nCurState,
                                                   NULL);
        }
        else {
            eError = OMX_ErrorIncorrectStateTransition;
        }
        break;

    case OMX_StateMax:
        JPEGDEC_DPRINT("HandleCommand: Cmd OMX_StateMax::\n");
        break;
    } /* End of Switch */

EXIT:
    JPEGDEC_DPRINT ("Exiting HandleCommand Function %x\n", eError);
    return eError;
} 
  /* End of HandleCommandJpegDec */

/* ========================================================================== */
/**
 * @fn HandleFreeOutputBufferFromAppJpegDec - Handle free output buffer from
 *  application reading in the nFree_outBuf_Q pipe, and queue to the LCML.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleFreeOutputBufferFromAppJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
    JPEGDEC_UAlgOutBufParamStruct *ptJPGDecUALGOutBufParam = NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int nRet;

    OMX_CHECK_PARAM(pComponentPrivate);
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;

    JPEGDEC_DPRINT("%s: read outport (in) buff header %p\n", __FUNCTION__, pBuffHead);

    nRet = read(pComponentPrivate->nFree_outBuf_Q[0], &pBuffHead, sizeof(pBuffHead));

    if (nRet == -1) {
        JPEGDEC_DPRINT ("Error while reading from the pipe\n");
        goto EXIT;
    }



    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    if ((pComponentPrivate->nCurState == OMX_StateIdle) || (pComponentPrivate->nToState == OMX_StateIdle)) {
        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
        JPEGDEC_DPRINT("Going to state %d, return buffer %p to client\n", pComponentPrivate->nToState, pBuffHead);
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
        pComponentPrivate->nOutPortOut ++;
        pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pBuffHead);
        }
        goto EXIT;
    }


    ptJPGDecUALGOutBufParam = (JPEGDEC_UAlgOutBufParamStruct *)pBuffPrivate->pUALGParams;
    ptJPGDecUALGOutBufParam->lOutBufCount = 0;
    ptJPGDecUALGOutBufParam->ulOutNumFrame = 1;
    ptJPGDecUALGOutBufParam->ulOutFrameAlign = 4;
    ptJPGDecUALGOutBufParam->ulOutFrameSize = pBuffHead->nAllocLen;

    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_DSP;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      pBuffHead->pBuffer,
                      pBuffHead->nFilledLen,
                      PERF_ModuleCommonLayer);
#endif

    eError = LCML_QueueBuffer(((LCML_DSP_INTERFACE*)pLcmlHandle)->pCodecinterfacehandle,
                              EMMCodecOuputBuffer,
                              pBuffHead->pBuffer,
                              pBuffHead->nAllocLen,
                              pBuffHead->nFilledLen,
                              (OMX_U8*)ptJPGDecUALGOutBufParam,
                              sizeof(JPEGDEC_UAlgOutBufParamStruct),
                              (OMX_U8*)pBuffHead);
    if (eError != OMX_ErrorNone) {
        goto EXIT;
    }
EXIT:
    return eError;
}   /* end of HandleFreeOutputBufferFromAppJpegDec */


/* ========================================================================== */
/**
 * @fn HandleDataBuf_FromAppJpegDec - Handle data to be encoded form
 *  application and queue to the LCML.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleDataBuf_FromAppJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBuffHead =  NULL;
    LCML_DSP_INTERFACE* pLcmlHandle = NULL;
    JPEGDEC_UAlgInBufParamStruct *ptJPGDecUALGInBufParam = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int nRet;


    OMX_CHECK_PARAM(pComponentPrivate);
    pLcmlHandle = (LCML_DSP_INTERFACE*)pComponentPrivate->pLCML;

    nRet = read(pComponentPrivate->nFilled_inpBuf_Q[0], &(pBuffHead), sizeof(pBuffHead));
    if (nRet == -1) {
        JPEGDEC_DPRINT("Error while reading from the pipe\n");
    }

    JPEGDEC_DPRINT("HandleDataBuf_FromAppJpegDec: read inport (in) buff header %p\n", pBuffHead);

    pBuffPrivate = pBuffHead->pInputPortPrivate;
    if ((pComponentPrivate->nCurState == OMX_StateIdle) || (pComponentPrivate->nToState == OMX_StateIdle)) {
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;
        JPEGDEC_DPRINT("Going to state %d, return buffer %p to client\n", pComponentPrivate->nToState, pBuffHead);

        pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                    pComponentPrivate->pHandle->pApplicationPrivate,
                    pBuffHead);
        goto EXIT;
    }

    ptJPGDecUALGInBufParam = (JPEGDEC_UAlgInBufParamStruct *)pBuffPrivate->pUALGParams;
    ptJPGDecUALGInBufParam->ulAlphaRGB = 0;
    ptJPGDecUALGInBufParam->lInBufCount = 0;
    ptJPGDecUALGInBufParam->ulInNumFrame = 1;
    ptJPGDecUALGInBufParam->ulInFrameAlign = 4;
    ptJPGDecUALGInBufParam->ulInFrameSize = pBuffHead->nFilledLen;
    ptJPGDecUALGInBufParam->ulInDisplayWidth = (int)pComponentPrivate->nInputFrameWidth;
    ptJPGDecUALGInBufParam->ulInResizeOption = (int)pComponentPrivate->pScalePrivate->xWidth;
    /*Slide decode*/
    ptJPGDecUALGInBufParam->ulNumMCURow = (int)pComponentPrivate->pSectionDecode->nMCURow;
    ptJPGDecUALGInBufParam->ulnumAU = (int)pComponentPrivate->pSectionDecode->nAU;
    /*Section decode*/
    ptJPGDecUALGInBufParam->ulXOrg = (int)pComponentPrivate->pSubRegionDecode->nXOrg;
    ptJPGDecUALGInBufParam->ulYOrg = (int)pComponentPrivate->pSubRegionDecode->nYOrg;
    ptJPGDecUALGInBufParam->ulXLength = (int)pComponentPrivate->pSubRegionDecode->nXLength;
    ptJPGDecUALGInBufParam->ulYLength = (int)pComponentPrivate->pSubRegionDecode->nYLength;
    

    if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_FormatCbYCrY) {
        ptJPGDecUALGInBufParam->forceChromaFormat= 4;
        ptJPGDecUALGInBufParam->RGB_Format = 9; /*RGB_Format should be set even if it's not use*/
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format16bitRGB565) {
        ptJPGDecUALGInBufParam->forceChromaFormat =  9;
        ptJPGDecUALGInBufParam->RGB_Format = 9;
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format24bitRGB888) {
        ptJPGDecUALGInBufParam->forceChromaFormat = 10;
        ptJPGDecUALGInBufParam->RGB_Format = 10;
    }
    else if (pComponentPrivate->nOutputColorFormat == OMX_COLOR_Format32bitARGB8888) {
        ptJPGDecUALGInBufParam->forceChromaFormat = 11;
        ptJPGDecUALGInBufParam->RGB_Format = 11;
    }
    else { /*Set DEFAULT (Original) color format*/
        ptJPGDecUALGInBufParam->forceChromaFormat = 1;
        ptJPGDecUALGInBufParam->RGB_Format = 9; /*RGB_Format should be set even if it's not use*/
    }
    JPEGDEC_DPRINT("ptJPGDecUALGInBufParam->forceChromaFormat = %d\n", ptJPGDecUALGInBufParam->forceChromaFormat );
    pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_DSP;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      pBuffHead->pBuffer,
                      pBuffHead->nFilledLen,
                      PERF_ModuleCommonLayer);
#endif

    JPEGDEC_DPRINT("forceChromaFormat\t= %d\n", ptJPGDecUALGInBufParam->forceChromaFormat);
    JPEGDEC_DPRINT("RGB_Format\t= %d\n", ptJPGDecUALGInBufParam->RGB_Format);
    JPEGDEC_DPRINT("ulInFrameSize\t= %d\n", ptJPGDecUALGInBufParam->ulInFrameSize);
    JPEGDEC_DPRINT("ulInDisplayWidth\t= %d\n", ptJPGDecUALGInBufParam->ulInDisplayWidth);
    JPEGDEC_DPRINT("ulInResizeOption\t= %d\n", ptJPGDecUALGInBufParam->ulInResizeOption);
    JPEGDEC_DPRINT("ulNumMCURow\t= %d\n", ptJPGDecUALGInBufParam->ulNumMCURow);
    JPEGDEC_DPRINT("ulnumAU\t= %d\n", ptJPGDecUALGInBufParam->ulnumAU);
    JPEGDEC_DPRINT("ulXOrg\t= %d    ", ptJPGDecUALGInBufParam->ulXOrg);
    JPEGDEC_DPRINT("ulYOrg\t= %d\n", ptJPGDecUALGInBufParam->ulYOrg);
    JPEGDEC_DPRINT("ulXLength\t= %d    ", ptJPGDecUALGInBufParam->ulXLength);
    JPEGDEC_DPRINT("ulXLenght\t= %d\n", ptJPGDecUALGInBufParam->ulYLength);
    JPEGDEC_DPRINT("pBuffHead->nFlags\t= %d\n", pBuffHead->nFlags);
    JPEGDEC_DPRINT("Queue INPUT bufheader %p\n", pBuffHead);
    eError = LCML_QueueBuffer(pLcmlHandle->pCodecinterfacehandle,
                              EMMCodecInputBuffer,
                              pBuffHead->pBuffer,
                              pBuffHead->nAllocLen,
                              pBuffHead->nFilledLen,
                              (OMX_U8 *) ptJPGDecUALGInBufParam,
                              sizeof(JPEGDEC_UAlgInBufParamStruct),
                              (OMX_U8 *)pBuffHead);

    if (eError != OMX_ErrorNone) {
        goto EXIT;
    }

EXIT:
    return eError;
}   /* End of HandleDataBuf_FromAppJpegDec */


/* ========================================================================== */
/**
 * @fn HandleDataBuf_FromDspJpegDec - Handle encoded data form DSP and
 *  render to application or another component.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleDataBuf_FromDspJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;

    JPEGDEC_DPRINT ("Buffer Came From DSP (output port)\n");
    OMX_CHECK_PARAM(pComponentPrivate);

    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    if (pBuffHead->pMarkData && pBuffHead->hMarkTargetComponent == pComponentPrivate->pHandle) {
        JPEGDEC_DPRINT("send OMX_MarkEvent\n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                        OMX_EventMark,
                                        JPEGDEC_OUTPUT_PORT,
                                        0,
                                        pBuffHead->pMarkData);
    }

    /*TUNNEL HERE*/
    if (pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent != NULL) {
        JPEGDEC_DPRINT ("Jpeg Sending Output buffer to TUNNEL component\n");

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[0]->pBufferHdr->pBuffer,
                          pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->pBufferPrivate[0]->pBufferHdr->nFilledLen,
                          PERF_ModuleLLMM);
#endif

        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_TUNNEL_COMPONENT;
        eError = OMX_EmptyThisBuffer(pComponentPrivate->pCompPort[JPEGDEC_OUTPUT_PORT]->hTunnelComponent, pBuffHead);
    }
    else {

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          pBuffHead->pBuffer,
                          pBuffHead->nFilledLen,
                          PERF_ModuleHLMM);
#endif


        if (pBuffHead->nFlags & OMX_BUFFERFLAG_EOS) {
            JPEGDEC_DPRINT("%s::%d:Received OMX_BUFFERFLAG_EOS, nFalgs= %x\n", __FUNCTION__, __LINE__, pBuffHead->nFlags);
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventBufferFlag,
                                                JPEGDEC_OUTPUT_PORT,
                                                pBuffHead->nFlags,
                                                NULL);
        }

        JPEGDEC_DPRINT("HandleDataBuf_FromDspJpegDec: buf %p pBuffPrivate->eBufferOwner %d\n", pBuffHead, pBuffPrivate->eBufferOwner);
        if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

            pComponentPrivate->cbInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                                 pBuffHead);
        }
    }


EXIT:
    JPEGDEC_DPRINT("Exit\n");
    return eError;
}   /* End of HandleDataBuf_FromDspJpegDec */



/* ========================================================================== */
/**
 * @fn HandleFreeDataBufJpegDec - Handle emptied input data from DSP and
 *  return to application or another component.
 * @param pComponentPrivate - components private structure
 * @param nParam1 - state to change.
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
 */
/* ========================================================================== */
OMX_ERRORTYPE HandleFreeDataBufJpegDec(JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE* pBuffHead )
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;

    JPEGDEC_DPRINT ("JPEG Entering HandleFreeBuf Function\n");
    OMX_CHECK_PARAM(pComponentPrivate);

    pBuffPrivate = pBuffHead->pInputPortPrivate;

    if (pBuffPrivate->eBufferOwner != JPEGDEC_BUFFER_CLIENT) {
        pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_CLIENT;

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      PREF(pBuffHead,pBuffer),
                      0,
                      PERF_ModuleHLMM);
#endif

        JPEGDEC_DPRINT("emptydone buf %p\n", pBuffHead);

        pComponentPrivate->cbInfo.EmptyBufferDone(pComponentPrivate->pHandle,
                                              pComponentPrivate->pHandle->pApplicationPrivate,
                                              pBuffHead);
    }

EXIT:
    JPEGDEC_DPRINT("JPEGexiting\n");
    return eError;
}   /* End of HandleFreeDataBufJpegDec */



/* ========================================================================== */
/**
 *  LCML_CallbackJpegDec() - handle callbacks from LCML
 * @param pComponentPrivate    handle for this instance of the component
 * @param argsCb = argument list
 * @return: OMX_ERRORTYPE
 *          OMX_ErrorNone on success
 *          !OMX_ErrorNone on failure
  **/
/* ========================================================================== */
OMX_ERRORTYPE LCML_CallbackJpegDec (TUsnCodecEvent event,
                                    void * argsCb [10])
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_BUFFERHEADERTYPE* pBuffHead = NULL;
    JPEGDEC_UAlgInBufParamStruct * ptJPGDecUALGInBufParam = NULL;
    JPEGDEC_PORT_TYPE *pPortType = NULL;
    OMX_U8* pBuffer = NULL;
    JPEGDEC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    int i = 0;

    if ( ((LCML_DSP_INTERFACE*)argsCb[6] ) != NULL ) {
        pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE*)((LCML_DSP_INTERFACE*)argsCb[6])->pComponentPrivate;
    }
    else {
        JPEGDEC_DPRINT("wrong in LCML callback, exit\n");
        goto EXIT;
    }

    if (event == EMMCodecBufferProcessed) {
        if ((int)argsCb [0] == EMMCodecOuputBuffer) {
            pBuffHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            pBuffer = (OMX_U8*)argsCb[1];
            pBuffPrivate = pBuffHead->pOutputPortPrivate;
            pBuffHead->nFilledLen = (int)argsCb[8];
            JPEGDEC_DPRINT("nFilled Len from DSP = %d\n",(int)argsCb[8]);

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               pBuffer,
                               (OMX_U32) argsCb[2],
                               PERF_ModuleCommonLayer);
#endif

        if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
            pComponentPrivate->nOutPortOut ++;
        }

        JPEGDEC_DPRINT("Filled Data from DSP \n");
        JPEGDEC_DPRINT("buffer summary (LCML for output buffer %p) %d %d\n", pBuffHead, 
                    pComponentPrivate->nInPortIn,
                    pComponentPrivate->nOutPortOut);

        pPortType = pComponentPrivate->pCompPort[JPEGDEC_INPUT_PORT];
        if((pBuffHead->nFlags == OMX_FALSE) && (pComponentPrivate->pSectionDecode->nMCURow == OMX_FALSE)){
            for (i = 0; i < pPortType->pPortDef->nBufferCountActual; i ++) {
                if (pPortType->sBufferFlagTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
                    JPEGDEC_DPRINT("JPEGdec:: %d: output buffer %d has flag %x\n", __LINE__,
                               pPortType->sBufferFlagTrack[i].buffer_id, 
                               pPortType->sBufferFlagTrack[i].flag);
                    pBuffHead->nFlags = pPortType->sBufferFlagTrack[i].flag;
                    pPortType->sBufferFlagTrack[i].flag = 0;
                    pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
                    break;
                }
            }
        }else{
            for (i = 0; i < pPortType->pPortDef->nBufferCountActual; i ++) {
                if (pPortType->sBufferFlagTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
                    JPEGDEC_DPRINT("JPEGdec:: %d: OUTPUT buffer %d has flag %x\n", __LINE__,
                               pPortType->sBufferFlagTrack[i].buffer_id, 
                               pPortType->sBufferFlagTrack[i].flag);
                    if(pPortType->sBufferFlagTrack[i].flag & OMX_BUFFERFLAG_EOS){
                        pPortType->sBufferFlagTrack[i].flag = pPortType->sBufferFlagTrack[i].flag & (!(OMX_BUFFERFLAG_EOS));
                        pBuffHead->nFlags |= pPortType->sBufferFlagTrack[i].flag;
                    }
                    pPortType->sBufferFlagTrack[i].flag = 0;
                    pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
                    break;
                }
            }
        }
        for (i = 0; i < pPortType->pPortDef->nBufferCountActual; i ++) {
            if (pPortType->sBufferMarkTrack[i].buffer_id == pComponentPrivate->nOutPortOut) {
                JPEGDEC_DPRINT("buffer ID %d has mark (output port)\n", pPortType->sBufferMarkTrack[i].buffer_id);
                pBuffHead->pMarkData = pPortType->sBufferMarkTrack[i].pMarkData;
                pBuffHead->hMarkTargetComponent = pPortType->sBufferMarkTrack[i].hMarkTargetComponent;
                pPortType->sBufferMarkTrack[i].buffer_id = 0xFFFFFFFF;
                break;
            }
        }
        if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
            pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_OUT;
            eError = HandleDataBuf_FromDspJpegDec(pComponentPrivate, pBuffHead);
            if (eError != OMX_ErrorNone) {
                JPEGDEC_DPRINT ("Error while reading dsp out q\n");
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventError,
                                                       OMX_ErrorUndefined,
                                                       OMX_TI_ErrorSevere,
                                                       "Error from Component Thread while processing dsp Responses");
            }
        }
        }
        if ((int) argsCb [0] == EMMCodecInputBuffer) {
            pBuffHead = (OMX_BUFFERHEADERTYPE*)argsCb[7];
            ptJPGDecUALGInBufParam = (JPEGDEC_UAlgInBufParamStruct *)argsCb[3];
            pBuffer = (OMX_U8*)argsCb[1];
            pBuffPrivate = pBuffHead->pInputPortPrivate;

#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                               pBuffer,
                               (OMX_U32) argsCb[8],
                               PERF_ModuleCommonLayer);
#endif
            if (pBuffPrivate->eBufferOwner == JPEGDEC_BUFFER_DSP) {
                pBuffPrivate->eBufferOwner = JPEGDEC_BUFFER_COMPONENT_OUT;
                eError = HandleFreeDataBufJpegDec(pComponentPrivate, pBuffHead);
                if (eError != OMX_ErrorNone) {
                    JPEGDEC_DPRINT ("Error while processing free input Buffers\n");
                    pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorUndefined,
                                                           OMX_TI_ErrorSevere,
                                                           "Error while processing free input buffers");
                }
            }
        }
    }

    if (event == EMMCodecProcessingStoped) {
        JPEGDEC_DPRINT("ENTERING TO EMMCodecProcessingStoped \n\n");
        if (pComponentPrivate->nToState == OMX_StateIdle) {
            pComponentPrivate->ExeToIdleFlag |= JPEGD_DSPSTOP;
        }

        pthread_mutex_lock(&pComponentPrivate->mJpegDecMutex);
        pthread_cond_signal(&pComponentPrivate->sStop_cond);
        pthread_mutex_unlock(&pComponentPrivate->mJpegDecMutex);

        goto EXIT;
    }
    if (event == EMMCodecDspError) {
        JPEGDEC_DPRINT("LCML_Callback : DSP [0]->%x, [4]->%x, [5]->%x\n", (int)argsCb[0] ,(int)argsCb[4], (int)argsCb[5]);
        JPEGDEC_DPRINT("Play compleated if: 0x500 = %x\n", (int)argsCb[5]);
        if(!((int)argsCb[5] == 0x500)){
            eError = OMX_ErrorHardware;
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorHardware,
                                                   OMX_TI_ErrorCritical,
                                                   NULL);
        }
        goto EXIT;
    }

    if (event == EMMCodecInternalError) {
        eError = OMX_ErrorHardware;
        JPEGDEC_DPRINT("JPEG-D: EMMCodecInternalError\n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventError,
                                               OMX_ErrorHardware,
                                               OMX_TI_ErrorCritical,
                                               NULL);
        goto EXIT;
    }
    if (event == EMMCodecProcessingPaused) {
        pComponentPrivate->nCurState = OMX_StatePause;
        /* Send StateChangeNotification to application */
        JPEGDEC_DPRINT("ENTERING TO EMMCodecProcessingPaused \n");
        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                               pComponentPrivate->pHandle->pApplicationPrivate,
                                               OMX_EventCmdComplete,
                                               OMX_CommandStateSet,
                                               pComponentPrivate->nCurState,
                                               NULL);
    }
    if (event == EMMCodecStrmCtrlAck) {
        JPEGDEC_DPRINT("event = EMMCodecStrmCtrlAck\n");
        if ((int)argsCb [0] == USN_ERR_NONE) {
            JPEGDEC_DPRINT("Callback: no error\n");
            pComponentPrivate->bFlushComplete = OMX_TRUE;
            pthread_mutex_lock(&(pComponentPrivate->mJpegDecFlushMutex));
            pthread_cond_signal(&(pComponentPrivate->sFlush_cond));
            pthread_mutex_unlock(&(pComponentPrivate->mJpegDecFlushMutex));
       }
    }


    JPEGDEC_DPRINT("Exiting the LCML_Callback function\n");
EXIT:
    return eError;
}   /* End of LCML_CallbackJpegDec */



#ifdef RESOURCE_MANAGER_ENABLED
/* ========================================================================== */
/**
 *  ResourceManagerCallback() - handle callbacks from Resource Manager
 * @param cbData    Resource Manager Command Data Structure
 * @return: void
  **/
/* ========================================================================== */

void ResourceManagerCallback(RMPROXY_COMMANDDATATYPE cbData)
{
    OMX_COMMANDTYPE Cmd = OMX_CommandStateSet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)cbData.hComponent;
    JPEGDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE RM_Error = *(cbData.RM_Error);
    
    JPEGDEC_DPRINT("%s: %d: RM_Error = %x\n", __FUNCTION__, __LINE__, RM_Error);

    pComponentPrivate = (JPEGDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    if (RM_Error == OMX_RmProxyCallback_ResourcesPreempted) {

        pComponentPrivate->bPreempted = 1;
        
        if (pComponentPrivate->nCurState == OMX_StateExecuting || 
            pComponentPrivate->nCurState == OMX_StatePause) {

            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorResourcesPreempted,
                                                   OMX_TI_ErrorSevere,
                                                   NULL);
            
            pComponentPrivate->nToState = OMX_StateIdle;
            JPEGDEC_DPRINT("Component Preempted. Going to IDLE State.\n");
        }
        else if (pComponentPrivate->nCurState == OMX_StateIdle){
            pComponentPrivate->nToState = OMX_StateLoaded;
            JPEGDEC_DPRINT("Component Preempted. Going to LOADED State.\n");            
        }
        
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->nToState, PERF_ModuleComponent);
#endif
        
        write (pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
        write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->nToState) ,sizeof(OMX_U32));
        
    }
    else if (RM_Error == OMX_RmProxyCallback_ResourcesAcquired ){

        if (pComponentPrivate->nCurState == OMX_StateWaitForResources) /* Wait for Resource Response */
        {
            pComponentPrivate->cbInfo.EventHandler (
    	                        pHandle, pHandle->pApplicationPrivate,
    	                        OMX_EventResourcesAcquired, 0,0,
    	                        NULL);
            
            pComponentPrivate->nToState = OMX_StateIdle;
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingCommand(pComponentPrivate->pPERF, Cmd, pComponentPrivate->nToState, PERF_ModuleComponent);
#endif
        
            write (pComponentPrivate->nCmdPipe[1], &Cmd, sizeof(Cmd));
            write (pComponentPrivate->nCmdDataPipe[1], &(pComponentPrivate->nToState) ,sizeof(OMX_U32));
            JPEGDEC_DPRINT("OMX_RmProxyCallback_ResourcesAcquired.\n");
        }            
        
    }

}
#endif




/*-------------------------------------------------------------------*/
/**
  * IsTIOMXComponent()
  *
  * Check if the component is TI component.
  *
  * @param hTunneledComp Component Tunnel Pipe
  *  
  * @retval OMX_TRUE   Input is a TI component.
  *         OMX_FALSE  Input is a not a TI component. 
  *
  **/
/*-------------------------------------------------------------------*/

OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_STRING cTunnelComponentName = NULL;
    OMX_VERSIONTYPE sTunnelComponentVersion;
    OMX_VERSIONTYPE sSpecVersion;
    OMX_UUIDTYPE sComponentUUID;
    OMX_STRING cSubstring = NULL;
    OMX_BOOL bResult = OMX_TRUE;

    MALLOC(cTunnelComponentName, char, COMP_MAX_NAMESIZE);            
    eError = OMX_GetComponentVersion (hComp, cTunnelComponentName, &sTunnelComponentVersion, &sSpecVersion, &sComponentUUID);
    /* Check if tunneled component is a TI component */

    cSubstring = strstr(cTunnelComponentName, "OMX.TI.");
    if(cSubstring == NULL) {
        bResult = OMX_FALSE;
    }

EXIT:
    FREE(cTunnelComponentName);
    return bResult;
} /* End of IsTIOMXComponent */

