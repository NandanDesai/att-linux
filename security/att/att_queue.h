#ifndef ATT_QUEUE_H
#define ATT_QUEUE_H

#include <linux/kfifo.h>          // For DECLARE_KFIFO, kfifo_in(), kfifo_out()
#include <linux/spinlock.h>       // For spinlock_t, spin_lock_irqsave/restore
#include <linux/kthread.h>        // For kthread_run(), kthread_should_stop(), struct task_struct
#include <linux/wait.h>           // For wait_event_interruptible(), DECLARE_WAIT_QUEUE_HEAD
#include <linux/string.h>         // For memcpy(), memset()
#include <linux/sched/signal.h>   // For sigfillset(), signal blocking
#include <linux/printk.h>         // For pr_info(), pr_warn()
#include <linux/smp.h>            // For smp_processor_id() (used in logging only)

// Max message size and queue size
#define ATT_MAX_MSG_SIZE 256
#define ATT_FIFO_SIZE    128

struct att_msg {
    unsigned char data[ATT_MAX_MSG_SIZE];
};

extern struct kfifo att_fifo;

// API to enqueue a message (non-blocking)
int att_enqueue_message(const unsigned char *input, size_t len);

// Kernel thread function to process dequeued messages
int att_worker_thread(void *data);

#endif // ATT_QUEUE_H
