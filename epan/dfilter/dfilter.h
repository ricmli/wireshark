/** @file
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DFILTER_H
#define DFILTER_H

#include <glib.h>
#include "ws_symbol_export.h"

/* Passed back to user */
typedef struct epan_dfilter dfilter_t;

#include <epan/proto.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct epan_dissect;

/* Module-level initialization */
void
dfilter_init(void);

/* Module-level cleanup */
void
dfilter_cleanup(void);

/* Compiles a string to a dfilter_t.
 * On success, sets the dfilter* pointed to by dfp
 * to either a NULL pointer (if the filter is a null
 * filter, as generated by an all-blank string) or to
 * a pointer to the newly-allocated dfilter_t
 * structure.
 *
 * On failure, *err_msg is set to point to the error
 * message.  This error message is allocated with
 * g_malloc(), and must be freed with g_free().
 * The dfilter* will be set to NULL after a failure.
 *
 * Returns TRUE on success, FALSE on failure.
 */
WS_DLL_PUBLIC
gboolean
dfilter_compile_real(const gchar *text, dfilter_t **dfp,
			gchar **err_msg, const char *caller);

#define dfilter_compile(text, dfp, err_msg) \
	dfilter_compile_real(text, dfp, err_msg, __func__)

/* Frees all memory used by dfilter, and frees
 * the dfilter itself. */
WS_DLL_PUBLIC
void
dfilter_free(dfilter_t *df);

/* Apply compiled dfilter */
WS_DLL_PUBLIC
gboolean
dfilter_apply_edt(dfilter_t *df, struct epan_dissect *edt);

/* Apply compiled dfilter */
gboolean
dfilter_apply(dfilter_t *df, proto_tree *tree);

/* Prime a proto_tree using the fields/protocols used in a dfilter. */
void
dfilter_prime_proto_tree(const dfilter_t *df, proto_tree *tree);

/* Refresh references in a compiled display filter. */
WS_DLL_PUBLIC
void
dfilter_load_field_references(const dfilter_t *df, proto_tree *tree);

/* Check if dfilter has interesting fields */
gboolean
dfilter_has_interesting_fields(const dfilter_t *df);

WS_DLL_PUBLIC
GPtrArray *
dfilter_deprecated_tokens(dfilter_t *df);

/* Print bytecode of dfilter to stdout */
WS_DLL_PUBLIC
void
dfilter_dump(dfilter_t *df);

/* Print bytecode of dfilter to log */
WS_DLL_PUBLIC
void
dfilter_log_full(const char *domain, enum ws_log_level level,
			const char *file, long line, const char *func,
			dfilter_t *dfcode, const char *msg);

#ifndef WS_DISABLE_DEBUG
#define dfilter_log(dfcode, msg) \
	dfilter_log_full(LOG_DOMAIN_DFILTER, LOG_LEVEL_NOISY,	\
				__FILE__, __LINE__, __func__,	\
				dfcode, msg)
#else
#define dfilter_log(dfcode, msg) (void)0
#endif

#define DFILTER_DEBUG_HERE(dfcode) \
	dfilter_log_full(LOG_DOMAIN_DFILTER, LOG_LEVEL_ECHO,	\
				__FILE__, __LINE__, __func__,	\
				dfcode, #dfcode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DFILTER_H */
