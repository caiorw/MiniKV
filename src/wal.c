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
    /* TODO: Adicionar código aqui se necessário! */

    self->buffer.records = malloc((size_t)config.buffer_size * sizeof(log_record_t));
    self->buffer_temp.records = malloc((size_t)config.buffer_size * sizeof(log_record_t));
    self->buffer.num_rec = 0;
    self->lsn = -1;
    self->encerrando = 0;

    pthread_mutex_init(&self->mutex_buffer, NULL);
    pthread_mutex_init(&self->mutex_durable, NULL);
    pthread_cond_init(&self->cond_nao_cheio, NULL);
    pthread_cond_init(&self->cond_escreveu, NULL);
    pthread_cond_init(&self->cond_nao_vazio, NULL);

    plog("[wal] Initialized\n");
}

// função nova para montar record
log_record_t make_record(long lsn, const char *from, const char *to, int amount) {
    log_record_t record;
    record.lsn = lsn;
    // copia string de from para from de log_record_t
    snprintf(record.from, sizeof(record.from), "%s", from);
    snprintf(record.to, sizeof(record.to), "%s", to);
    record.amount = amount;

    return record;
}

void wal_finalize(wal_t *self)
{
    /* TODO: Adicionar código aqui se necessário! */
    free(self->buffer.records);
    free(self->buffer_temp.records);
    pthread_mutex_destroy(&self->mutex_buffer);
    pthread_mutex_destroy(&self->mutex_durable);
    pthread_cond_destroy(&self->cond_nao_vazio);
    pthread_cond_destroy(&self->cond_escreveu);
    pthread_cond_destroy(&self->cond_nao_cheio);

    plog("[wal] Finalized\n");
}

long wal_append(wal_t *self, const char *from, const char *to, int amount)
{
    pthread_mutex_lock(&self->mutex_buffer);

    // enquanto o buffer estiver vazio, thread de log dorme
    // a thread de log so acorda quando cond_nao_cheio avisa
    while (self->buffer.num_rec == config.buffer_size) {
        pthread_cond_wait(&self->cond_nao_cheio, &self->mutex_buffer);
    }
    long lsn_return = ++self->lsn;
    int i = self->buffer.num_rec;
    // monta o record
    self->buffer.records[i] = make_record(lsn_return, from, to, amount);
    self->buffer.num_rec++;
    // avisa que produziu 1 record
    pthread_cond_signal(&self->cond_nao_vazio);
    pthread_mutex_unlock(&self->mutex_buffer);

    return lsn_return;
}

void wal_wait_durable(wal_t *self, long lsn)
{
    /* TODO: Implementar! */
    pthread_mutex_lock(&self->mutex_durable);
    // threads clientes dormem enquanto o lsn passado não é menor ou igual ao maior lsn do disco (ainda nao foi gravado)
    while (lsn > disk_durable_lsn(self->disk)) {
        pthread_cond_wait(&self->cond_escreveu, &self->mutex_durable);
    }
    pthread_mutex_unlock(&self->mutex_durable);
}

// função nova, chamada no shutdown 
// aqui define a flag encerrando como 1 e acorda a thread de log dormindo em cond_nao_vazio para sair do laço while em wal_writer_run e seguir com o shutdown 
void wal_close(wal_t *self) {
    pthread_mutex_lock(&self->mutex_buffer);
    self->encerrando = 1;
    pthread_cond_signal(&self->cond_nao_vazio);
    pthread_mutex_unlock(&self->mutex_buffer);
}

void * wal_writer_run(void *arg)
{
    wal_t *self = (wal_t *) arg;

    plog("[wal] Log writer is now running!\n");

    /* TODO: Implementar! Enquanto o log não for encerrado, retire todos os
       registros disponíveis no buffer e grave-os com disk_write_batch(). */
    (void) self;

    // fica em loop até thread de log ser encerrada
    while(1) {
        pthread_mutex_lock(&self->mutex_buffer);
        // equanto o buffer estiver vazio e a thread de log nao estiver encerrando, a thread dorme até que algo seja produzido no buffer
        while(self->buffer.num_rec == 0 && !self->encerrando) {
            pthread_cond_wait(&self->cond_nao_vazio, &self->mutex_buffer);
        }
        // sai do loop só quando o buffer ta vazio mesmo
        if (self->buffer.num_rec == 0 && self->encerrando) {
            pthread_mutex_unlock(&self->mutex_buffer);
            break;
        }
        // copia buffer para um temporario para liberar ele logo
        unsigned int n = self->buffer.num_rec;
        memcpy(self->buffer_temp.records, self->buffer.records, n * sizeof(log_record_t));
        self->buffer.num_rec = 0;
        pthread_cond_broadcast(&self->cond_nao_cheio);
        pthread_mutex_unlock(&self->mutex_buffer);

        disk_write_batch(self->disk, self->buffer_temp.records, n);
        
    
        pthread_mutex_lock(&self->mutex_durable);
        pthread_cond_broadcast(&self->cond_escreveu);
        pthread_mutex_unlock(&self->mutex_durable);
        
    }

    plog("[wal] Log writer has finished!\n");
    return NULL;
}
