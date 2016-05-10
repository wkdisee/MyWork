/* 
 version: 0.1
 written in 04/19
 using this version to test dso:
 loading the shared library of skf in windows.
 */

#include <stdio.h>
#include <string>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/dso.h>
#include <openssl/engine.h>

#ifndef OPENSSL_NO_RSA
# include <openssl/rsa.h>
#endif
#ifndef OPENSSL_NO_DSA
# include <openssl/dsa.h>
#endif
#ifndef OPENSSL_NO_DH
# include <openssl/dh.h>
#endif
#include <openssl/bn.h>

#ifndef OPENSSL_NO_HW
# ifndef OPENSSL_NO_HW_SKF

/*-
 * Attribution notice: nCipher have said several times that it's OK for
 * us to implement a general interface to their boxes, and recently declared
 * their HWCryptoHook to be public, and therefore available for us to use.
 * Thanks, nCipher.
 *
 * The hwcryptohook.h included here is from May 2000.
 * [Richard Levitte]
 */
/*
#  ifdef FLAT_INC
#   include "hwskf.h"  //???????????
#  else
#   include "vendor_defns/hwskf.h" //??????
#  endif
*/
#include <openssl/evp.h>
#include "Guomi/SKFAPI.h"

#  define HWSKF_LIB_NAME "SKF engine"

static int hwskf_destroy(ENGINE *e);
static int hwskf_init(ENGINE *e);
static int hwskf_finish(ENGINE *e);
static int hwskf_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f)(void));

/* KM stuff */
static EVP_PKEY *hwskf_load_privkey(ENGINE *eng, const char *key_id,
                                     UI_METHOD *ui_method,
                                     void *callback_data);
static EVP_PKEY *hwskf_load_pubkey(ENGINE *eng, const char *key_id,
                                    UI_METHOD *ui_method,
                                    void *callback_data);

/* The definitions for control commands specific to this engine */
#  define HWSKF_CMD_SO_PATH              ENGINE_CMD_BASE
static const ENGINE_CMD_DEFN hwskf_cmd_defns[] = {
    {HWSKF_CMD_SO_PATH,
     "SO_PATH",
     "Specifies the path to the 'hwskf' shared library",
     ENGINE_CMD_FLAG_STRING},
    {0, NULL, NULL, 0}
};

#  ifndef OPENSSL_NO_RSA
/* Our internal RSA_METHOD that we provide pointers to */
static RSA_METHOD hwskf_rsa = {
    "SKF RSA method",
    NULL,
    NULL,
    NULL,
    NULL,
    hwskf_rsa_mod_exp,
    hwskf_mod_exp_mont,
    NULL,
    hwskf_rsa_finish,
    0,
    NULL,
    NULL,
    NULL,
    NULL
};
#  endif

static RAND_METHOD hwskf_rand = {
    /* "SKF RAND method", */
    NULL,
    hwskf_rand_bytes,
    NULL,
    NULL,
    hwskf_rand_bytes,
    hwskf_rand_status,
};

/* Constants used when creating the ENGINE */
static const char *engine_hwskf_id = "skf";
static const char *engine_hwskf_name = "SKF hardware engine support";
#  ifndef OPENSSL_NO_DYNAMIC_ENGINE
/* Compatibility hack, the dynamic library uses this form in the path */
static const char *engine_hwskf_id_alt = "Guomi";
#  endif

static BIO *logstream = NULL;

/*
 * This internal function is used by ENGINE_skf() and possibly by the
 * "dynamic" ENGINE support too
 */
static int bind_helper(ENGINE *e)
{
#  ifndef OPENSSL_NO_RSA
    const RSA_METHOD *meth1;
#  endif
    if (!ENGINE_set_id(e, engine_hwskf_id) ||
        !ENGINE_set_name(e, engine_hwskf_name) ||
        
        !ENGINE_set_destroy_function(e, hwskf_destroy) ||
        !ENGINE_set_init_function(e, hwskf_init) ||
        !ENGINE_set_finish_function(e, hwskf_finish) ||
        !ENGINE_set_ctrl_function(e, hwskf_ctrl) ||
        !ENGINE_set_load_privkey_function(e, hwskf_load_privkey) ||
        !ENGINE_set_load_pubkey_function(e, hwskf_load_pubkey) ||
        !ENGINE_set_cmd_defns(e, hwskf_cmd_defns))
        return 0;

#  ifndef OPENSSL_NO_RSA
    /*
     * We know that the "PKCS1_SSLeay()" functions hook properly to the
     * cswift-specific mod_exp and mod_exp_crt so we use those functions. NB:
     * We don't use ENGINE_openssl() or anything "more generic" because
     * something like the RSAref code may not hook properly, and if you own
     * one of these cards then you have the right to do RSA operations on it
     * anyway!
     */
     /*
    meth1 = RSA_PKCS1_SSLeay();
    hwskf_rsa.rsa_pub_enc = meth1->rsa_pub_enc;
    hwskf_rsa.rsa_pub_dec = meth1->rsa_pub_dec;
    hwskf_rsa.rsa_priv_enc = meth1->rsa_priv_enc;
    hwskf_rsa.rsa_priv_dec = meth1->rsa_priv_dec;
    */
#  endif

    /* Ensure the hwcrhk error handling is set up */
    //ERR_load_HWSKF_strings();
    return 1;
}

#  ifdef OPENSSL_NO_DYNAMIC_ENGINE
static ENGINE *engine_skf(void)
{
    ENGINE *ret = ENGINE_new();
    if (!ret)
        return NULL;
    if (!bind_helper(ret)) {
        ENGINE_free(ret);
        return NULL;
    }
    return ret;
}

void ENGINE_load_skf(void)
{
    /* Copied from eng_[openssl|dyn].c */
    ENGINE *toadd = engine_skf();
    if (!toadd)
        return;
    ENGINE_add(toadd);
    ENGINE_free(toadd);
    ERR_clear_error();
}
#  endif

/*
 * This is a process-global DSO handle used for loading and unloading the
 * HWCryptoHook library. NB: This is only set (or unset) during an init() or
 * finish() call (reference counts permitting) and they're operating with
 * global locks, so this should be thread-safe implicitly.
 */
static DSO *hwskf_dso = NULL;

/*
 * These are the function pointers that are (un)set when the library has
 * successfully (un)loaded.
 */

/* Used in the DSO operations. */
static const char *HWSKF_LIBNAME = NULL;
static void free_HWSKF_LIBNAME(void)
{
    if (HWSKF_LIBNAME)
        OPENSSL_free((void *)HWSKF_LIBNAME);
    HWSKF_LIBNAME = NULL;
}
static const char *get_HWSKF_LIBNAME(void)
{
    if (HWSKF_LIBNAME)
        return HWSKF_LIBNAME;
    return "ShuttleCsp11_3000GM";
}

static long set_HWSKF_LIBNAME(const char *name)
{
    free_HWSKF_LIBNAME();
    return (((HWSKF_LIBNAME = BUF_strdup(name)) != NULL) ? 1 : 0);

/* Destructor (complements the "ENGINE_chil()" constructor) */
static int hwskf_destroy(ENGINE *e)
{
    free_HWSKF_LIBNAME();
    //ERR_unload_HWSKF_strings();
    return 1;
}
static int hwskf_init(ENGINE *e){
    if (hwskf_dso != NULL) {
        //HWCRHKerr(HWCRHK_F_HWCRHK_INIT, HWCRHK_R_ALREADY_LOADED);
        goto err;
    }
    /* Attempt to load ShuttleCsp11_3000GM.dll/whatever. */
    hwskf_dso = DSO_load(NULL, get_HWSKF_LIBNAME(), NULL, 0);
    if (hwskf_dso == NULL) {
        //HWCRHKerr(HWCRHK_F_HWCRHK_INIT, HWCRHK_R_DSO_FAILURE);
        goto err;
    }

 err:
    if (hwskf_dso)
        DSO_free(hwskf_dso);
    hwskf_dso = NULL;
    return 0;
}

static int hwskf_finish(ENGINE *e)
{
    int to_return = 1;
    free_HWSKF_LIBNAME();
    if (hwskf_dso == NULL) {
        //HWCRHKerr(HWCRHK_F_HWCRHK_FINISH, HWCRHK_R_NOT_LOADED);
        to_return = 0;
        goto err;
    }

    if (!DSO_free(hwskf_dso)) {
        //HWCRHKerr(HWCRHK_F_HWCRHK_FINISH, HWCRHK_R_DSO_FAILURE);
        to_return = 0;
        goto err;
    }
 err:
    if (logstream)
        BIO_free(logstream);
    hwskf_dso = NULL;
    
    return to_return;
}

static int hwskf_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f) (void))
{
    int to_return = 1;

    switch (cmd) {
    case HWSKF_CMD_SO_PATH:
        if (hwskf_dso) {
            //HWCRHKerr(HWCRHK_F_HWCRHK_CTRL, HWCRHK_R_ALREADY_LOADED);
            return 0;
        }
        if (p == NULL) {
            //HWCRHKerr(HWCRHK_F_HWCRHK_CTRL, ERR_R_PASSED_NULL_PARAMETER);
            return 0;
        }
        return set_HWSKF_LIBNAME((const char *)p);
    case ENGINE_CTRL_SET_LOGSTREAM:
        {
            BIO *bio = (BIO *)p;

            CRYPTO_w_lock(CRYPTO_LOCK_ENGINE);
            if (logstream) {
                BIO_free(logstream);
                logstream = NULL;
            }
            if (CRYPTO_add(&bio->references, 1, CRYPTO_LOCK_BIO) > 1)
                logstream = bio;
            //else
                //HWCRHKerr(HWCRHK_F_HWCRHK_CTRL, HWCRHK_R_BIO_WAS_FREED);
        }
        CRYPTO_w_unlock(CRYPTO_LOCK_ENGINE);
        break;

        /* The command isn't understood by this engine */
    default:
        //HWCRHKerr(HWCRHK_F_HWCRHK_CTRL,
        //          HWCRHK_R_CTRL_COMMAND_NOT_IMPLEMENTED);
        to_return = 0;
        break;
    }

    return to_return;
}

static EVP_PKEY *hwskf_load_privkey(ENGINE *eng, const char *key_id,
                                     UI_METHOD *ui_method,
                                     void *callback_data)
{
#  ifndef OPENSSL_NO_RSA
    RSA *rtmp = NULL;
#  endif
    EVP_PKEY *res = NULL;

err:
#  ifndef OPENSSL_NO_RSA
    if (rtmp)
        RSA_free(rtmp);
#  endif
    return NULL;
}

static EVP_PKEY *hwskf_load_pubkey(ENGINE *eng, const char *key_id,
                                    UI_METHOD *ui_method, void *callback_data)
{
    EVP_PKEY *res = NULL;

    return res;
 err:
    if (res)
        EVP_PKEY_free(res);
    return NULL;
}


/*
 * This stuff is needed if this ENGINE is being compiled into a
 * self-contained shared-library.
 */
#  ifndef OPENSSL_NO_DYNAMIC_ENGINE
static int bind_fn(ENGINE *e, const char *id)
{
    if (id && (strcmp(id, engine_hwskf_id) != 0) &&
        (strcmp(id, engine_hwskf_id_alt) != 0))
        return 0;
    if (!bind_helper(e))
        return 0;
    return 1;
}
IMPLEMENT_DYNAMIC_CHECK_FN()
IMPLEMENT_DYNAMIC_BIND_FN(bind_fn)
#  endif    /* OPENSSL_NO_DYNAMIC_ENGINE */
# endif     /* !OPENSSL_NO_HW_CHIL */
#endif      /* !OPENSSL_NO_HW */


