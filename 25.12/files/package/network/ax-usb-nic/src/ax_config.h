/* ax_config.h */
#ifndef __AX_CONFIG_H__
#define __AX_CONFIG_H__

// ==========================================
// 1. User Settings 1: Enable, 0: Disable
// ==========================================
#define ENABLE_IOCTL_DEBUG          0
#define ENABLE_AUTODETACH_FUNC      0
#define ENABLE_MAC_PASS             0
#define ENABLE_INT_AGGRESSIVE       1
#define ENABLE_INT_POLLING          0
#define ENABLE_AUTOSUSPEND          0
#define ENABLE_TX_TASKLET           0
#define ENABLE_RX_TASKLET           0
#define ENABLE_QUEUE_PRIORITY       0
#define ENABLE_LPM                  1
#define ENABLE_SUSPEND_LP           1

#define ENABLE_QAV                  0
#define ENABLE_RX_PREEMPT           0
#define ENABLE_COE                  0
#define ENABLE_LSO                  1

#define ENABLE_PTP_FUNC             0
#define ENABLE_PTP_DEBUG            0

#define ENABLE_PTP_125M_CLK         0
#define ENABLE_PTP_PPS              0

#define ENABLE_PTP_NORMAL_PKT       0


// ==========================================
// 2. Logic Mapping
// ==========================================

#if (ENABLE_IOCTL_DEBUG == 1)
    #undef  ENABLE_IOCTL_DEBUG
    #define ENABLE_IOCTL_DEBUG
#else
    #undef  ENABLE_IOCTL_DEBUG
#endif

#if (ENABLE_AUTODETACH_FUNC == 1)
    #undef  ENABLE_AUTODETACH_FUNC
    #define ENABLE_AUTODETACH_FUNC
#else
    #undef  ENABLE_AUTODETACH_FUNC
#endif

#if (ENABLE_MAC_PASS == 1)
    #undef  ENABLE_MAC_PASS
    #define ENABLE_MAC_PASS
#else
    #undef  ENABLE_MAC_PASS
#endif

#if (ENABLE_INT_AGGRESSIVE == 1)
    #undef  ENABLE_INT_AGGRESSIVE
    #define ENABLE_INT_AGGRESSIVE
#else
    #undef  ENABLE_INT_AGGRESSIVE
#endif

#if (ENABLE_INT_POLLING == 1)
    #undef  ENABLE_INT_POLLING/
    #define ENABLE_INT_POLLING
#else
    #undef  ENABLE_INT_POLLING
#endif

#if (ENABLE_AUTOSUSPEND == 1)
    #undef  ENABLE_AUTOSUSPEND
    #define ENABLE_AUTOSUSPEND
#else
    #undef  ENABLE_AUTOSUSPEND
#endif

#if (ENABLE_TX_TASKLET == 1)
    #undef  ENABLE_TX_TASKLET
    #define ENABLE_TX_TASKLET
#else
    #undef  ENABLE_TX_TASKLET
#endif

#if (ENABLE_RX_TASKLET == 1)
    #undef  ENABLE_RX_TASKLET
    #define ENABLE_RX_TASKLET
#else
    #undef  ENABLE_RX_TASKLET
#endif

#if (ENABLE_QUEUE_PRIORITY == 1)
    #undef  ENABLE_QUEUE_PRIORITY
    #define ENABLE_QUEUE_PRIORITY
#else
    #undef  ENABLE_QUEUE_PRIORITY
#endif

#if (ENABLE_LPM == 1)
    #undef  ENABLE_LPM
    #define ENABLE_LPM
#else
    #undef  ENABLE_LPM
#endif

#if (ENABLE_SUSPEND_LP == 1)
    #undef  ENABLE_SUSPEND_LP
    #define ENABLE_SUSPEND_LP
#else
    #undef  ENABLE_SUSPEND_LP
#endif

#if (ENABLE_QAV == 1)
    #undef  ENABLE_QAV
    #define ENABLE_QAV
#else
    #undef  ENABLE_QAV
#endif

#if (ENABLE_RX_PREEMPT == 1)
    #undef  ENABLE_RX_PREEMPT
    #define ENABLE_RX_PREEMPT
#else
    #undef  ENABLE_RX_PREEMPT
#endif

#if (ENABLE_COE == 1)
    #undef  ENABLE_COE
    #define ENABLE_COE
#else
    #undef  ENABLE_COE
#endif

#if (ENABLE_LSO == 1)
    #undef  ENABLE_LSO
    #define ENABLE_LSO
#else
    #undef  ENABLE_LSO
#endif

#if (ENABLE_PTP_FUNC == 1)
    #undef  ENABLE_PTP_FUNC
    #define ENABLE_PTP_FUNC

    #if (ENABLE_PTP_DEBUG == 1)
        #undef  ENABLE_PTP_DEBUG
        #define ENABLE_PTP_DEBUG
    #else
        #undef  ENABLE_PTP_DEBUG
    #endif

    #if (ENABLE_PTP_125M_CLK == 1)
        #undef  ENABLE_PTP_125M_CLK
        #define ENABLE_PTP_125M_CLK
    #else
        #undef  ENABLE_PTP_125M_CLK
    #endif

    #if (ENABLE_PTP_PPS == 1)
        #undef  ENABLE_PTP_PPS
        #define ENABLE_PTP_PPS
    #else
        #undef  ENABLE_PTP_PPS
    #endif
#else
    #undef  ENABLE_PTP_FUNC
#endif

#if (ENABLE_PTP_NORMAL_PKT == 1)
    #undef  ENABLE_PTP_NORMAL_PKT
    #define ENABLE_PTP_NORMAL_PKT
#else
    #undef  ENABLE_PTP_NORMAL_PKT
#endif

#endif /* __AX_CONFIG_H__ */