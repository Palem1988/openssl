/*
 * Copyright 1995-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "internal/cryptlib_int.h"
#include "internal/err.h"
#include "internal/err_int.h"
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/opensslconf.h>
#include "internal/thread_once.h"
#include "internal/ctype.h"
#include "internal/constant_time_locl.h"
#include "e_os.h"
#include "err_locl.h"

static int err_load_strings(const ERR_STRING_DATA *str);

static void ERR_STATE_free(ERR_STATE *s);
#ifndef OPENSSL_NO_ERR
static ERR_STRING_DATA ERR_str_libraries[] = {
    {ERR_PACK(ERR_LIB_NONE, 0, 0), "unknown library"},
    {ERR_PACK(ERR_LIB_SYS, 0, 0), "system library"},
    {ERR_PACK(ERR_LIB_BN, 0, 0), "bignum routines"},
    {ERR_PACK(ERR_LIB_RSA, 0, 0), "rsa routines"},
    {ERR_PACK(ERR_LIB_DH, 0, 0), "Diffie-Hellman routines"},
    {ERR_PACK(ERR_LIB_EVP, 0, 0), "digital envelope routines"},
    {ERR_PACK(ERR_LIB_BUF, 0, 0), "memory buffer routines"},
    {ERR_PACK(ERR_LIB_OBJ, 0, 0), "object identifier routines"},
    {ERR_PACK(ERR_LIB_PEM, 0, 0), "PEM routines"},
    {ERR_PACK(ERR_LIB_DSA, 0, 0), "dsa routines"},
    {ERR_PACK(ERR_LIB_X509, 0, 0), "x509 certificate routines"},
    {ERR_PACK(ERR_LIB_ASN1, 0, 0), "asn1 encoding routines"},
    {ERR_PACK(ERR_LIB_CONF, 0, 0), "configuration file routines"},
    {ERR_PACK(ERR_LIB_CRYPTO, 0, 0), "common libcrypto routines"},
    {ERR_PACK(ERR_LIB_EC, 0, 0), "elliptic curve routines"},
    {ERR_PACK(ERR_LIB_ECDSA, 0, 0), "ECDSA routines"},
    {ERR_PACK(ERR_LIB_ECDH, 0, 0), "ECDH routines"},
    {ERR_PACK(ERR_LIB_SSL, 0, 0), "SSL routines"},
    {ERR_PACK(ERR_LIB_BIO, 0, 0), "BIO routines"},
    {ERR_PACK(ERR_LIB_PKCS7, 0, 0), "PKCS7 routines"},
    {ERR_PACK(ERR_LIB_X509V3, 0, 0), "X509 V3 routines"},
    {ERR_PACK(ERR_LIB_PKCS12, 0, 0), "PKCS12 routines"},
    {ERR_PACK(ERR_LIB_RAND, 0, 0), "random number generator"},
    {ERR_PACK(ERR_LIB_DSO, 0, 0), "DSO support routines"},
    {ERR_PACK(ERR_LIB_TS, 0, 0), "time stamp routines"},
    {ERR_PACK(ERR_LIB_ENGINE, 0, 0), "engine routines"},
    {ERR_PACK(ERR_LIB_OCSP, 0, 0), "OCSP routines"},
    {ERR_PACK(ERR_LIB_UI, 0, 0), "UI routines"},
    {ERR_PACK(ERR_LIB_FIPS, 0, 0), "FIPS routines"},
    {ERR_PACK(ERR_LIB_CMS, 0, 0), "CMS routines"},
    {ERR_PACK(ERR_LIB_CRMF, 0, 0), "CRMF routines"},
    {ERR_PACK(ERR_LIB_CMP, 0, 0), "CMP routines"},
    {ERR_PACK(ERR_LIB_HMAC, 0, 0), "HMAC routines"},
    {ERR_PACK(ERR_LIB_CT, 0, 0), "CT routines"},
    {ERR_PACK(ERR_LIB_ASYNC, 0, 0), "ASYNC routines"},
    {ERR_PACK(ERR_LIB_OSSL_STORE, 0, 0), "STORE routines"},
    {ERR_PACK(ERR_LIB_SM2, 0, 0), "SM2 routines"},
    {ERR_PACK(ERR_LIB_ESS, 0, 0), "ESS routines"},
    {ERR_PACK(ERR_LIB_PROV, 0, 0), "Provider routines"},
    {0, NULL},
};

static ERR_STRING_DATA ERR_str_reasons[] = {
    {ERR_R_SYS_LIB, "system lib"},
    {ERR_R_BN_LIB, "BN lib"},
    {ERR_R_RSA_LIB, "RSA lib"},
    {ERR_R_DH_LIB, "DH lib"},
    {ERR_R_EVP_LIB, "EVP lib"},
    {ERR_R_BUF_LIB, "BUF lib"},
    {ERR_R_OBJ_LIB, "OBJ lib"},
    {ERR_R_PEM_LIB, "PEM lib"},
    {ERR_R_DSA_LIB, "DSA lib"},
    {ERR_R_X509_LIB, "X509 lib"},
    {ERR_R_ASN1_LIB, "ASN1 lib"},
    {ERR_R_EC_LIB, "EC lib"},
    {ERR_R_BIO_LIB, "BIO lib"},
    {ERR_R_PKCS7_LIB, "PKCS7 lib"},
    {ERR_R_X509V3_LIB, "X509V3 lib"},
    {ERR_R_ENGINE_LIB, "ENGINE lib"},
    {ERR_R_UI_LIB, "UI lib"},
    {ERR_R_OSSL_STORE_LIB, "STORE lib"},
    {ERR_R_ECDSA_LIB, "ECDSA lib"},

    {ERR_R_NESTED_ASN1_ERROR, "nested asn1 error"},
    {ERR_R_MISSING_ASN1_EOS, "missing asn1 eos"},

    {ERR_R_FATAL, "fatal"},
    {ERR_R_MALLOC_FAILURE, "malloc failure"},
    {ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED,
     "called a function you should not call"},
    {ERR_R_PASSED_NULL_PARAMETER, "passed a null parameter"},
    {ERR_R_INTERNAL_ERROR, "internal error"},
    {ERR_R_DISABLED, "called a function that was disabled at compile-time"},
    {ERR_R_INIT_FAIL, "init fail"},
    {ERR_R_OPERATION_FAIL, "operation fail"},

    {0, NULL},
};
#endif

static CRYPTO_ONCE err_init = CRYPTO_ONCE_STATIC_INIT;
static int set_err_thread_local;
static CRYPTO_THREAD_LOCAL err_thread_local;

static CRYPTO_ONCE err_string_init = CRYPTO_ONCE_STATIC_INIT;
static CRYPTO_RWLOCK *err_string_lock;

static ERR_STRING_DATA *int_err_get_item(const ERR_STRING_DATA *);

/*
 * The internal state
 */

static LHASH_OF(ERR_STRING_DATA) *int_error_hash = NULL;
static int int_err_library_number = ERR_LIB_USER;

static unsigned long get_error_values(int inc, int top, const char **file,
                                      int *line, const char **data,
                                      int *flags);

static unsigned long err_string_data_hash(const ERR_STRING_DATA *a)
{
    unsigned long ret, l;

    l = a->error;
    ret = l ^ ERR_GET_LIB(l);
    return (ret ^ ret % 19 * 13);
}

static int err_string_data_cmp(const ERR_STRING_DATA *a,
                               const ERR_STRING_DATA *b)
{
    if (a->error == b->error)
        return 0;
    return a->error > b->error ? 1 : -1;
}

static ERR_STRING_DATA *int_err_get_item(const ERR_STRING_DATA *d)
{
    ERR_STRING_DATA *p = NULL;

    CRYPTO_THREAD_read_lock(err_string_lock);
    p = lh_ERR_STRING_DATA_retrieve(int_error_hash, d);
    CRYPTO_THREAD_unlock(err_string_lock);

    return p;
}

#ifndef OPENSSL_NO_ERR
/* 2019-05-21: Russian and Ukrainian locales on Linux require more than 6,5 kB */
# define SPACE_SYS_STR_REASONS 8 * 1024
# define NUM_SYS_STR_REASONS 127

static ERR_STRING_DATA SYS_str_reasons[NUM_SYS_STR_REASONS + 1];
/*
 * SYS_str_reasons is filled with copies of strerror() results at
 * initialization. 'errno' values up to 127 should cover all usual errors,
 * others will be displayed numerically by ERR_error_string. It is crucial
 * that we have something for each reason code that occurs in
 * ERR_str_reasons, or bogus reason strings will be returned for SYSerr(),
 * which always gets an errno value and never one of those 'standard' reason
 * codes.
 */

static void build_SYS_str_reasons(void)
{
    /* OPENSSL_malloc cannot be used here, use static storage instead */
    static char strerror_pool[SPACE_SYS_STR_REASONS];
    char *cur = strerror_pool;
    size_t cnt = 0;
    static int init = 1;
    int i;
    int saveerrno = get_last_sys_error();

    CRYPTO_THREAD_write_lock(err_string_lock);
    if (!init) {
        CRYPTO_THREAD_unlock(err_string_lock);
        return;
    }

    for (i = 1; i <= NUM_SYS_STR_REASONS; i++) {
        ERR_STRING_DATA *str = &SYS_str_reasons[i - 1];

        str->error = ERR_PACK(ERR_LIB_SYS, 0, i);
        /*
         * If we have used up all the space in strerror_pool,
         * there's no point in calling openssl_strerror_r()
         */
        if (str->string == NULL && cnt < sizeof(strerror_pool)) {
            if (openssl_strerror_r(i, cur, sizeof(strerror_pool) - cnt)) {
                size_t l = strlen(cur);

                str->string = cur;
                cnt += l;
                cur += l;

                /*
                 * VMS has an unusual quirk of adding spaces at the end of
                 * some (most? all?) messages. Lets trim them off.
                 */
                while (cur > strerror_pool && ossl_isspace(cur[-1])) {
                    cur--;
                    cnt--;
                }
                *cur++ = '\0';
                cnt++;
            }
        }
        if (str->string == NULL)
            str->string = "unknown";
    }

    /*
     * Now we still have SYS_str_reasons[NUM_SYS_STR_REASONS] = {0, NULL}, as
     * required by ERR_load_strings.
     */

    init = 0;

    CRYPTO_THREAD_unlock(err_string_lock);
    /* openssl_strerror_r could change errno, but we want to preserve it */
    set_sys_error(saveerrno);
    err_load_strings(SYS_str_reasons);
}
#endif

static void ERR_STATE_free(ERR_STATE *s)
{
    int i;

    if (s == NULL)
        return;
    for (i = 0; i < ERR_NUM_ERRORS; i++) {
        err_clear_data(s, i, 1);
    }
    OPENSSL_free(s);
}

DEFINE_RUN_ONCE_STATIC(do_err_strings_init)
{
    if (!OPENSSL_init_crypto(0, NULL))
        return 0;
    err_string_lock = CRYPTO_THREAD_lock_new();
    if (err_string_lock == NULL)
        return 0;
    int_error_hash = lh_ERR_STRING_DATA_new(err_string_data_hash,
                                            err_string_data_cmp);
    if (int_error_hash == NULL) {
        CRYPTO_THREAD_lock_free(err_string_lock);
        err_string_lock = NULL;
        return 0;
    }
    return 1;
}

void err_cleanup(void)
{
    if (set_err_thread_local != 0)
        CRYPTO_THREAD_cleanup_local(&err_thread_local);
    CRYPTO_THREAD_lock_free(err_string_lock);
    err_string_lock = NULL;
    lh_ERR_STRING_DATA_free(int_error_hash);
    int_error_hash = NULL;
}

/*
 * Legacy; pack in the library.
 */
static void err_patch(int lib, ERR_STRING_DATA *str)
{
    unsigned long plib = ERR_PACK(lib, 0, 0);

    for (; str->error != 0; str++)
        str->error |= plib;
}

/*
 * Hash in |str| error strings. Assumes the URN_ONCE was done.
 */
static int err_load_strings(const ERR_STRING_DATA *str)
{
    CRYPTO_THREAD_write_lock(err_string_lock);
    for (; str->error; str++)
        (void)lh_ERR_STRING_DATA_insert(int_error_hash,
                                       (ERR_STRING_DATA *)str);
    CRYPTO_THREAD_unlock(err_string_lock);
    return 1;
}

int ERR_load_ERR_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    err_load_strings(ERR_str_libraries);
    err_load_strings(ERR_str_reasons);
    build_SYS_str_reasons();
#endif
    return 1;
}

int ERR_load_strings(int lib, ERR_STRING_DATA *str)
{
    if (ERR_load_ERR_strings() == 0)
        return 0;

    err_patch(lib, str);
    err_load_strings(str);
    return 1;
}

int ERR_load_strings_const(const ERR_STRING_DATA *str)
{
    if (ERR_load_ERR_strings() == 0)
        return 0;
    err_load_strings(str);
    return 1;
}

int ERR_unload_strings(int lib, ERR_STRING_DATA *str)
{
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    CRYPTO_THREAD_write_lock(err_string_lock);
    /*
     * We don't need to ERR_PACK the lib, since that was done (to
     * the table) when it was loaded.
     */
    for (; str->error; str++)
        (void)lh_ERR_STRING_DATA_delete(int_error_hash, str);
    CRYPTO_THREAD_unlock(err_string_lock);

    return 1;
}

void err_free_strings_int(void)
{
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return;
}

/********************************************************/

void ERR_clear_error(void)
{
    int i;
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return;

    for (i = 0; i < ERR_NUM_ERRORS; i++) {
        err_clear(es, i, 0);
    }
    es->top = es->bottom = 0;
}

unsigned long ERR_get_error(void)
{
    return get_error_values(1, 0, NULL, NULL, NULL, NULL);
}

unsigned long ERR_get_error_line(const char **file, int *line)
{
    return get_error_values(1, 0, file, line, NULL, NULL);
}

unsigned long ERR_get_error_line_data(const char **file, int *line,
                                      const char **data, int *flags)
{
    return get_error_values(1, 0, file, line, data, flags);
}

unsigned long ERR_peek_error(void)
{
    return get_error_values(0, 0, NULL, NULL, NULL, NULL);
}

unsigned long ERR_peek_error_line(const char **file, int *line)
{
    return get_error_values(0, 0, file, line, NULL, NULL);
}

unsigned long ERR_peek_error_line_data(const char **file, int *line,
                                       const char **data, int *flags)
{
    return get_error_values(0, 0, file, line, data, flags);
}

unsigned long ERR_peek_last_error(void)
{
    return get_error_values(0, 1, NULL, NULL, NULL, NULL);
}

unsigned long ERR_peek_last_error_line(const char **file, int *line)
{
    return get_error_values(0, 1, file, line, NULL, NULL);
}

unsigned long ERR_peek_last_error_line_data(const char **file, int *line,
                                            const char **data, int *flags)
{
    return get_error_values(0, 1, file, line, data, flags);
}

static unsigned long get_error_values(int inc, int top, const char **file,
                                      int *line, const char **data,
                                      int *flags)
{
    int i = 0;
    ERR_STATE *es;
    unsigned long ret;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    if (inc && top) {
        if (file)
            *file = "";
        if (line)
            *line = 0;
        if (data)
            *data = "";
        if (flags)
            *flags = 0;

        return ERR_R_INTERNAL_ERROR;
    }

    while (es->bottom != es->top) {
        if (es->err_flags[es->top] & ERR_FLAG_CLEAR) {
            err_clear(es, es->top, 0);
            es->top = es->top > 0 ? es->top - 1 : ERR_NUM_ERRORS - 1;
            continue;
        }
        i = (es->bottom + 1) % ERR_NUM_ERRORS;
        if (es->err_flags[i] & ERR_FLAG_CLEAR) {
            es->bottom = i;
            err_clear(es, es->bottom, 0);
            continue;
        }
        break;
    }

    if (es->bottom == es->top)
        return 0;

    if (top)
        i = es->top;            /* last error */
    else
        i = (es->bottom + 1) % ERR_NUM_ERRORS; /* first error */

    ret = es->err_buffer[i];
    if (inc) {
        es->bottom = i;
        es->err_buffer[i] = 0;
    }

    if (file != NULL && line != NULL) {
        if (es->err_file[i] == NULL) {
            *file = "NA";
            *line = 0;
        } else {
            *file = es->err_file[i];
            *line = es->err_line[i];
        }
    }

    if (data == NULL) {
        if (inc) {
            err_clear_data(es, i, 0);
        }
    } else {
        if (es->err_data[i] == NULL) {
            *data = "";
            if (flags != NULL)
                *flags = 0;
        } else {
            *data = es->err_data[i];
            if (flags != NULL)
                *flags = es->err_data_flags[i];
        }
    }
    return ret;
}

void ERR_error_string_n(unsigned long e, char *buf, size_t len)
{
    char lsbuf[64], rsbuf[64];
    const char *ls, *rs;
    unsigned long f = 0, l, r;

    if (len == 0)
        return;

    l = ERR_GET_LIB(e);
    ls = ERR_lib_error_string(e);
    if (ls == NULL) {
        BIO_snprintf(lsbuf, sizeof(lsbuf), "lib(%lu)", l);
        ls = lsbuf;
    }

    rs = ERR_reason_error_string(e);
    r = ERR_GET_REASON(e);
    if (rs == NULL) {
        BIO_snprintf(rsbuf, sizeof(rsbuf), "reason(%lu)", r);
        rs = rsbuf;
    }

    BIO_snprintf(buf, len, "error:%08lX:%s:%s:%s", e, ls, "", rs);
    if (strlen(buf) == len - 1) {
        /* Didn't fit; use a minimal format. */
        BIO_snprintf(buf, len, "err:%lx:%lx:%lx:%lx", e, l, f, r);
    }
}

/*
 * ERR_error_string_n should be used instead for ret != NULL as
 * ERR_error_string cannot know how large the buffer is
 */
char *ERR_error_string(unsigned long e, char *ret)
{
    static char buf[256];

    if (ret == NULL)
        ret = buf;
    ERR_error_string_n(e, ret, (int)sizeof(buf));
    return ret;
}

const char *ERR_lib_error_string(unsigned long e)
{
    ERR_STRING_DATA d, *p;
    unsigned long l;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init)) {
        return NULL;
    }

    l = ERR_GET_LIB(e);
    d.error = ERR_PACK(l, 0, 0);
    p = int_err_get_item(&d);
    return ((p == NULL) ? NULL : p->string);
}

const char *ERR_func_error_string(unsigned long e)
{
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return NULL;
    return ERR_GET_LIB(e) == ERR_LIB_SYS ? "system library" : NULL;
}

const char *ERR_reason_error_string(unsigned long e)
{
    ERR_STRING_DATA d, *p = NULL;
    unsigned long l, r;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init)) {
        return NULL;
    }

    l = ERR_GET_LIB(e);
    r = ERR_GET_REASON(e);
    d.error = ERR_PACK(l, 0, r);
    p = int_err_get_item(&d);
    if (!p) {
        d.error = ERR_PACK(0, 0, r);
        p = int_err_get_item(&d);
    }
    return ((p == NULL) ? NULL : p->string);
}

/* TODO(3.0): arg ignored for now */
static void err_delete_thread_state(void *arg)
{
    ERR_STATE *state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (state == NULL)
        return;

    CRYPTO_THREAD_set_local(&err_thread_local, NULL);
    ERR_STATE_free(state);
}

#if !OPENSSL_API_1_1_0
void ERR_remove_thread_state(void *dummy)
{
}
#endif

#if !OPENSSL_API_1_0_0
void ERR_remove_state(unsigned long pid)
{
}
#endif

DEFINE_RUN_ONCE_STATIC(err_do_init)
{
    set_err_thread_local = 1;
    return CRYPTO_THREAD_init_local(&err_thread_local, NULL);
}

ERR_STATE *ERR_get_state(void)
{
    ERR_STATE *state;
    int saveerrno = get_last_sys_error();

    if (!OPENSSL_init_crypto(OPENSSL_INIT_BASE_ONLY, NULL))
        return NULL;

    if (!RUN_ONCE(&err_init, err_do_init))
        return NULL;

    state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (state == (ERR_STATE*)-1)
        return NULL;

    if (state == NULL) {
        if (!CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)-1))
            return NULL;

        if ((state = OPENSSL_zalloc(sizeof(*state))) == NULL) {
            CRYPTO_THREAD_set_local(&err_thread_local, NULL);
            return NULL;
        }

        if (!ossl_init_thread_start(NULL, NULL, err_delete_thread_state)
                || !CRYPTO_THREAD_set_local(&err_thread_local, state)) {
            ERR_STATE_free(state);
            CRYPTO_THREAD_set_local(&err_thread_local, NULL);
            return NULL;
        }

        /* Ignore failures from these */
        OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    }

    set_sys_error(saveerrno);
    return state;
}

/*
 * err_shelve_state returns the current thread local error state
 * and freezes the error module until err_unshelve_state is called.
 */
int err_shelve_state(void **state)
{
    int saveerrno = get_last_sys_error();

    /*
     * Note, at present our only caller is OPENSSL_init_crypto(), indirectly
     * via ossl_init_load_crypto_nodelete(), by which point the requested
     * "base" initialization has already been performed, so the below call is a
     * NOOP, that re-enters OPENSSL_init_crypto() only to quickly return.
     *
     * If are no other valid callers of this function, the call below can be
     * removed, avoiding the re-entry into OPENSSL_init_crypto().  If there are
     * potential uses that are not from inside OPENSSL_init_crypto(), then this
     * call is needed, but some care is required to make sure that the re-entry
     * remains a NOOP.
     */
    if (!OPENSSL_init_crypto(OPENSSL_INIT_BASE_ONLY, NULL))
        return 0;

    if (!RUN_ONCE(&err_init, err_do_init))
        return 0;

    *state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (!CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)-1))
        return 0;

    set_sys_error(saveerrno);
    return 1;
}

/*
 * err_unshelve_state restores the error state that was returned
 * by err_shelve_state previously.
 */
void err_unshelve_state(void* state)
{
    if (state != (void*)-1)
        CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)state);
}

int ERR_get_next_error_library(void)
{
    int ret;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    CRYPTO_THREAD_write_lock(err_string_lock);
    ret = int_err_library_number++;
    CRYPTO_THREAD_unlock(err_string_lock);
    return ret;
}

static int err_set_error_data_int(char *data, size_t size, int flags,
                                  int deallocate)
{
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    err_clear_data(es, es->top, deallocate);
    err_set_data(es, es->top, data, size, flags);

    return 1;
}

void ERR_set_error_data(char *data, int flags)
{
    /*
     * This function is void so we cannot propagate the error return. Since it
     * is also in the public API we can't change the return type.
     *
     * We estimate the size of the data.  If it's not flagged as allocated,
     * then this is safe, and if it is flagged as allocated, then our size
     * may be smaller than the actual allocation, but that doesn't matter
     * too much, the buffer will remain untouched or will eventually be
     * reallocated to a new size.
     *
     * callers should be advised that this function takes over ownership of
     * the allocated memory, i.e. they can't count on the pointer to remain
     * valid.
     */
    err_set_error_data_int(data, strlen(data) + 1, flags, 1);
}

void ERR_add_error_data(int num, ...)
{
    va_list args;
    va_start(args, num);
    ERR_add_error_vdata(num, args);
    va_end(args);
}

void ERR_add_error_vdata(int num, va_list args)
{
    int i, len, size;
    int flags = ERR_TXT_MALLOCED | ERR_TXT_STRING;
    char *str, *arg;
    ERR_STATE *es;

    /* Get the current error data; if an allocated string get it. */
    es = ERR_get_state();
    if (es == NULL)
        return;
    i = es->top;

    /*
     * If err_data is allocated already, re-use the space.
     * Otherwise, allocate a small new buffer.
     */
    if ((es->err_data_flags[i] & flags) == flags) {
        str = es->err_data[i];
        size = es->err_data_size[i];

        /*
         * To protect the string we just grabbed from tampering by other
         * functions we may call, or to protect them from freeing a pointer
         * that may no longer be valid at that point, we clear away the
         * data pointer and the flags.  We will set them again at the end
         * of this function.
         */
        es->err_data[i] = NULL;
        es->err_data_flags[i] = 0;
    } else if ((str = OPENSSL_malloc(size = 81)) == NULL) {
        return;
    } else {
        str[0] = '\0';
    }
    len = strlen(str);

    while (--num >= 0) {
        arg = va_arg(args, char *);
        if (arg == NULL)
            arg = "<NULL>";
        len += strlen(arg);
        if (len >= size) {
            char *p;

            size = len + 20;
            p = OPENSSL_realloc(str, size);
            if (p == NULL) {
                OPENSSL_free(str);
                return;
            }
            str = p;
        }
        OPENSSL_strlcat(str, arg, (size_t)size);
    }
    if (!err_set_error_data_int(str, size, flags, 0))
        OPENSSL_free(str);
}

int ERR_set_mark(void)
{
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    if (es->bottom == es->top)
        return 0;
    es->err_flags[es->top] |= ERR_FLAG_MARK;
    return 1;
}

int ERR_pop_to_mark(void)
{
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    while (es->bottom != es->top
           && (es->err_flags[es->top] & ERR_FLAG_MARK) == 0) {
        err_clear(es, es->top, 0);
        es->top = es->top > 0 ? es->top - 1 : ERR_NUM_ERRORS - 1;
    }

    if (es->bottom == es->top)
        return 0;
    es->err_flags[es->top] &= ~ERR_FLAG_MARK;
    return 1;
}

int ERR_clear_last_mark(void)
{
    ERR_STATE *es;
    int top;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    top = es->top;
    while (es->bottom != top
           && (es->err_flags[top] & ERR_FLAG_MARK) == 0) {
        top = top > 0 ? top - 1 : ERR_NUM_ERRORS - 1;
    }

    if (es->bottom == top)
        return 0;
    es->err_flags[top] &= ~ERR_FLAG_MARK;
    return 1;
}

void err_clear_last_constant_time(int clear)
{
    ERR_STATE *es;
    int top;

    es = ERR_get_state();
    if (es == NULL)
        return;

    top = es->top;

    /*
     * Flag error as cleared but remove it elsewhere to avoid two errors
     * accessing the same error stack location, revealing timing information.
     */
    clear = constant_time_select_int(constant_time_eq_int(clear, 0),
                                     0, ERR_FLAG_CLEAR);
    es->err_flags[top] |= clear;
}
