#include "core.h"
#include "tr.h"

#define NACK_RATE_UNIT   10000

static ssize_t flush_send(ECPConnection *conn, ECPTimerItem *ti) {
    ECPBuffer packet;
    ECPBuffer payload;
    unsigned char pkt_buf[ECP_SIZE_PKT_BUF(0, ECP_MTYPE_RBFLUSH, conn)];
    unsigned char pld_buf[ECP_SIZE_PLD_BUF(0, ECP_MTYPE_RBFLUSH, conn)];

    packet.buffer = pkt_buf;
    packet.size = ECP_SIZE_PKT_BUF(0, ECP_MTYPE_RBFLUSH, conn);
    payload.buffer = pld_buf;
    payload.size = ECP_SIZE_PLD_BUF(0, ECP_MTYPE_RBFLUSH, conn);

    ecp_pld_set_type(payload.buffer, payload.size, ECP_MTYPE_RBFLUSH);
    if (ti == NULL) {
        ECPTimerItem _ti;
        int rv;

        rv = ecp_timer_item_init(&_ti, conn, ECP_MTYPE_RBACK, 3, 500);
        if (rv) return rv;

        _ti.retry = flush_send;
        rv = ecp_timer_push(&_ti);
        if (rv) return rv;
    }
    return ecp_rbuf_pld_send(conn, &packet, &payload, ECP_SIZE_PLD(0, ECP_MTYPE_RBFLUSH), 0, 0);
}

static void cc_flush(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;
    ECPRBuffer *rbuf = &buf->rbuf;
    ecp_seq_t pkt_buf_cnt = rbuf->seq_max - rbuf->seq_start + 1;
    ecp_win_t pkt_cc_cnt = buf->win_size > buf->in_transit ? buf->win_size - buf->in_transit : 0;
    int pkt_to_send = pkt_buf_cnt < pkt_cc_cnt ? pkt_buf_cnt : pkt_cc_cnt;
    int i;

    ECPTimerItem ti[ECP_MAX_TIMER];
    unsigned short max_t = 0;

    if (pkt_to_send) {
        unsigned int idx = _ecp_rbuf_msg_idx(rbuf, buf->seq_cc);
        unsigned int _idx = idx;

        for (i=0; i<pkt_to_send; i++) {
            if (!(rbuf->msg[_idx].flags & ECP_RBUF_FLAG_IN_CCONTROL)) break;
            rbuf->msg[_idx].flags &= ~ECP_RBUF_FLAG_IN_CCONTROL;
            if (rbuf->msg[_idx].idx_t != -1) {
                ECPRBTimer *timer = &buf->timer;
                ECPRBTimerItem *item = &timer->item[rbuf->msg[_idx].idx_t];

                item->occupied = 0;
                ti[max_t] = item->item;
                rbuf->msg[_idx].idx_t = max_t;
                max_t++;
            }
            _idx = ECP_RBUF_IDX_MASK(_idx + 1, rbuf->msg_size);
        }
        pkt_to_send = i;
        _idx = idx;

#ifdef ECP_WITH_PTHREAD
        pthread_mutex_unlock(&buf->mutex);
#endif

        for (i=0; i<pkt_to_send; i++) {
            ECPBuffer packet;

            if (rbuf->msg[_idx].idx_t != -1) ecp_timer_push(&ti[rbuf->msg[_idx].idx_t]);
            packet.buffer = rbuf->msg[_idx].msg;
            packet.size = rbuf->msg[_idx].size;
            ecp_pkt_send(conn->sock, &conn->node.addr, &packet, rbuf->msg[_idx].size, 0);
            _idx = ECP_RBUF_IDX_MASK(_idx + 1, rbuf->msg_size);
        }

#ifdef ECP_WITH_PTHREAD
        pthread_mutex_lock(&buf->mutex);
#endif

        buf->in_transit += (ecp_win_t)pkt_to_send;
        buf->cnt_cc -= (ecp_win_t)pkt_to_send;
        buf->seq_cc += (ecp_seq_t)pkt_to_send;
    }
}

ssize_t ecp_rbuf_handle_ack(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size, ECP2Buffer *b) {
    ECPRBSend *buf;
    ssize_t rsize = sizeof(ecp_seq_t)+sizeof(ecp_ack_t);
    ecp_seq_t seq_ack = 0;
    ecp_ack_t ack_map = 0;
    int i;
    int do_flush = 0;
    int rv = ECP_OK;

    buf = conn->rbuf.send;
    if (buf == NULL) return size;
    if (size < 0) return size;
    if (size < rsize) return ECP_ERR;

    seq_ack = \
        (msg[0] << 24) | \
        (msg[1] << 16) | \
        (msg[2] << 8)  | \
        (msg[3]);
    ack_map = \
        (msg[4] << 24) | \
        (msg[5] << 16) | \
        (msg[6] << 8)  | \
        (msg[7]);

    ecp_tr_release(b->packet, 1);
    ecp_tr_flag_set(ECP_SEND_FLAG_MORE);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif

    ECPRBuffer *rbuf = &buf->rbuf;
    int idx = _ecp_rbuf_msg_idx(rbuf, seq_ack);
    if (idx < 0) rv = idx;

    if (!rv) {
        seq_ack++;
        if (buf->flags & ECP_RBUF_FLAG_CCONTROL) buf->in_transit = buf->cnt_cc ? buf->seq_cc - seq_ack : rbuf->seq_max - seq_ack + 1;

        if (ack_map != ECP_ACK_FULL) {
            int nack_first = 0;
            unsigned int msg_start;
            ecp_seq_t seq_start;
            ecp_win_t nack_cnt = 0;

            seq_ack -= ECP_SIZE_ACKB;

#ifdef ECP_WITH_PTHREAD
            pthread_mutex_unlock(&buf->mutex);
#endif

            for (i=0; i<ECP_SIZE_ACKB; i++) {
                if ((ack_map & ((ecp_ack_t)1 << (ECP_SIZE_ACKB - i - 1))) == 0) {
                    nack_cnt++;
                    if (buf->flags & ECP_RBUF_FLAG_RELIABLE) {
                        unsigned int _idx = ECP_RBUF_IDX_MASK(idx + 1 - ECP_SIZE_ACKB + i, rbuf->msg_size);
                        if ((rbuf->msg[_idx].size == 0) || (rbuf->msg[_idx].flags & ECP_RBUF_FLAG_SYS)) {
                            ECPBuffer packet;
                            ECPBuffer payload;
                            unsigned char pkt_buf[ECP_SIZE_PKT_BUF(0, ECP_MTYPE_NOP, conn)];
                            unsigned char pld_buf[ECP_SIZE_PLD_BUF(0, ECP_MTYPE_NOP, conn)];

                            packet.buffer = pkt_buf;
                            packet.size = ECP_SIZE_PKT_BUF(0, ECP_MTYPE_NOP, conn);
                            payload.buffer = pld_buf;
                            payload.size = ECP_SIZE_PLD_BUF(0, ECP_MTYPE_NOP, conn);

                            ecp_pld_set_type(payload.buffer, payload.size, ECP_MTYPE_NOP);
                            ecp_rbuf_pld_send(conn, &packet, &payload, ECP_SIZE_PLD(0, ECP_MTYPE_NOP), 0, seq_ack + i);
                        } else {
                            ECPBuffer packet;
                            packet.buffer = rbuf->msg[_idx].msg;
                            packet.size = rbuf->msg[_idx].size;
                            ecp_pkt_send(conn->sock, &conn->node.addr, &packet, rbuf->msg[_idx].size, 0);
                        }
                        if (!nack_first) {
                            nack_first = 1;
                            seq_start = seq_ack + i;
                            msg_start = _idx;
                        }
                    }
                }
            }

#ifdef ECP_WITH_PTHREAD
            pthread_mutex_lock(&buf->mutex);
#endif

            if (buf->flags & ECP_RBUF_FLAG_CCONTROL) buf->in_transit += nack_cnt;
            buf->nack_rate = (buf->nack_rate * 7 + ((ECP_SIZE_ACKB - nack_cnt) * NACK_RATE_UNIT) / ECP_SIZE_ACKB) / 8;
            if (buf->flags & ECP_RBUF_FLAG_RELIABLE) {
                rbuf->seq_start = seq_start;
                rbuf->msg_start = msg_start;
            } else {
                rbuf->seq_start = seq_ack + ECP_SIZE_ACKB;
                rbuf->msg_start = ECP_RBUF_IDX_MASK(idx + 1, rbuf->msg_size);
            }
        } else {
            buf->nack_rate = (buf->nack_rate * 7) / 8;
            rbuf->seq_start = seq_ack;
            rbuf->msg_start = ECP_RBUF_IDX_MASK(idx + 1, rbuf->msg_size);
        }
        if (buf->flush) {
            if (ECP_SEQ_LT(buf->seq_flush, rbuf->seq_start)) buf->flush = 0;
            if (buf->flush) do_flush = 1;
        }
        if (buf->cnt_cc) cc_flush(conn);
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    if (!rv && do_flush) {
        ssize_t _rv;

        _rv = flush_send(conn, NULL);
        if (_rv < 0) rv = _rv;
    }

    ecp_tr_flag_clear(ECP_SEND_FLAG_MORE);
    ecp_tr_release(b->packet, 0);

    if (rv) return rv;
    return rsize;
}

int ecp_rbuf_send_create(ECPConnection *conn, ECPRBSend *buf, ECPRBMessage *msg, unsigned int msg_size) {
    int rv;

    memset(buf, 0, sizeof(ECPRBRecv));
    rv = _ecp_rbuf_init(&buf->rbuf, msg, msg_size);
    if (rv) return rv;

#ifdef ECP_WITH_PTHREAD
    rv = pthread_mutex_init(&buf->mutex, NULL);
    if (rv) return ECP_ERR;
#endif

    conn->rbuf.send = buf;
    return ECP_OK;
}

void ecp_rbuf_send_destroy(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;

    if (buf == NULL) return;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_destroy(&buf->mutex);
#endif

    conn->rbuf.send = NULL;
}

int ecp_rbuf_send_start(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;

    if (buf == NULL) return ECP_ERR;

    return _ecp_rbuf_start(&buf->rbuf, conn->seq_out);
}

int ecp_rbuf_set_wsize(ECPConnection *conn, ecp_win_t size) {
    ECPRBSend *buf = conn->rbuf.send;

    if (buf == NULL) return ECP_ERR;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif

    buf->win_size = size;
    if (buf->cnt_cc) cc_flush(conn);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    return ECP_OK;
}

int ecp_rbuf_flush(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;
    ecp_seq_t seq;
    ssize_t rv;

    if (buf == NULL) return ECP_ERR;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&conn->mutex);
#endif
    seq = conn->seq_out;
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&conn->mutex);
#endif

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif

    if (buf->flush) {
        if (ECP_SEQ_LT(buf->seq_flush, seq)) buf->seq_flush = seq;
    } else {
        buf->flush = 1;
        buf->seq_flush = seq;
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    rv = flush_send(conn, NULL);
    if (rv < 0) return rv;

    return ECP_OK;
}
