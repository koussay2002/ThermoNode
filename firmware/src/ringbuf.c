#include "ringbuf.h"

static frame_t buf[RINGBUF_CAP];
static size_t  head;   /* index d'écriture */
static size_t  tail;   /* index de lecture */
static size_t  count;

void ringbuf_reset(void) { head = 0u; tail = 0u; count = 0u; }
size_t ringbuf_count(void) { return count; }

bool ringbuf_push(const frame_t *f)
{
    bool kept = true;
    if (count >= RINGBUF_CAP) {
        tail = (tail + 1u) % RINGBUF_CAP;   /* écrase la plus ancienne */
        count--;
        kept = false;
    }
    buf[head] = *f;
    head = (head + 1u) % RINGBUF_CAP;
    count++;
    return kept;
}

bool ringbuf_pop(frame_t *f)
{
    if (count == 0u) {
        return false;
    }
    *f = buf[tail];
    tail = (tail + 1u) % RINGBUF_CAP;
    count--;
    return true;
}

bool ringbuf_peek(frame_t *f)
{
    if (count == 0u) {
        return false;
    }
    *f = buf[tail];
    return true;
}
