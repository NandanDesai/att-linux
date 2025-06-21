#include "att_queue.h"            // For ATT_MAX_MSG_SIZE, ATT_FIFO_SIZE, function declarations

// Internal FIFO and synchronization
static DEFINE_SPINLOCK(fifo_lock);
static DECLARE_WAIT_QUEUE_HEAD(fifo_waitq);

struct kfifo att_fifo;

/**
 * att_enqueue_message - enqueue a byte array to the FIFO
 * @input: pointer to data buffer
 * @len: length of data
 *
 * This is intended to be called from a security hook context.
 * It must be fast, non-blocking, and concurrency-safe.
 */
int att_enqueue_message(const unsigned char *input, size_t len)
{
    unsigned long flags;
    struct att_msg msg;

    if (!input || len == 0)
        return -EINVAL;

    if (len >= ATT_MAX_MSG_SIZE)
        len = ATT_MAX_MSG_SIZE - 1;

    memset(&msg, 0, sizeof(struct att_msg));
    memcpy(msg.data, input, len);
    msg.data[len] = '\0';

    spin_lock_irqsave(&fifo_lock, flags);
    if (!kfifo_is_full(&att_fifo)) {
        kfifo_in(&att_fifo, &msg, sizeof(struct att_msg));
        spin_unlock_irqrestore(&fifo_lock, flags);
        wake_up_interruptible(&fifo_waitq);
        pr_info("att-queue: Enqueued (att_enqueue_message): %s\n", msg.data);
        return 0;
    } else {
        spin_unlock_irqrestore(&fifo_lock, flags);
        pr_warn("att-queue: FIFO full, message dropped\n");
        return -ENOSPC;
    }
}

/**
 * att_worker_thread - long-lived secure kernel thread
 *
 * This thread is not stoppable by userspace and is expected to run
 * for the full system uptime. It processes messages securely.
 */
int att_worker_thread(void *data)
{
    struct att_msg msg;
    unsigned long flags;

    pr_info("att-queue: Secure worker thread started on CPU %d\n", smp_processor_id());

    while (!kthread_should_stop()) {
        // Sleep until new data arrives or shutdown is requested
        wait_event_interruptible(fifo_waitq,
            !kfifo_is_empty(&att_fifo) || kthread_should_stop());

        if (kthread_should_stop())
            break;

        while (1) {
            spin_lock_irqsave(&fifo_lock, flags);
            if (kfifo_is_empty(&att_fifo)) {
                spin_unlock_irqrestore(&fifo_lock, flags);
                break;
            }

            int ret = kfifo_out(&att_fifo, &msg, sizeof(struct att_msg));
            spin_unlock_irqrestore(&fifo_lock, flags);

            if (ret != sizeof(struct att_msg)) {
                pr_warn("att-queue: Partial message received, size=%d\n", ret);
                break;
            }

            // Do the actual work outside the lock
            pr_info("att-queue: Securely processing (att_worker_thread): %s\n", msg.data);
        }
    }

    pr_info("att-queue: Secure worker thread shutting down.\n");
    return 0;
}
