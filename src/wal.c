/*
 * wal.c - O log (write-ahead log) com group commit.
 *
 * Implemente todas as funções deste arquivo.
 *
 * VOCÊ DEVE ALTERAR ESTE ARQUIVO.
 */

#include <stdlib.h>
#include <string.h>
#include "wal.h"

void wal_init(wal_t *self, disk_t *disk)
{
    self->disk = disk;
    self->buffer = malloc((size_t)config.buffer_size * sizeof(log_record_t));
    self->lsn = 0;
    /* TODO: Adicionar código aqui se necessário! */

    plog("[wal] Initialized\n");
}

void wal_finalize(wal_t *self)
{
    /* TODO: Adicionar código aqui se necessário! */
    free(self->buffer);
    plog("[wal] Finalized\n");
}

long wal_append(wal_t *self, const char *from, const char *to, int amount)
{
    /* TODO: Implementar! O código abaixo deve ser modificado! */
    self->lsn++;
    // tem mais lsn do que tamanho no buffer, faz mod para colocar no buffer
    int i = (int)self->lsn % config.buffer_size;

    // salva o record
    log_record_t record;
    record.lsn = self->lsn;
    // copia string de from para from de log_record_t
    snprintf(record.from, sizeof(record.from), "%s", from);
    snprintf(record.to, sizeof(record.to), "%s", to);
    record.amount = amount;

    self->buffer[i] = record;
    return self->lsn;
}

void wal_wait_durable(wal_t *self, long lsn)
{
    /* TODO: Implementar! */
}

void * wal_writer_run(void *arg)
{
    wal_t *self = (wal_t *) arg;

    plog("[wal] Log writer is now running!\n");

    /* TODO: Implementar! Enquanto o log não for encerrado, retire todos os
       registros disponíveis no buffer e grave-os com disk_write_batch(). */
    (void) self;

    plog("[wal] Log writer has finished!\n");
    return NULL;
}
