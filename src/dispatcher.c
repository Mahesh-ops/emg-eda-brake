#include "../include/dispatcher.h"

/* Include all downstream consumers */
#include "../include/input_acq.h"
#include "../include/signal_proc.h"
#include "../include/safety_manager.h"
#include "../include/supervisor.h"
#include "../include/output_manager.h"
#include "../include/hal.h"

/* ── Configuration ──────────────────────────────────────────────────── */

#define DISPATCHER_QUEUE_SIZE 64  /**< Maximum number of pending events */

/* ── Internal State ─────────────────────────────────────────────────── */

static DispatchEvent evt_queue[DISPATCHER_QUEUE_SIZE];
static int head;
static int tail;

/* ── Public Interface ───────────────────────────────────────────────── */

void dispatcher_init(void)
{
    head = 0;
    tail = 0;
    hal_log("[DISP] dispatcher initialized");
}

void dispatcher_post(const DispatchEvent *ev)
{
    int next_head = (head + 1) % DISPATCHER_QUEUE_SIZE;
    
    if (next_head == tail) {
        /* Queue overflow: drop the event to prevent memory corruption */
        hal_log("[ERR] Dispatcher queue overflow! Event dropped.");
        return;
    }

    /* Copy the event payload into the ring buffer */
    evt_queue[head] = *ev;
    head = next_head;
}

void dispatcher_run_once(void)
{
    /* Process all events currently sitting in the queue.
     * Note: If a handler posts a new event, it will be added to the head
     * and processed sequentially within this same loop. */
    while (head != tail) {
        
        /* 1. Pop the oldest event */
        DispatchEvent ev = evt_queue[tail];
        tail = (tail + 1) % DISPATCHER_QUEUE_SIZE;

        /* 2. Route the event to the correct module handler */
        switch (ev.type) {
            
            case SYS_EVT_TICK_1MS:
                /* The 1ms heartbeat triggers hardware polling and log flushes */
                input_poll();
                output_flush_logs();
                break;

            case SYS_EVT_SAMPLES_PARSED:
                /* Send raw sample frames to the DSP feature engine */
                signal_proc_handle_event(&ev.payload.frame);
                break;

            case SYS_EVT_FEATURES_READY:
                /* Send extracted RMS/features to the intent threshold logic */
                safety_handle_event(&ev.payload.features);
                break;

            case SYS_EVT_INTENT_STATE:
                /* Send UML state transition requests to the Supervisor */
                supervisor_post_event(ev.payload.intent);
                break;

            case SYS_EVT_CMD_BRAKE:
                /* Send hardware actuation commands to the Output manager */
                output_set_brake_request(ev.payload.brake_cmd);
                break;

            default:
                hal_log("[WARN] Dispatcher received unknown event type");
                break;
        }
    }
}