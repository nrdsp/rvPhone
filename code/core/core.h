#define ECP_OK                      0
#define ECP_ERR                     -1
#define ECP_ERR_TIMEOUT             -2
#define ECP_ERR_ALLOC               -3

#define ECP_ERR_MAX_SOCK_CONN       -10
#define ECP_ERR_MAX_CTYPE           -11
#define ECP_ERR_MAX_MTYPE           -12
#define ECP_ERR_MIN_PKT             -13
#define ECP_ERR_MAX_PLD             -14
#define ECP_ERR_MIN_MSG             -15
#define ECP_ERR_MAX_MSG             -16
#define ECP_ERR_NET_ADDR            -17

#define ECP_ERR_CONN_NOT_FOUND      -20
#define ECP_ERR_ECDH_KEY_DUP        -21
#define ECP_ERR_ECDH_IDX            -22
#define ECP_ERR_ECDH_IDX_LOCAL      -23
#define ECP_ERR_ECDH_IDX_REMOTE     -24
#define ECP_ERR_RNG                 -25
#define ECP_ERR_ENCRYPT             -26
#define ECP_ERR_DECRYPT             -27
#define ECP_ERR_SEND                -28
#define ECP_ERR_RECV                -29
#define ECP_ERR_SEQ                 -30
#define ECP_ERR_CLOSED              -31
#define ECP_ERR_HANDLE              -32
#define ECP_ERR_NOT_IMPLEMENTED     -99

#define ECP_SIZE_PROTO              2
#define ECP_SIZE_SEQ                4

#define ECP_MAX_SOCK_CONN           16
#define ECP_MAX_SOCK_KEY            8
#define ECP_MAX_CONN_KEY            2
#define ECP_MAX_NODE_KEY            2
#define ECP_MAX_CTYPE               8
#define ECP_MAX_MTYPE               16
#define ECP_MAX_MTYPE_SYS           8

#define ECP_SIZE_PKT_HDR            (ECP_SIZE_PROTO+1+ECP_ECDH_SIZE_KEY+ECP_AEAD_SIZE_NONCE)
#define ECP_SIZE_PLD_HDR            (ECP_SIZE_SEQ)
#define ECP_SIZE_MSG_HDR            (ECP_SIZE_PLD_HDR+1)

#define ECP_MAX_PKT                 1412
#define ECP_MAX_PLD                 (ECP_MAX_PKT-ECP_SIZE_PKT_HDR-ECP_AEAD_SIZE_TAG)
#define ECP_MAX_MSG                 (ECP_MAX_PLD-ECP_SIZE_MSG_HDR)

#define ECP_MIN_PKT                 (ECP_SIZE_PKT_HDR+ECP_SIZE_MSG_HDR+ECP_AEAD_SIZE_TAG)

#define ECP_POLL_TIMEOUT            500
#define ECP_ECDH_IDX_INV            0xFF
#define ECP_ECDH_IDX_PERMA          0x0F

#define ECP_MTYPE_MASK              0x3f
#define ECP_MTYPE_FLAG_TIMER        0x80
#define ECP_MTYPE_FLAG_REP          0x40

#define ECP_MTYPE_OPEN              0x00
#define ECP_MTYPE_KGET              0x01
#define ECP_MTYPE_KPUT              0x02

#define ECP_MTYPE_OPEN_REQ          (ECP_MTYPE_OPEN)
#define ECP_MTYPE_OPEN_REP          (ECP_MTYPE_OPEN | ECP_MTYPE_FLAG_REP)
#define ECP_MTYPE_KGET_REQ          (ECP_MTYPE_KGET)
#define ECP_MTYPE_KGET_REP          (ECP_MTYPE_KGET | ECP_MTYPE_FLAG_REP)
#define ECP_MTYPE_KPUT_REQ          (ECP_MTYPE_KPUT)
#define ECP_MTYPE_KPUT_REP          (ECP_MTYPE_KPUT | ECP_MTYPE_FLAG_REP)

#define ECP_SIZE_PLD(X)             ((X) + ECP_SIZE_MSG_HDR)
#define ECP_SIZE_PKT(X)             ((X) + ECP_SIZE_PKT_HDR+ECP_SIZE_MSG_HDR+ECP_AEAD_SIZE_TAG)

#define ECP_CONN_FLAG_REG           0x01
#define ECP_CONN_FLAG_OPEN          0x02

#define ecp_conn_is_reg(conn)       ((conn->flags) & ECP_CONN_FLAG_REG)
#define ecp_conn_is_open(conn)      ((conn->flags) & ECP_CONN_FLAG_OPEN)

#include "config.h"

#include <stddef.h>
#include <stdint.h>

typedef long ssize_t;
typedef uint32_t ecp_seq_t;

#ifdef ECP_WITH_PTHREAD
#include <pthread.h>
#endif

#include "posix/transport.h"
#include "crypto/crypto.h"
#include "timer.h"

#ifdef ECP_WITH_RBUF
#ifdef ECP_WITH_MSGQ
#include "msgq.h"
#endif
#include "rbuf.h"
#endif

#ifdef ECP_DEBUG
#include <stdio.h>
#define DPRINT(cnd, format, ...)    { if (cnd) { fprintf (stderr, format, __VA_ARGS__); } }
#else
#define DPRINT(cnd, format, ...)    {}
#endif

struct ECPContext;
struct ECPSocket;
struct ECPConnection;

typedef int ecp_rng_t (void *, size_t);

typedef int ecp_conn_handler_new_t (struct ECPSocket *s, struct ECPConnection **c, struct ECPConnection *p, unsigned char s_idx, unsigned char c_idx, unsigned char *pub, ecp_aead_key_t *sh, unsigned char *msg, size_t sz);
typedef ssize_t ecp_conn_handler_msg_t (struct ECPConnection *c, ecp_seq_t s, unsigned char t, unsigned char *msg, ssize_t sz);

typedef struct ECPConnection * ecp_conn_alloc_t (unsigned char t);
typedef void ecp_conn_free_t (struct ECPConnection *c);
typedef int ecp_conn_create_t (struct ECPConnection *c, unsigned char *msg, size_t sz);
typedef void ecp_conn_destroy_t (struct ECPConnection *c);
typedef ssize_t ecp_conn_open_t (struct ECPConnection *c);
typedef void ecp_conn_close_t (struct ECPConnection *c);

typedef struct ECPCryptoIface {
    int init;
    int (*dh_mkpair) (ecp_dh_public_t *p, ecp_dh_private_t *s, ecp_rng_t *rb);
    int (*dh_shsec) (ecp_aead_key_t *sh, ecp_dh_public_t *p, ecp_dh_private_t *s);
    unsigned char *(*dh_pub_get_buf) (ecp_dh_public_t *p);
    void (*dh_pub_to_buf) (unsigned char *b, ecp_dh_public_t *p);
    void (*dh_pub_from_buf) (ecp_dh_public_t *p, unsigned char *b);
    int (*dh_pub_eq) (unsigned char *p1, ecp_dh_public_t *p2);
    unsigned int (*dh_pub_hash_fn) (unsigned char *p);
    int (*dh_pub_hash_eq) (unsigned char *p1, unsigned char *p2);
    
    ssize_t (*aead_enc) (unsigned char *ct, size_t cl, unsigned char *pt, size_t pl, ecp_aead_key_t *k, unsigned char *n);
    ssize_t (*aead_dec) (unsigned char *pt, size_t pl, unsigned char *ct, size_t cl, ecp_aead_key_t *k, unsigned char *n);
    int (*dsa_mkpair) (ecp_dsa_public_t *p, ecp_dsa_private_t *s, ecp_rng_t *rb);
    int (*dsa_sign) (unsigned char *sig, unsigned char *m, size_t ml, ecp_dsa_public_t *p, ecp_dsa_private_t *s);
    int (*dsa_verify) (unsigned char *m, size_t ml, unsigned char *sig, ecp_dsa_public_t *p);
} ECPCryptoIface;

typedef struct ECPTransportIface {
    int init;
    int (*open) (ECPNetSock *, void *addr);
    void (*close) (ECPNetSock *);
    int (*poll) (ECPNetSock *, int);
    ssize_t (*send) (ECPNetSock *, void *, size_t, ECPNetAddr *);
    ssize_t (*recv) (ECPNetSock *, void *, size_t, ECPNetAddr *);
    int (*addr_eq) (ECPNetAddr *, ECPNetAddr *);
    int (*addr_set) (ECPNetAddr *, void *addr);
} ECPTransportIface;

typedef struct ECPTimeIface {
    int init;
    unsigned int (*abstime_ms) (unsigned int);
    void (*sleep_ms) (unsigned int);
} ECPTimeIface;

#ifdef ECP_WITH_HTABLE
typedef struct ECPHTableIface {
    int init;
    void *(*create) (struct ECPContext *c);
    void (*destroy) (void *t);
    int (*insert) (void *t, unsigned char *k, struct ECPConnection *v);
    struct ECPConnection *(*remove) (void *t, unsigned char *k);
    struct ECPConnection *(*search) (void *t, unsigned char *k);
} ECPHTableIface;
#endif

typedef struct ECPDHKey {
    ecp_dh_public_t public;
    ecp_dh_private_t private;
    unsigned char valid;
} ECPDHKey;

typedef struct ECPDHRKey {
    unsigned char idx;
    ecp_dh_public_t public;
} ECPDHRKey;

typedef struct ECPDHShared {
    ecp_aead_key_t secret;
    unsigned char valid;
} ECPDHShared;

typedef struct ECPDHRKeyBucket {
    ECPDHRKey key[ECP_MAX_NODE_KEY];
    unsigned char key_curr;
    unsigned char key_idx_map[ECP_MAX_SOCK_KEY];
} ECPDHRKeyBucket;

typedef struct ECPNode {
    ECPNetAddr addr;
    ecp_dh_public_t public;
} ECPNode;

typedef struct ECPConnHandler {
    ecp_conn_handler_msg_t *msg[ECP_MAX_MTYPE];
    ecp_conn_create_t *conn_create;
    ecp_conn_destroy_t *conn_destroy;
    ecp_conn_open_t *conn_open;
    ecp_conn_close_t *conn_close;
} ECPConnHandler;

typedef struct ECPSockCTable {
    struct ECPConnection *array[ECP_MAX_SOCK_CONN];
    unsigned short size;
#ifdef ECP_WITH_HTABLE
    void *htable;
#endif
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_t mutex;
#endif
} ECPSockCTable;

typedef struct ECPContext {
    ecp_rng_t *rng;
    ecp_conn_alloc_t *conn_alloc;
    ecp_conn_free_t *conn_free;
    ECPCryptoIface cr;
    ECPTransportIface tr;
    ECPTimeIface tm;
#ifdef ECP_WITH_HTABLE
    ECPHTableIface ht;
#endif
    ssize_t (*pack) (struct ECPConnection *conn, unsigned char *packet, size_t pkt_size, unsigned char s_idx, unsigned char c_idx, unsigned char *payload, size_t payload_size, ECPNetAddr *addr, ecp_seq_t *seq, int *rbuf_idx);
    ssize_t (*pack_raw) (struct ECPSocket *sock, struct ECPConnection *parent, unsigned char *packet, size_t pkt_size, unsigned char s_idx, unsigned char c_idx, ecp_dh_public_t *public, ecp_aead_key_t *shsec, unsigned char *nonce, ecp_seq_t seq, unsigned char *payload, size_t payload_size, ECPNetAddr *addr);
    ECPConnHandler *handler[ECP_MAX_CTYPE];
} ECPContext;

typedef struct ECPSocket {
    ECPContext *ctx;
    unsigned char running;
    unsigned int poll_timeout;
    ECPNetSock sock;
    ECPDHKey key_perma;
    ECPDHKey key[ECP_MAX_SOCK_KEY];
    unsigned char key_curr;
    ECPSockCTable conn;
    ECPTimer timer;
    ecp_conn_handler_new_t *conn_new;
#ifdef ECP_WITH_PTHREAD
    pthread_t rcvr_thd;
    pthread_mutex_t mutex;
#endif
} ECPSocket;

typedef struct ECPConnection {
    unsigned char type;
    unsigned char out;
    unsigned char flags;
    unsigned short refcount;
    ecp_seq_t seq_out;
    ecp_seq_t seq_in;
    uint32_t seq_in_bitmap;
    ECPSocket *sock;
    ECPNode node;
    ECPDHRKeyBucket remote;
    ECPDHKey key[ECP_MAX_CONN_KEY];
    unsigned char key_curr;
    unsigned char key_idx[ECP_MAX_NODE_KEY];
    unsigned char key_idx_curr;
    unsigned char key_idx_map[ECP_MAX_SOCK_KEY];
    ECPDHShared shared[ECP_MAX_NODE_KEY][ECP_MAX_NODE_KEY];
    unsigned char nonce[ECP_AEAD_SIZE_NONCE];
#ifdef ECP_WITH_RBUF
    ECPConnRBuffer rbuf;
#endif
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_t mutex;
#endif
    struct ECPConnection *parent;
    void *conn_data;
} ECPConnection;

int ecp_init(ECPContext *ctx);
int ecp_crypto_init(ECPCryptoIface *t);
int ecp_transport_init(ECPTransportIface *t);
int ecp_time_init(ECPTimeIface *t);
#ifdef ECP_WITH_HTABLE
int ecp_htable_init(ECPHTableIface *h);
#endif

int ecp_dhkey_generate(ECPContext *ctx, ECPDHKey *key);
int ecp_node_init(ECPContext *ctx, ECPNode *node, ecp_dh_public_t *public, void *addr);

int ecp_ctx_create(ECPContext *ctx);
int ecp_ctx_destroy(ECPContext *ctx);

int ecp_sock_create(ECPSocket *sock, ECPContext *ctx, ECPDHKey *key);
void ecp_sock_destroy(ECPSocket *sock);
int ecp_sock_open(ECPSocket *sock, void *myaddr);
void ecp_sock_close(ECPSocket *sock);
int ecp_sock_dhkey_get_curr(ECPSocket *sock, unsigned char *idx, unsigned char *public);
int ecp_sock_dhkey_new(ECPSocket *sock);

int ecp_conn_create(ECPConnection *conn, ECPSocket *sock, unsigned char ctype);
void ecp_conn_destroy(ECPConnection *conn);
int ecp_conn_register(ECPConnection *conn);
void ecp_conn_unregister(ECPConnection *conn);

int ecp_conn_init(ECPConnection *conn, ECPNode *node);
int ecp_conn_open(ECPConnection *conn, ECPNode *node);
int ecp_conn_close(ECPConnection *conn, unsigned int timeout);

int ecp_conn_handler_init(ECPConnHandler *handler);
ssize_t ecp_conn_send_open(ECPConnection *conn);
int ecp_conn_handle_new(ECPSocket *sock, ECPConnection **_conn, ECPConnection *parent, unsigned char s_idx, unsigned char c_idx, unsigned char *c_public, ecp_aead_key_t *shsec, unsigned char *payload, size_t payload_size);
ssize_t ecp_conn_handle_open(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size);
ssize_t ecp_conn_handle_kget(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size);
ssize_t ecp_conn_handle_kput(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size);
ssize_t ecp_conn_handle_exec(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size);

int ecp_conn_dhkey_new(ECPConnection *conn);
int ecp_conn_dhkey_new_pub(ECPConnection *conn, unsigned char idx, unsigned char *public);
int ecp_conn_dhkey_get_curr(ECPConnection *conn, unsigned char *idx, unsigned char *public);

ssize_t ecp_pack(ECPContext *ctx, unsigned char *packet, size_t pkt_size, unsigned char s_idx, unsigned char c_idx, ecp_dh_public_t *public, ecp_aead_key_t *shsec, unsigned char *nonce, ecp_seq_t seq, unsigned char *payload, size_t payload_size);
ssize_t ecp_pack_raw(ECPSocket *sock, ECPConnection *parent, unsigned char *packet, size_t pkt_size, unsigned char s_idx, unsigned char c_idx, ecp_dh_public_t *public, ecp_aead_key_t *shsec, unsigned char *nonce, ecp_seq_t seq, unsigned char *payload, size_t payload_size, ECPNetAddr *addr);
ssize_t ecp_conn_pack(ECPConnection *conn, unsigned char *packet, size_t pkt_size, unsigned char s_idx, unsigned char c_idx, unsigned char *payload, size_t payload_size, ECPNetAddr *addr, ecp_seq_t *seq, int *rbuf_idx);

ssize_t ecp_pkt_handle(ECPSocket *sock, ECPNetAddr *addr, ECPConnection *parent, unsigned char *packet, size_t pkt_size);
ssize_t ecp_pkt_send(ECPSocket *sock, ECPNetAddr *addr, unsigned char *packet, size_t pkt_size);
ssize_t ecp_pkt_recv(ECPSocket *sock, ECPNetAddr *addr, unsigned char *packet, size_t pkt_size);

ssize_t ecp_msg_handle(ECPConnection *conn, ecp_seq_t seq, unsigned char *msg, size_t msg_size);
unsigned char *ecp_pld_get_buf(unsigned char *payload);
void ecp_pld_set_type(unsigned char *payload, unsigned char mtype);
unsigned char ecp_pld_get_type(unsigned char *payload);
ssize_t ecp_pld_send(ECPConnection *conn, unsigned char *payload, size_t payload_size);
ssize_t ecp_pld_send_wkey(ECPConnection *conn, unsigned char s_idx, unsigned char c_idx, unsigned char *payload, size_t payload_size);
ssize_t ecp_pld_send_raw(ECPSocket *sock, ECPConnection *parent, ECPNetAddr *addr, unsigned char s_idx, unsigned char c_idx, ecp_dh_public_t *public, ecp_aead_key_t *shsec, unsigned char *nonce, ecp_seq_t seq, unsigned char *payload, size_t payload_size);

ssize_t ecp_send(ECPConnection *conn, unsigned char *payload, size_t payload_size);
ssize_t ecp_receive(ECPConnection *conn, unsigned char mtype, unsigned char *msg, size_t msg_size, unsigned int timeout);

int ecp_receiver(ECPSocket *sock);
int ecp_start_receiver(ECPSocket *sock);
int ecp_stop_receiver(ECPSocket *sock);
