/* ringbuf.h — ENF-05 mémorisation de 60 trames en cas de perte de liaison (FIFO circulaire) */
#ifndef TN_RINGBUF_H
#define TN_RINGBUF_H
#include "types.h"

#define RINGBUF_CAP 60u

void   ringbuf_reset(void);
/* Ajoute une trame ; si plein, écrase la plus ancienne et renvoie false (perte) */
bool   ringbuf_push(const frame_t *f);
/* Retire la plus ancienne ; false si vide */
bool   ringbuf_pop(frame_t *f);
/* Lit la plus ancienne sans la retirer ; false si vide */
bool   ringbuf_peek(frame_t *f);
size_t ringbuf_count(void);

#endif
