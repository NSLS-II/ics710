/* $Name: H11515E $ */

/*!
 * \file   ics710api.h
 * \brief  ICS710 API Header
 *
 * \par    Module:
 *         libics710.a
 *
 * \par    Copyright:
 *         Copyright 2000 GE Intelligent Platforms (Ottawa) Ltd.  All rights reserved.
 *
 * \author Sergey Rozin
 *
 * \if 0
 *         Version:
 *         $Id: ics710api.h,v 1.13 2010-04-08 14:03:58 rkadaman Exp $
 *
 *         Revision Log:
 *         $Log: ics710api.h,v $
 *         Revision 1.13  2010-04-08 14:03:58  rkadaman
 *         Update Copyright.
 *
 *         Revision 1.12  2007-01-11 19:01:44  srozin
 *         Adde bit swapping
 *
 *         Revision 1.11  2006/01/27 20:48:21  srozin
 *         Added packing
 *
 *         Revision 1.10  2005/06/09 12:38:14  srozin
 *         Compiled
 *
 *         Revision 1.9  2005/06/08 17:58:12  srozin
 *         Added x86-64 support
 *
 *         Revision 1.8  2004/06/29 14:53:47  kdedman
 *         Changed parameter desc
 *
 *         Revision 1.7  2004/06/28 20:39:03  srozin
 *         Added examples template
 *
 *         Revision 1.6  2004/06/04 13:42:40  srozin
 *         Added External Trigger Mode
 *
 *         Revision 1.5  2004/05/31 12:48:29  kdedman
 *         Changes to comments
 *
 *         Revision 1.4  2004/05/28 19:28:00  kdedman
 *         Kim changed comments for ICS710_CONTROL Struct in ics710api.h
 *
 *         Revision 1.3  2004/05/20 18:16:56  srozin
 *         Added Reset Wait declaration
 *
 *         Revision 1.2  2004/05/20 17:56:20  srozin
 *         Added Reset Wait
 *
 *         Revision 1.1.1.1  2004/03/18 20:02:48  srozin
 *         ICS710 Linux SDK
 *
 *
 * \endif
 */


#ifndef _ICS710API_H_
#define _ICS710API_H_


#ifndef DOC_THIS
#ifndef __KERNEL__
#include <endian.h>
#endif /* __KERNEL__ */
#endif /* DOC_THIS */


#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)) || (!defined(__BYTE_ORDER) && defined(__BIG_ENDIAN))
#define ICS710_BE
#endif

#ifndef HANDLE
/*! \def     HANDLE
 *  \brief   Driver handle type
 */
#define HANDLE int
#endif


/*! \def     ICS710_OK
 *  \brief   Status - Success
 */
#define ICS710_OK               0

/*! \def     ICS710_ERROR
 *  \brief   Status - Error
 */
#define ICS710_ERROR            -1


/*! \def     ICS710_DISABLE
 *  \brief   Disable flag
 */
#define ICS710_DISABLE          0

/*! \def     ICS710_ENABLE
 *  \brief   Enable flag
 */
#define ICS710_ENABLE           1


/*! \def     ICS710_INACTIVE
 *  \brief   Inactive flag
 */
#define ICS710_INACTIVE         ICS710_DISABLE

/*! \def     ICS710_ACTIVE
 *  \brief   Active flag
 */
#define ICS710_ACTIVE           ICS710_ENABLE


/*! \def     ICS710_CONTINUOUS
 *  \brief   Define continuous mode
 */
#define ICS710_CONTINUOUS       0

/*! \def     ICS710_CAPTURE_NOPRETRG
 *  \brief   Define capture without pre-trigger storage
 */
#define ICS710_CAPTURE_NOPRETRG   1

/*! \def     ICS710_CAPTURE_WITHPRETRG
 *  \brief   Define capture with pre-trigger storage
 */
#define  ICS710_CAPTURE_WITHPRETRG   2


/*! \def     ICS710_SAMP_NORMAL
 *  \brief   Define Normal sampling speed
 */
#define ICS710_SAMP_NORMAL       0

/*! \def     ICS710_SAMP_DOUBLE
 *  \brief   Define Double sampling speed
 */
#define ICS710_SAMP_DOUBLE   1

/*! \def     ICS710_SAMP_QUAD
 *  \brief   Define Quad sampling speed
 */
#define  ICS710_SAMP_QUAD   2

/*! \def     ICS710_ZCAL_INTERNAL
 *  \brief   Define internal source zero calibration
 */
#define ICS710_ZCAL_INTERNAL    1

/*! \def     ICS710_ZCAL_EXTERNAL
 *  \brief   Define external source zero calibration
 */
#define ICS710_ZCAL_EXTERNAL    0


/*! \def     ICS710_UNPACKED_DATA
 *  \brief   Define unpacked adc data
 */
#define ICS710_UNPACKED_DATA    0

/*! \def     ICS710_PACKED_DATA
 *  \brief   Define packed adc data
 */
#define ICS710_PACKED_DATA    1


/*! \def     ICS710_TRIG_INTERNAL
 *  \brief   Define internal triggering
 */
#define ICS710_TRIG_INTERNAL    0

/*! \def     ICS710_TRIG_EXTERNAL
 *  \brief   Define external triggering
 */
#define ICS710_TRIG_EXTERNAL    1


/*! \def     ICS710_CLOCK_INTERNAL
 *  \brief   Define internal clock
 */
#define ICS710_CLOCK_INTERNAL   0

/*! \def     ICS710_CLOCK_EXTERNAL
 *  \brief   Define external clock
 */
#define ICS710_CLOCK_EXTERNAL   1


/*! \def     ICS710_EXTRIG_HIGH
 *  \brief   Define external trigger mode to acquire ADC data when level high
 */
#define ICS710_EXTRIG_HIGH      0

/*! \def     ICS710_EXTRIG_RISING
 *  \brief   Define external trigger mode to acquire ADC data on raising edge
 */
#define ICS710_EXTRIG_RISING    1

/*! \def     ICS710_EXTRIG_LOW
 *  \brief   Define external trigger mode to acquire ADC data when level low
 */
#define ICS710_EXTRIG_LOW       2

/*! \def     ICS710_EXTRIG_FALLING
 *  \brief   Define external trigger mode to acquire ADC data on falling edge
 */
#define ICS710_EXTRIG_FALLING   3



#pragma pack(push,1)

/*! \struct  ICS710_BLOCK ics710api.h "inc/ics710api.h"
 *  \brief   User data block description structure
 *  \remarks This structure is used to read a block of data from the board. \n
 *           The User must specify an \a offset of data (local memory), a \a count of \n
 *           long words to transfer, an \a increment setting for the copying direction\n
 *           (increment/don't increment) and a \a user buffer pointer to allocated memory. 
 */
typedef struct {
  unsigned long long       offset; /*!< offset of data */
  unsigned long long       count;  /*!< number of long words to transfer */
  unsigned int            incr;   /*!< direction 1 - increment, 0 - don't increment */
  unsigned long long      *buffer; /*!< user buffer */
}ICS710_BLOCK;


/*! \struct  ICS710_STATUS ics710api.h "inc/ics710api.h"
 *  \brief   Status Register content
 *  \remarks The Status Register is used to indicate the events that have occurred. \n
 *           The user can poll the register.
 */
typedef struct {
#ifdef ICS710_BE
  unsigned long long      filler1               : 55;
  unsigned long long      daughter_card_present : 1;  /*!< Daughter Card is present, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      clk_ready             : 1;  /*!< Clock Frequency register is accessible, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      cal                   : 1;  /*!< ADC is in offset calibration cycle, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      board_triggered       : 1;  /*!< Board is triggered, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      irq                   : 1;  /*!< ADC Interrupt Pending, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      diag_fifo_empty       : 1;  /*!< Diagnostic Fifo Empty, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      fpdp_buf_oflow        : 1;  /*!< FPDP Buffer Overflow Interrupt, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      pci_buf_oflow         : 1;  /*!< PCI Buffer Overflow Interrupt, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      adc_intrpt_rqst       : 1;  /*!< ADC Interrupt Request, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
#else
  unsigned long long      adc_intrpt_rqst       : 1;  /*!< ADC Interrupt Request, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      pci_buf_oflow         : 1;  /*!< PCI Buffer Overflow Interrupt, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      fpdp_buf_oflow        : 1;  /*!< FPDP Buffer Overflow Interrupt, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      diag_fifo_empty       : 1;  /*!< Diagnostic Fifo Empty, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      irq                   : 1;  /*!< ADC Interrupt Pending, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      board_triggered       : 1;  /*!< Board is triggered, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      cal                   : 1;  /*!< ADC is in offset calibration cycle, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      clk_ready             : 1;  /*!< Clock Frequency register is accessible, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      daughter_card_present : 1;  /*!< Daughter Card is present, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE*/
  unsigned long long      filler1               : 55;
#endif
} ICS710_STATUS,*pICS710_STATUS;


/*! \struct  ICS710_CONTROL ics710api.h "inc/ics710api.h"
 *  \brief   The Control Register content
 *  \remarks The Control Register is used to enable the ADCs and DACs \n
 *           and to control their modes of operation.
 */
typedef struct {
#ifdef ICS710_BE
  unsigned long long      filler1               : 41; /*!< Filler */
  unsigned long long      extrig_mode           : 2;  /*!< Determines external triggering mode, e.g. \ref ICS710_EXTRIG_HIGH, \ref ICS710_EXTRIG_LOW, \ref ICS710_EXTRIG_RAISING or \ref ICS710_EXTRIG_FALLING */
  unsigned long long      extrig_term           : 1;  /*!< Enable external trigger termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      extclk_term           : 1;  /*!< Enable external clock termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_termin           : 1;  /*!< Enable FPDP termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_termin            : 1;  /*!< Enable ADC termination on local bus, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_master           : 1;  /*!< Select FPDP master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_master            : 1;  /*!< Select ADC master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      system_master         : 1;  /*!< Select system master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      acq_mode              : 2;  /*!< Determines acquisition mode, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      packed_data           : 1;  /*!< Enable/disable data packing in PCI&FPDP, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      filler2               : 1;  /*!< RESERVED */ 
  unsigned long long      zero_cal              : 1;  /*!< Zero calibration method select, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_hpfilter_enable   : 1;  /*!< Enable/disable ADC converter high pass filter, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      oversamp_ratio        : 2;  /*!< Set oversampling ratio of ADC converter, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      int_trigger           : 1;  /*!< Trigger the board, e.g. (CR<00> must be selected) \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      enable                : 1;  /*!< Enable sampling, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_enable           : 1;  /*!< Data output to FPDP, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      diag_mode_enable      : 1;  /*!< Diagnostic mode enable, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_clock_select      : 1;  /*!< Internal/external ADC sampling clock, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      trigger_select        : 1;  /*!< Internal/external trigger select, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
#else
  unsigned long long      trigger_select        : 1;  /*!< Internal/external trigger select, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_clock_select      : 1;  /*!< Internal/external ADC sampling clock, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      diag_mode_enable      : 1;  /*!< Diagnostic mode enable, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_enable           : 1;  /*!< Data output to FPDP, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      enable                : 1;  /*!< Enable sampling, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      int_trigger           : 1;  /*!< Trigger the board, e.g. (CR<00> must be selected) \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      oversamp_ratio        : 2;  /*!< Set oversampling ratio of ADC converter, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_hpfilter_enable   : 1;  /*!< Enable/disable ADC converter high pass filter, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      zero_cal              : 1;  /*!< Zero calibration method select, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      filler2               : 1;  /*!< RESERVED */ 
  unsigned long long      packed_data           : 1;  /*!< Enable/disable data packing in PCI&FPDP, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      acq_mode              : 2;  /*!< Determines acquisition mode, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      system_master         : 1;  /*!< Select system master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_master            : 1;  /*!< Select ADC master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_master           : 1;  /*!< Select FPDP master board, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      adc_termin            : 1;  /*!< Enable ADC termination on local bus, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      fpdp_termin           : 1;  /*!< Enable FPDP termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      extclk_term           : 1;  /*!< Enable external clock termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      extrig_term           : 1;  /*!< Enable external trigger termination, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      extrig_mode           : 2;  /*!< Determines external triggering mode, e.g. \ref ICS710_EXTRIG_HIGH, \ref ICS710_EXTRIG_LOW, \ref ICS710_EXTRIG_RAISING or \ref ICS710_EXTRIG_FALLING */
  unsigned long long      filler1               : 41; /*!< Filler */
#endif
} ICS710_CONTROL,*pICS710_CONTROL;


/*! \struct  ICS710_MASTER_CONTROL ics710api.h "inc/ics710api.h"
 *  \brief   Master Control Register content
 *  \remarks The Master Control Register is used to enable the ADCs and DACs \n
 *           and to control their modes of operation.
 */
typedef struct {
#ifdef ICS710_BE
  unsigned long long      filler1               : 49;
  unsigned long long      board_address         : 5;
  unsigned long long      numbers_of_channels   : 10;
#else
  unsigned long long      numbers_of_channels   : 10;
  unsigned long long      board_address         : 5;
  unsigned long long      filler1               : 49;
#endif
} ICS710_MASTER_CONTROL,*pICS710_MASTER_CONTROL;


/*! \struct  ICS710_GAIN ics710api.h "inc/ics710api.h"
 *  \brief   Gain Register content (input voltage bits)
 *  \remarks The Gain Register is used to set \n
 *           and to control gain (bits <3-0> in the Gain Register).
 */
typedef struct {
#ifdef ICS710_BE
  unsigned long long      filler1               :60;
  unsigned long long      input_voltage_range   :4;  /*!< Refer to settings in the ICS-710 Operating Manual, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
#else
  unsigned long long      input_voltage_range   :4;  /*!< Refer to settings in the ICS-710 Operating Manual, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      filler1               :60;
#endif
} ICS710_GAIN,*pICS710_GAIN;


/*! \struct  ICS710_FILTER ics710api.h "inc/ics710api.h"
 *  \brief   Gain Register content (cut-off frequency bits)
 *  \remarks The Gain Register is used to set and to control \n
 *           cut-off frequency range (bits <7-4> in the Gain Register).
 */
typedef struct {
#ifdef ICS710_BE
  unsigned long long      filler2               : 56;
  unsigned long long      cutoff_freq_range     : 4;  /*!< Refer to settings in the ICS-710 Operating Manual, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      filler1               : 4;
#else
  unsigned long long      filler1               : 4;
  unsigned long long      cutoff_freq_range     : 4;  /*!< Refer to settings in the ICS-710 Operating Manual, e.g. \ref ICS710_ACTIVE or \ref ICS710_INACTIVE */
  unsigned long long      filler2               : 56;
#endif
}ICS710_FILTER,*pICS710_FILTER;


#pragma pack(pop)

#ifndef DOC_THIS

#if !(defined(NT4_DRIVER) || defined(FOR_WDM))

/* DLL support */
#ifdef EXPORT
#undef EXPORT
#endif

#ifdef WIN32
#ifdef ICS710API_EXPORTS
    #define EXPORT __declspec(dllexport) WINAPI
#else
    #define EXPORT __declspec(dllimport) WINAPI
#endif
#else
    #define EXPORT
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* ICS710 Motherboard API calls */

void* EXPORT ics710AllocateDmaBuffer   (HANDLE hnd,  int                   size);
void* EXPORT ics710FreeDmaBuffer       (HANDLE hnd,  void                 *address, int size);

int EXPORT ics710ReadDoneGet            (HANDLE hnd, int                   *pDone);
int EXPORT ics710WriteDoneGet           (HANDLE hnd, int                   *pDone);

int EXPORT ics710WaitADCInt             (HANDLE hnd, int                   *timeout);
int EXPORT ics710ADCDoneGet             (HANDLE hnd, int                   *pDone);
int EXPORT ics710WaitPCIOverflowInt     (HANDLE hnd, int                   *timeout);
int EXPORT ics710WaitPCIOverflowDoneGet (HANDLE hnd, int                   *pDone);
int EXPORT ics710WaitFPDPOverflowInt    (HANDLE hnd, int                   *timeout);
int EXPORT ics710WaitFPDPOverflowDoneGet(HANDLE hnd, int                   *pDone);

int EXPORT ics710StatusGet              (HANDLE hnd, ICS710_STATUS         *pStatus);
int EXPORT ics710ControlGet             (HANDLE hnd, ICS710_CONTROL        *pControl);
int EXPORT ics710ControlSet             (HANDLE hnd, ICS710_CONTROL        *pControl);
int EXPORT ics710MasterControlSet       (HANDLE hnd, ICS710_MASTER_CONTROL *pMasterControl);
int EXPORT ics710MasterControlGet       (HANDLE hnd, ICS710_MASTER_CONTROL *pMasterControl);
int EXPORT ics710GainGet                (HANDLE hnd, ICS710_GAIN           *pGain);
int EXPORT ics710GainSet                (HANDLE hnd, ICS710_GAIN           *pGain);
int EXPORT ics710FilterGet              (HANDLE hnd, ICS710_FILTER         *pFilter);
int EXPORT ics710FilterSet              (HANDLE hnd, ICS710_FILTER         *pFilter);
int EXPORT ics710ChannelCountGet        (HANDLE hnd, unsigned long long    *pChannel);
int EXPORT ics710ChannelCountSet        (HANDLE hnd, unsigned long long    *pChannel);
int EXPORT ics710BufferLengthGet        (HANDLE hnd, unsigned long long    *pLength);
int EXPORT ics710BufferLengthSet        (HANDLE hnd, unsigned long long    *pLength);
int EXPORT ics710DecimationGet          (HANDLE hnd, unsigned long long    *pDec);
int EXPORT ics710DecimationSet          (HANDLE hnd, unsigned long long    *pDec);
int EXPORT ics710AcquireCountGet        (HANDLE hnd, unsigned long long    *pCount);
int EXPORT ics710AcquireCountSet        (HANDLE hnd, unsigned long long    *pCount);
int EXPORT ics710FPDPFramesGet          (HANDLE hnd, unsigned long long    *pValue);
int EXPORT ics710FPDPFramesSet          (HANDLE hnd, unsigned long long    *pValue);

int EXPORT ics710DmaLocalSpaceSet       (HANDLE hnd, unsigned long long    *pValue);

int EXPORT ics710ADCClockSet            (HANDLE hnd, double                *pClock, double *pActualFreq);
int EXPORT ics710FPDPClockSet           (HANDLE hnd, double                *pClock, double *pActualFreq);

int EXPORT ics710Enable                 (HANDLE hnd);
int EXPORT ics710Disable                (HANDLE hnd);
int EXPORT ics710Arm                    (HANDLE hnd);
int EXPORT ics710Trigger                (HANDLE hnd);
int EXPORT ics710UnTrigger              (HANDLE hnd);
int EXPORT ics710ADCReset               (HANDLE hnd);
int EXPORT ics710ADCResetBlock          (HANDLE hnd, double                 sampleRate);
int EXPORT ics710ADCResetNoBlock        (HANDLE hnd, double                 sampleRate, unsigned int *timeToWait);
int EXPORT ics710BufferReset            (HANDLE hnd);
int EXPORT ics710BoardReset             (HANDLE hnd);


int EXPORT ics710ADCFPDPDefaultClockSet (HANDLE hnd, double *pADCFreq, double *pFPDPFreq);

#ifdef __cplusplus
}
#endif


#endif /* (!defined(NT4_DRIVER) && !defined(WDM_DRIVER)) */

#endif /* DOC_THIS */


#endif /* _ICS710API_H_ */
