/*
 * wal.h - O log (write-ahead log) com group commit: buffer de registros e
 * thread de log, que grava os registros em disco em lotes.
 *
 * Adicione atributos às structs e assinaturas de novas funções onde indicado.
 */

#ifndef __WAL_H__
#define __WAL_H__

    #include <pthread.h>
    #include <semaphore.h>
    #include "storage.h"

    /**
     * @brief O log (write-ahead log) com group commit.
     */
    typedef struct buffer {
        log_record_t *records;
        size_t num_rec;
    } buffer_t;

    typedef struct wal
    {
        disk_t *disk; /* Referência para o disco. */
        buffer_t buffer;
        buffer_t buffer_temp;
        long lsn;

        // um mutex que protege as operações de produtor e consumidor do buffer e outro que protege a escrita no disco
        // desse modo, ainda podemos produzir registros no buffer enquanto escrevemos um lote no disco
        pthread_mutex_t mutex_buffer, mutex_durable;
        pthread_cond_t cond_nao_cheio, cond_escreveu, cond_nao_vazio;
        int encerrando;
        /* TODO: Adicione aqui os atributos que achar necessários para implementar o
        comportamento do log. Esses atributos deverão ser usados pelas funções
        do log. */
    } wal_t;

    /*============================================================================*
     * Funções utilizadas em arquivos que incluem esse .h                         *
     *============================================================================*/

    /**
     * @brief Inicializa o log. O buffer de log deve comportar no máximo
     * config.buffer_size registros.
     *
     * @param self O log.
     * @param disk O disco.
     */
    extern void wal_init(wal_t *self, disk_t *disk);

    /**
     * @brief Finaliza o log.
     *
     * @param self O log.
     */
    extern void wal_finalize(wal_t *self);

    /**
     * @brief Coloca um registro de transferência no buffer de log. Se o buffer
     * estiver cheio, espera até haver espaço.
     *
     * @param self O log.
     * @param from Conta de origem.
     * @param to Conta de destino.
     * @param amount Valor transferido.
     *
     * @returns O LSN atribuído ao registro (0, 1, 2, ...).
     */
    extern long wal_append(wal_t *self, const char *from, const char *to, int amount);

    /**
     * @brief Espera até que o registro com LSN lsn tenha sido gravado em disco.
     *
     * @param self O log.
     * @param lsn O LSN do registro.
     */
    extern void wal_wait_durable(wal_t *self, long lsn);

    /**
     * @brief Função executada pela thread de log. Retira todos os registros
     * disponíveis no buffer e os grava de uma vez com disk_write_batch().
     *
     * @param arg O argumento de entrada da thread, que deverá ser um wal_t.
     */
    extern void * wal_writer_run(void *arg);

    /*============================================================================*
     * ATENÇÃO: Insira aqui funções que você quiser adicionar à interface para    *
     * serem usadas em arquivos que incluem esse header.                          *
     *============================================================================*/

     extern log_record_t make_record(long lsn, const char *from, const char *to, int amount);

     extern void wal_close(wal_t *wal);

#endif /*__WAL_H__*/
