/*----------------------------------------------------------------------------
 *      Huawei - Huawei LiteOS
 *----------------------------------------------------------------------------
 *      Name:    Workqueue.h
 *      Purpose: High-precision-software-timer-related functions
 *      Rev.:    V1.0.0
 *----------------------------------------------------------------------------
 *

 * Copyright (c) 2014, Huawei Technologies Co., Ltd.
 * All rights reserved.
 *---------------------------------------------------------------------------*/

 /** @defgroup workqueue Workqueue
 *  @ingroup linux
*/
#ifndef __LITEOS_WORKQUEUE_H__
#define __LITEOS_WORKQUEUE_H__

#include <securec.h>
#include "los_task.h"
//#include <semaphore.h>
#include "los_memory.h"
#include "los_event.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */


struct lock_class_item {
};

/**
 * @ingroup workqueue
 * System default workqueue.
 *
 */
extern struct workqueue_body *g_pstSystemWq;

/**
 * @ingroup workqueue
 * Workqueue task control block structure.
 *
 */
typedef struct LOS_TASK_CB task_struct;

/**
 * @ingroup workqueue
 * Define atomic_long_t as a signed integer.
 *
 */
typedef int atomic_long_t;

struct work_body;

/**
 * @ingroup workqueue
 * Workqueue handling function.
 *
 */
typedef void (*work_func_t)(struct work_body *);


#define WORKQ_TASK_DEFAULT_PRI    31
#define WORKQ_IRQ_DEFAULT_PRI     15

#define WORKQUEUE_USER_DATA_MAX_NUM       8
/**
 * @ingroup workqueue
 * Work structure.
 * A work is a node in a workqueue.
 *
 */
struct work_body
{
    LOS_DL_LIST entry;    /**< Pointer to a doubly linked list of a work*/
    work_func_t func;           /**< Work handling function*/
    UINT32 work_status;     /**< Work status*/
    UINT16 work_priority;     /**< Work priority*/
    UINT16 work_recycle;
    atomic_long_t data[WORKQUEUE_USER_DATA_MAX_NUM]; /**< Input parameter of the work handling function*/
};

/**
 * @ingroup workqueue
 * Workqueue control structure.
 * It awakes a workqueue or makes a workqueue sleep.
 *
 */
typedef struct tag_cpu_workqueue_struct
{
    LOS_DL_LIST worklist;               /**< Pointer to a work doubly linked list that has been used.*/
    LOS_DL_LIST free_worklist;          /**< Pointer to a work doubly linked list that free to use.*/
    struct work_body *work_array;
    struct workqueue_body *wq;             /**< Workqueue that contains the workqueue control structure*/
    task_struct *thread;                     /**< Workqueue handling thread*/
}cpu_workqueue_struct;

/**
 * @ingroup workqueue
 * Definition of a workqueue structure.
 *
 */
struct workqueue_body
{
    cpu_workqueue_struct *cpu_wq;   /**< Workqueue control structure*/
    LOS_DL_LIST list;                     /**< Pointer to a workqueue doubly linked list*/
    EVENT_CB_S wq_event;               /**< Event of a workqueue */
    unsigned int wq_id;                                   /**< Workqueue ID  */
    int delayed_work_count;             /**< Number of delayed works in a workqueue*/
    char *name;                                /**< Workqueue name*/
    int singlethread;                         /**< Whether to create a new working task. 0 indicates that the default working task will be used.*/
    int wq_status;                            /**< Workqueue status*/
    int freezeable;                            /**< Not in use temporarily*/
    int rt;                                         /**< Not in use temporarily*/
};

/**
 * @ingroup workqueue
 * Work status enumeration.
 *
 */
enum work_status
{
    WORK_BUSY_PENDING    = 1<<0,    /**< The status of work item is pending execution*/
    WORK_BUSY_RUNNING    = 1<<1,    /**< The status of work item is running*/
    WORK_STRUCT_PENDING = 1<<0,    /**< work item is pending execution*/
    WORK_STRUCT_RUNNING = 1<<1,   /**< work item is running*/
};

/**
 * @ingroup workqueue
 * Work status enumeration.
 *
 */
enum work_recycle
{
    WORK_ONCE = 0,
    WORK_RECYCLE,
};
/**
 * @ingroup workqueue
 * Initialize a work.
 *
 */
#define INIT_WORK(_work, _func)          \
    do {      \
        LOS_ListInit(&((_work)->entry));    \
        (_work)->func = (_func);            \
        (VOID)memset_s((_work)->data, sizeof(int)*WORKQUEUE_USER_DATA_MAX_NUM, \
            0, sizeof(int)*WORKQUEUE_USER_DATA_MAX_NUM); \
        (_work)->work_status = 0;            \
        (_work)->work_recycle = WORK_ONCE;           \
        (_work)->work_priority = WORKQ_TASK_DEFAULT_PRI;\
    } while (0)

/**
 * @ingroup  workqueue
 * @brief Create a workqueue.
 *
 *@par Description:
 * This API is used to create a workqueue that has a specified name.
 *
 * @attention
 * <ul>
 * <li> The passed-in workqueue name is a character string that cannot be null and is the only identifier of the workqueue, make sure it is unique. </li>
 * </ul>
 *
 * @param  name  [IN] Workqueue name.
 *
 * @retval  NULL                   The workqueue fails to be created.
 * @retval  workqueue_body*      The workqueue is successfully created.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contains the API declaration.</li></ul>
 * @see destroy_workqueue
 * @since Huawei LiteOS V100R001C00
 */
extern struct workqueue_body * create_singlethread_workqueue(char *name);
/**
 * @ingroup  workqueue
 * @brief Create a workqueue.
 *
 *@par Description:
 * This API is used to create a workqueue that has a specified name.
 *
 * @attention
 * <ul>
 * <li> The passed-in workqueue name is a character string that cannot be null and is the only identifier of the workqueue, make sure it is unique. </li>
 * </ul>
 *
 * @param  name  [IN] Workqueue name.
 *
 * @retval  NULL                   The workqueue fails to be created.
 * @retval  workqueue_body*      The workqueue is successfully created.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contains the API declaration.</li></ul>
 * @see destroy_workqueue
 * @since Huawei LiteOS V100R001C00
 */
#define create_workqueue(name)      create_singlethread_workqueue(name)

/**
 * @ingroup  workqueue
 * @brief Delete a workqueue.
 *
 *@par Description:
 * This API is used to delete a workqueue that has a specified handle.
 *
 * @attention
 * <ul>
 * <li>The name of the workqueue will be null and the workqueue cannot be used again after the workqueue is deleted. </li>
 * </ul>
 *
 * @param  wq  [IN] Workqueue handle.
 *
 * @retval  None.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contains the API declaration.</li></ul>
 * @see create_workqueue
 * @since Huawei LiteOS V100R001C00
 */
void destroy_workqueue(struct workqueue_body *wq);

/**
 * @ingroup  workqueue
 * @brief Queue a work on a workqueue.
 *
 *@par Description:
 * This API is used to queue a work on a specified workqueue.
 *
 * @attention
 * <ul>
 * <li>The parameter wq and work shoud be valid memory, otherwise, the system may be abnormal. </li>
 * <li>The work will be immediately queued on the workqueue. </li>
 * </ul>
 *
 * @param  wq     [IN] Workqueue handle.
 * @param  work  [IN] Work handle.
 *
 * @retval #TRUE      The work is successfully queued on the workqueue.
 * @retval #FALSE     The work fails to be queued on the workqueue.
 * @par Dependency:
 * <ul>
 * <li>This function should be used after create_singlethread_workqueue() or create_workqueue() has been called</li>
 * <li>workqueue.h: the header file that contains the API declaration.</li>
 * </ul>
 * @see cancel_work_sync
 * @since Huawei LiteOS V100R001C00
 */
BOOL queue_work(struct workqueue_body *wq, struct work_body *work);
/**
 * @ingroup  workqueue
 * @brief Put a work in a default workqueue.
 *
 *@par Description:
 * This API is used to put a work in the default workqueue that is created when Huawei LiteOS is initialized.
 *
 * @attention
 * <ul>
 * <li>The parameter work shoud be valid memory, otherwise, the system may be abnormal. </li>
 * <li>The default workqueue is g_pSystemWq. </li>
 * </ul>
 *
 * @param  work        [IN] Work handle.
 *
 * @retval #TRUE      The work is successfully put in the workqueue.
 * @retval #FALSE     The work fails to be put in the workqueue.
 * @par Dependency:
 * <ul>
 * <li>This function should be used after create_singlethread_workqueue() or create_workqueue() has been called</li>
 * <li>workqueue.h: the header file that contains the API declaration.</li>
 * </ul>
 * @see cancel_work_sync
 * @since Huawei LiteOS V100R001C00
 */
BOOL schedule_work(struct work_body *work);
/**
 * @ingroup  workqueue
 * @brief Query the work status.
 *
 *@par Description:
 * This API is used to query the status of a work and a delayed work.
 *
 * @attention
 * <ul>
 * <li>The parameter dwork shoud be valid memory, otherwise, the system may be abnormal.</li>
 * </ul>
 *
 * @param  work          [IN] Work handle.
 *
 * @retval #WORK_BUSY_PENDING      The work is pending.
 * @retval #WORK_BUSY_RUNNING      The work is running.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contains the API declaration.</li></ul>
 * @see None.
 * @since Huawei LiteOS V100R001C00
 */
unsigned int work_busy(struct work_body *work);
/**
 * @ingroup  workqueue
 * @brief Immediately execute a work.
 *
 *@par Description:
 * This API is used to immediately execute a specified work and wait for the execution to end.
 *
 * @attention
 * <ul>
 * <li>flush_work() should be used after queue_work() has been called.</li>
 * <li>The parameter work shoud be valid memory, otherwise, the system may be abnormal.</li>
 * </ul>
 *
 * @param  work          [IN] Work handle.
 *
 * @retval #TRUE      The operation succeeds.
 * @retval #FALSE     The operation fails.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contains the API declaration.</li></ul>
 * @see None.
 * @since Huawei LiteOS V100R001C00
 */
BOOL flush_work(struct work_body *work);

/**
 * @ingroup  workqueue
 * @brief Cancel a work.
 *
 *@par Description:
 * This API is used to cancel a work that is pending or running.
 *
 * @attention
 * <ul>
 * <li>cancel_work_sync() should be used after queue_work() has been called.</li>
 * <li>The parameter work shoud be valid memory, otherwise, the system may be abnormal.</li>
 * </ul>
 *
 * @param  work          [IN] Work handle.
 *
 * @retval #TRUE      The work is successfully canceled.
 * @retval #FALSE     The work fails to be canceled.
 * @par Dependency:
 * <ul><li>workqueue.h: the header file that contain the API declaration.</li></ul>
 * @see queue_work
 * @since Huawei LiteOS V100R001C00
 */
BOOL cancel_work_sync(struct work_body *work);

typedef  VOID (*irq_bottom_half_handler_t)(struct work_body *);

/**
 *@ingroup interrupt
 *@brief Bottom half interrupt handler.
 *
 *@par Description:
 *This API is used to realize the function of the bottom half of an interrupt handler. The bottom half is executed in handler.
 *@attention
 *<ul>
 *<li>The passed work_queue must be valid rather than a wild pointer.</li>
 *<li>The input parameter handler and data must be valid, otherwise, the system may be abnormal.</li>
 *<li>The memory that is pointed to by work_body and dynamically requested by this API needs to be released in the bottom half interrupt handler specified by handler. If it is not released, memory leak will result.</li>
 *</ul>
 *
 *@param work_queue   [IN] Work queue created when Huawei LiteOS Kernel is initialized or created by users.
 *@param handler   [IN] Bottom half interrupt handler.
 *@param data   [IN] input param of work func.
 *@retval  #FALSE                                                              The bottom half interrupt handler fails to be created.
 *@retval  #TRUE                                                               The bottom half interrupt handler is successfully created.
 *@par Dependency:
 *<ul><li>interrupt.h: the header file that contains the API declaration.</li></ul>
 *@see
 *@since Huawei LiteOS V100R001C00
 */
BOOL irq_bottom_half(struct work_body *work);

int init_default_workqueue(UINT32 work_num);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* __LITEOS_WORKQUEUE_H__ */
