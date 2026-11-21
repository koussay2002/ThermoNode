#include "ringbuf.h"

static frame_t buf[RINGBUF_CAP];
static size_t  rb_head;   /* index d'écriture */
static size_t  tail;   /* index de lecture */
static size_t  rb_count;

void ringbuf_reset(void) { rb_head = 0u; tail = 0u; rb_count = 0u; }
size_t ringbuf_count(void) { return rb_count; }

bool ringbuf_push(const frame_t *f)
{
    bool kept = true;
    if (rb_count >= RINGBUF_CAP) {
        tail = (tail + 1u) % RINGBUF_CAP;   /* écrase la plus ancienne */
        rb_count--;
        kept = false;
    }
    buf[rb_head] = *f;
    rb_head = (rb_head + 1u) % RINGBUF_CAP;
    rb_count++;
    return kept;
}

bool ringbuf_pop(frame_t *f)
{
    if (rb_count == 0u) {
        return false;
    }
    *f = buf[tail];
    tail = (tail + 1u) % RINGBUF_CAP;
    rb_count--;
    return true;
}

bool ringbuf_peek(frame_t *f)
{
    if (rb_count == 0u) {
        return false;
    }
    *f = buf[tail];
    return true;
}
