/* ---------------------- MISCELLANEOUS ----------------------- */
#define OS_APP_HOOKS_EN 1u /* Application-defined hooks arecalled from the uC/OS-II hooks */
#define OS_ARG_CHK_EN 1u /* Enable (1) or Disable (0) argumentchecking */
#define OS_CPU_HOOKS_EN 1u /* uC/OS-II hooks are found in theprocessor port files */
#define OS_DEBUG_EN 0u /* Enable(1) debug variables*/
#define OS_EVENT_MULTI_EN 1u /* Include code for OSEventPendMulti()*/
#define OS_EVENT_NAME_EN 1u /* Enable names for Sem, Mutex, Mboxand Q */
#define OS_LOWEST_PRIO 63u /* Defines the lowest priority thatcan be assigned ... */
								/* ... MUST NEVER be higher than 254!*/
#define OS_MAX_EVENTS 10u /* Max. number of event control blocksin your application */
#define OS_MAX_FLAGS 5u /* Max. number of Event Flag Groupsin your application */
#define OS_MAX_MEM_PART 5u /* Max. number of memory partitions*/
#define OS_MAX_QS 4u /* Max. number of queue control blocksin your application */
#define OS_MAX_TASKS 20u /* Max. number of tasks in yourapplication, MUST be >= 2 */
#define OS_SCHED_LOCK_EN 1u /* Include code for OSSchedLock() andOSSchedUnlock() */

#define OS_TICK_STEP_EN 1u /* Enable tick stepping feature foruC/OS-View */
#define OS_TICKS_PER_SEC 100u /* Set the number of ticks in onesecond */
#define OS_TLS_TBL_SIZE 0u /* Size of Thread-Local Storage Table*/

/* --------------------- TASK STACK SIZE ---------------------- */

#define OS_TASK_TMR_STK_SIZE 128u /* Timer task stack size (# ofOS_STK wide entries) */
#define OS_TASK_STAT_STK_SIZE 128u /* Statistics task stack size (# ofOS_STK wide entries) */
#define OS_TASK_IDLE_STK_SIZE 128u /* Idle task stack size (# ofOS_STK wide entries) */

/* --------------------- TASK MANAGEMENT ---------------------- */

#define OS_TASK_CHANGE_PRIO_EN 1u /* Include code forOSTaskChangePrio() */
#define OS_TASK_CREATE_EN 1u /* Include code for OSTaskCreate()*/
#define OS_TASK_CREATE_EXT_EN 1u /* Include code forOSTaskCreateExt() */
#define OS_TASK_DEL_EN 1u /* Include code for OSTaskDel()*/
#define OS_TASK_NAME_EN 1u /* Enable task names*/
#define OS_TASK_PROFILE_EN 1u /* Include variables in OS_TCB forprofiling */
#define OS_TASK_QUERY_EN 1u /* Include code for OSTaskQuery()*/
#define OS_TASK_REG_TBL_SIZE 1u /* Size of task variables array(#of INT32U entries) */
#define OS_TASK_STAT_EN 1u /* Enable (1) or Disable(0) thestatistics task */
#define OS_TASK_STAT_STK_CHK_EN 1u /* Check task stacks fromstatistic task */
#define OS_TASK_SUSPEND_EN 1u /* Include code forOSTaskSuspend() and OSTaskResume() */
#define OS_TASK_SW_HOOK_EN 1u /* Include code for OSTaskSwHook()*/

/* ----------------------- EVENT FLAGS ------------------------ */

#define OS_FLAG_EN 1u /* Enable (1) or Disable (0) codegeneration for EVENT FLAGS */
#define OS_FLAG_ACCEPT_EN 1u /* Include code for OSFlagAccept()*/
#define OS_FLAG_DEL_EN 1u /* Include code for OSFlagDel()*/
#define OS_FLAG_NAME_EN 1u /* Enable names for event flaggroup */
#define OS_FLAG_QUERY_EN 1u /* Include code for OSFlagQuery()*/
#define OS_FLAG_WAIT_CLR_EN 1u /* Include code for Wait on ClearEVENT FLAGS */
#define OS_FLAGS_NBITS 16u /* Size in #bits of OS_FLAGS data type(8, 16 or 32) */

/* -------------------- MESSAGE MAILBOXES --------------------- */

#define OS_MBOX_EN 1u /* Enable (1) or Disable (0) codegeneration for MAILBOXES */
#define OS_MBOX_ACCEPT_EN 1u /* Include code for OSMboxAccept()*/
#define OS_MBOX_DEL_EN 1u /* Include code for OSMboxDel()*/
#define OS_MBOX_PEND_ABORT_EN 1u /* Include code forOSMboxPendAbort() */
#define OS_MBOX_POST_EN 1u /* Include code for OSMboxPost()*/
#define OS_MBOX_POST_OPT_EN 1u /* Include code forOSMboxPostOpt() */
#define OS_MBOX_QUERY_EN 1u /* Include code for OSMboxQuery()*/

/* --------------------- MEMORY MANAGEMENT -------------------- */

#define OS_MEM_EN 1u /* Enable (1) or Disable (0) codegeneration for MEMORY MANAGER */
#define OS_MEM_NAME_EN 1u /* Enable memory partition names*/
#define OS_MEM_QUERY_EN 1u /* Include code for OSMemQuery()*/

/* ---------------- MUTUAL EXCLUSION SEMAPHORES --------------- */

#define OS_MUTEX_EN 1u /* Enable (1) or Disable (0) codegeneration for MUTEX */
#define OS_MUTEX_ACCEPT_EN 1u /* Include code forOSMutexAccept() */

#define OS_MUTEX_DEL_EN 1u /* Include code for OSMutexDel()*/
#define OS_MUTEX_QUERY_EN 1u /* Include code for OSMutexQuery()*/

/* ---------------------- MESSAGE QUEUES ---------------------- */

#define OS_Q_EN 1u /* Enable (1) or Disable (0) codegeneration for QUEUES */
#define OS_Q_ACCEPT_EN 1u /* Include code for OSQAccept()*/
#define OS_Q_DEL_EN 1u /* Include code for OSQDel()*/
#define OS_Q_FLUSH_EN 1u /* Include code for OSQFlush()*/
#define OS_Q_PEND_ABORT_EN 1u /* Include code for OSQPendAbort()*/
#define OS_Q_POST_EN 1u /* Include code for OSQPost()*/
#define OS_Q_POST_FRONT_EN 1u /* Include code for OSQPostFront()*/
#define OS_Q_POST_OPT_EN 1u /* Include code for OSQPostOpt()*/
#define OS_Q_QUERY_EN 1u /* Include code for OSQQuery()*/

/* ------------------------ SEMAPHORES ------------------------ */

#define OS_SEM_EN 1u /* Enable (1) or Disable (0) codegeneration for SEMAPHORES */
#define OS_SEM_ACCEPT_EN 1u /* Include code for OSSemAccept()*/
#define OS_SEM_DEL_EN 1u /* Include code for OSSemDel()*/
#define OS_SEM_PEND_ABORT_EN 1u /* Include code forOSSemPendAbort() */
#define OS_SEM_QUERY_EN 1u /* Include code for OSSemQuery()*/
#define OS_SEM_SET_EN 1u /* Include code for OSSemSet()*/

/* --------------------- TIME MANAGEMENT ---------------------- */

#define OS_TIME_DLY_HMSM_EN 1u /* Include code forOSTimeDlyHMSM() */
#define OS_TIME_DLY_RESUME_EN 1u /* Include code forOSTimeDlyResume() */

#define OS_TIME_GET_SET_EN 1u /* Include code for OSTimeGet()and OSTimeSet() */
#define OS_TIME_TICK_HOOK_EN 1u /* Include code forOSTimeTickHook() */

/* --------------------- TIMER MANAGEMENT --------------------- */

#define OS_TMR_EN 0u /* Enable (1) or Disable (0) codegeneration for TIMERS */
#define OS_TMR_CFG_MAX 16u /* Maximum number of timers*/
#define OS_TMR_CFG_NAME_EN 1u /* Determine timer names*/
#define OS_TMR_CFG_WHEEL_SIZE 7u /* Size of timer wheel (#Spokes)*/
#define OS_TMR_CFG_TICKS_PER_SEC 10u /* Rate at which timer managementtask runs (Hz) */

/* ---------------------- TRACE RECORDER ---------------------- */
#define OS_TRACE_EN 0u /* Enable (1) or Disable (0) uC/OS-IITrace instrumentation */
#define OS_TRACE_API_ENTER_EN 0u /* Enable (1) or Disable (0) uC/OS-IITrace API enter instrum. */
#define OS_TRACE_API_EXIT_EN 0u /* Enable (1) or Disable (0) uC/OS-IITrace API exit instrum. */
