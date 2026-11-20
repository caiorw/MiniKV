/*
 * db.c - O banco: consultas, transferências e snapshots.
 *
 * Implemente as funções de sincronização (db_acquire_*, db_release_*,
 * db_snapshot_begin/end). As operações db_get, db_transfer e db_snapshot, no
 * final do arquivo, já estão prontas e NÃO devem ser alteradas.
 *
 * VOCÊ DEVE ALTERAR ESTE ARQUIVO.
 */

#include <semaphore.h>
#include <stdlib.h>
#include "db.h"
#include "config.h"

void db_init(db_t *self, wal_t *wal)
{
    /* Cria as contas (NÃO ALTERE!). */
    char key[KEY_LEN];
    self->wal = wal;
    table_init(&self->table, config.accounts);
    for (unsigned int i = 0; i < config.accounts; i++) {
        account_key(i, key);
        table_insert(&self->table, key, INITIAL_BALANCE);
    }

    /* TODO: Adicionar código aqui se necessário! */

    self->partitions = malloc((size_t)config.partitions * sizeof(partition_t));
    for (unsigned int i = 0; i < config.partitions; i++) {
        pthread_rwlock_init(&self->partitions[i].rwlock, NULL);
    }

    sem_init(self->semaphore, 0, config.buffer_size);

    plog("[db] Initialized\n");
}

void db_finalize(db_t *self)
{
    /* TODO: Adicionar código aqui se necessário! */
    for (unsigned int i = 0; i < config.partitions; i++) {
        pthread_rwlock_destroy(&self->partitions[i].rwlock);
    }
    free(self->partitions);
    table_finalize(&self->table);
    plog("[db] Finalized\n");
}

void db_acquire_read(db_t *self, const char *key)
{
    /* TODO: Implementar! */
    unsigned int p = db_partition_of(self, key);
    pthread_rwlock_rdlock(&self->partitions[p].rwlock);

}

void db_release_read(db_t *self, const char *key)
{
    /* TODO: Implementar! */
    unsigned int p = db_partition_of(self, key);
    pthread_rwlock_unlock(&self->partitions[p].rwlock);
}

void db_acquire_pair(db_t *self, const char *from, const char *to)
{
    /* TODO: Implementar! */
    unsigned int fromPartition = db_partition_of(self, from);
    unsigned int toPartition = db_partition_of(self, to);

    if (fromPartition == toPartition) {
        pthread_rwlock_wrlock(&self->partitions[fromPartition].rwlock);
    } else {
        pthread_rwlock_t *primeiro = fromPartition < toPartition ? &self->partitions[fromPartition].rwlock : &self->partitions[toPartition].rwlock;
        pthread_rwlock_t *segundo = fromPartition > toPartition ? &self->partitions[fromPartition].rwlock : &self->partitions[toPartition].rwlock;

        pthread_rwlock_wrlock(primeiro);
        pthread_rwlock_wrlock(segundo);
    }
    
}

void db_release_pair(db_t *self, const char *from, const char *to)
{
    /* TODO: Implementar! */
    unsigned int fromPartition = db_partition_of(self, from);
    unsigned int toPartition = db_partition_of(self, to);

    if (fromPartition == toPartition) {
        pthread_rwlock_unlock(&self->partitions[fromPartition].rwlock);
    } else {
        pthread_rwlock_t *primeiro = fromPartition < toPartition ? &self->partitions[fromPartition].rwlock : &self->partitions[toPartition].rwlock;
        pthread_rwlock_t *segundo = fromPartition > toPartition ? &self->partitions[fromPartition].rwlock : &self->partitions[toPartition].rwlock;

        pthread_rwlock_unlock(segundo);
        pthread_rwlock_unlock(primeiro);
    }
}

void db_snapshot_begin(db_t *self)
{
    /* TODO: Implementar! */
}

void db_snapshot_end(db_t *self)
{
    /* TODO: Implementar! */
}

/* --------------------------------------------------------- */
/* ATENÇÃO: Não será necessário modificar as funções abaixo! */
/* --------------------------------------------------------- */

unsigned int db_partition_of(db_t *self, const char *key)
{
    return table_hash(key) % config.partitions;
}

int db_get(db_t *self, const char *key)
{
    db_acquire_read(self, key);
    int balance = table_get(&self->table, key);
    db_release_read(self, key);
    return balance;
}

long db_transfer(db_t *self, const char *from, const char *to, int amount)
{
    long lsn = -1;

    /* Trava as duas contas e, se houver saldo, transfere e registra no log. */
    db_acquire_pair(self, from, to);
    if (table_get(&self->table, from) >= amount) {
        table_add(&self->table, from, -amount);
        table_add(&self->table, to, amount);
        lsn = wal_append(self->wal, from, to, amount);
    }
    db_release_pair(self, from, to);

    /* Só confirma ao cliente depois que o registro estiver em disco. */
    if (lsn >= 0)
        wal_wait_durable(self->wal, lsn);

    return lsn;
}

double db_snapshot(db_t *self, int *balances)
{
    double start = now_ms();
    db_snapshot_begin(self);
    double waited = now_ms() - start;

    for (unsigned int i = 0; i < table_size(&self->table); i++)
        balances[i] = table_value_at(&self->table, i);

    db_snapshot_end(self);
    return waited;
}
