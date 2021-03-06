/*
 * Internal prototypes and structures for the kadmind password update plugin.
 *
 * Written by Russ Allbery <eagle@eyrie.org>
 * Based on code developed by Derrick Brashear and Ken Hornstein of Sine
 *     Nomine Associates, on behalf of Stanford University.
 * Copyright 2006, 2007, 2010, 2013
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifndef PLUGIN_INTERNAL_H
#define PLUGIN_INTERNAL_H 1

#include <config.h>
#include <portable/krb5.h>
#include <portable/macros.h>
#include <portable/stdbool.h>

#ifdef HAVE_KRB5_KADM5_HOOK_PLUGIN
# include <krb5/kadm5_hook_plugin.h>
#else
typedef struct kadm5_hook_modinfo_st kadm5_hook_modinfo;
#endif

/* Used to store a list of strings, managed by the sync_vector_* functions. */
struct vector {
    size_t count;
    size_t allocated;
    char **strings;
};

/*
 * Local configuration information for the module.  This contains all the
 * parameters that are read from the krb5-sync sub-section of the appdefaults
 * section when the module is initialized.
 *
 * MIT Kerberos uses this type as an abstract data type for any data that a
 * kadmin hook needs to carry.  Reuse it since then we get type checking for
 * at least the MIT plugin.
 */
struct kadm5_hook_modinfo_st {
    char *ad_admin_server;
    char *ad_base_instance;
    struct vector *ad_instances;
    char *ad_keytab;
    char *ad_ldap_base;
    char *ad_principal;
    bool ad_queue_only;
    char *ad_realm;
    char *queue_dir;
    bool syslog;
};

BEGIN_DECLS

/* Default to a hidden visibility for all internal functions. */
#pragma GCC visibility push(hidden)

/* Initialize the plugin and set up configuration. */
krb5_error_code sync_init(krb5_context, kadm5_hook_modinfo **);

/* Free the internal plugin state. */
void sync_close(krb5_context, kadm5_hook_modinfo *);

/* Handle a password change. */
krb5_error_code sync_chpass(kadm5_hook_modinfo *, krb5_context,
                            krb5_principal, const char *password);

/* Handle an account status change. */
krb5_error_code sync_status(kadm5_hook_modinfo *, krb5_context,
                            krb5_principal, bool enabled);

/* Password changing in Active Directory. */
krb5_error_code sync_ad_chpass(kadm5_hook_modinfo *, krb5_context,
                               krb5_principal, const char *password);

/* Account status update in Active Directory. */
krb5_error_code sync_ad_status(kadm5_hook_modinfo *, krb5_context,
                               krb5_principal, bool enabled);

/*
 * Sets exists true to true if the principal has only one component and
 * two-component principal with instance added exists in the Kerberos
 * database, false otherwise.  Returns an error if we cannot determine whether
 * the principal exists.
 */
krb5_error_code sync_instance_exists(krb5_context, krb5_principal,
                                     const char *instance, bool *exists);

/* Returns true if there is a queue conflict for this operation. */
krb5_error_code sync_queue_conflict(kadm5_hook_modinfo *, krb5_context,
                                    krb5_principal, const char *operation,
                                    bool *conflict);

/* Writes an operation to the queue. */
krb5_error_code sync_queue_write(kadm5_hook_modinfo *, krb5_context,
                                 krb5_principal, const char *operation,
                                 const char *password);

/*
 * Manage vectors, which are counted lists of strings.  The functions that
 * return a boolean return false if memory allocation fails.
 */
struct vector *sync_vector_new(void)
    __attribute__((__malloc__));
bool sync_vector_add(struct vector *, const char *string)
    __attribute__((__nonnull__));
void sync_vector_free(struct vector *);

/*
 * vector_split_multi splits on a set of characters.  If the vector argument
 * is NULL, a new vector is allocated; otherwise, the provided one is reused.
 * Returns NULL on memory allocation failure, after which the provided vector
 * may have been modified to only have partial results.
 *
 * Empty strings will yield zero-length vectors.  Adjacent delimiters are
 * treated as a single delimiter by vector_split_multi.  Any leading or
 * trailing delimiters are ignored, so this function will never create
 * zero-length strings (similar to the behavior of strtok).
 */
struct vector *sync_vector_split_multi(const char *string, const char *seps,
                                       struct vector *)
    __attribute__((__nonnull__(1, 2)));

/*
 * Obtain configuration settings from krb5.conf.  These are wrappers around
 * the krb5_appdefault_* APIs that handle setting the section name, obtaining
 * the local default realm and using it to find settings, and doing any
 * necessary conversion.
 */
void sync_config_boolean(krb5_context, const char *, bool *)
    __attribute__((__nonnull__));
krb5_error_code sync_config_list(krb5_context, const char *, struct vector **)
    __attribute__((__nonnull__));
void sync_config_string(krb5_context, const char *, char **)
    __attribute__((__nonnull__));

/*
 * Store a configuration, generic, or system error in the Kerberos context,
 * appending the strerror results to the message in the _system case and the
 * LDAP error string in the _ldap case.  Returns the error code set.
 */
krb5_error_code sync_error_config(krb5_context, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));
krb5_error_code sync_error_generic(krb5_context, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));
krb5_error_code sync_error_ldap(krb5_context, int, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 3, 4)));
krb5_error_code sync_error_system(krb5_context, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));

/* Log messages to syslog if configured to do so. */
void sync_syslog_debug(kadm5_hook_modinfo *, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));
void sync_syslog_info(kadm5_hook_modinfo *, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));
void sync_syslog_notice(kadm5_hook_modinfo *, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));
void sync_syslog_warning(kadm5_hook_modinfo *, const char *format, ...)
    __attribute__((__nonnull__, __format__(printf, 2, 3)));

/* Undo default visibility change. */
#pragma GCC visibility pop

END_DECLS

#endif /* !PLUGIN_INTERNAL_H */
