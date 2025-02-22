/*
 * Copyright 2011 - 2025
 * Andr\xe9 Malo or his licensors, as applicable
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cext.h"
EXT_INIT_FUNC;

typedef unsigned char rchar;
#ifdef U
#undef U
#endif
#define U(c) ((rchar)(c))

typedef struct {
    const rchar *start;
    const rchar *sentinel;
    const rchar *tsentinel;
    Py_ssize_t at_group;
    int in_macie5;
    int in_rule;
    int keep_bang_comments;
} rcssmin_ctx_t;

typedef enum {
    NEED_SPACE_MAYBE = 0,
    NEED_SPACE_NEVER
} need_space_flag;


#define RCSSMIN_DULL_BIT         (1 << 0)
#define RCSSMIN_HEX_BIT          (1 << 1)
#define RCSSMIN_ESC_BIT          (1 << 2)
#define RCSSMIN_SPACE_BIT        (1 << 3)
#define RCSSMIN_STRING_DULL_BIT  (1 << 4)
#define RCSSMIN_NMCHAR_BIT       (1 << 5)
#define RCSSMIN_URI_DULL_BIT     (1 << 6)
#define RCSSMIN_PRE_CHAR_BIT     (1 << 7)
#define RCSSMIN_POST_CHAR_BIT    (1 << 8)

static const unsigned short rcssmin_charmask[128] = {
     21,  21,  21,  21,  21,  21,  21,  21,
     21,  28,   8,  21,   8,   8,  21,  21,
     21,  21,  21,  21,  21,  21,  21,  21,
     21,  21,  21,  21,  21,  21,  21,  21,
     28, 469,   4,  85,  85,  85,  84,   4,
    149, 277,  85,  84, 469, 117,  85,  84,
    115, 115, 115, 115, 115, 115, 115, 115,
    115, 115, 468, 340,  85, 469, 468,  85,
     84, 115, 115, 115, 115, 115, 115, 117,
    117, 117, 117, 117, 117, 117, 117, 117,
    117, 117, 117, 117, 117, 117, 117, 117,
    117, 117, 117, 213,   4, 341,  85, 117,
     85, 115, 115, 115, 115, 115, 115, 117,
    117, 117, 117, 117, 117, 117, 117, 117,
    117, 117, 117, 117, 117, 116, 117, 117,
    117, 117, 117, 468,  85, 468,  85,  21
};

#define RCSSMIN_IS_DULL(c) ((U(c) > 127) || \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_DULL_BIT))

#define RCSSMIN_IS_HEX(c) ((U(c) <= 127) && \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_HEX_BIT))

#define RCSSMIN_IS_ESC(c) ((U(c) > 127) || \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_ESC_BIT))

#define RCSSMIN_IS_SPACE(c) ((U(c) <= 127) && \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_SPACE_BIT))

#define RCSSMIN_IS_STRING_DULL(c) ((U(c) > 127) || \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_STRING_DULL_BIT))

#define RCSSMIN_IS_NMCHAR(c) ((U(c) > 127) || \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_NMCHAR_BIT))

#define RCSSMIN_IS_URI_DULL(c) ((U(c) > 127) || \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_URI_DULL_BIT))

#define RCSSMIN_IS_PRE_CHAR(c) ((U(c) <= 127) && \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_PRE_CHAR_BIT))

#define RCSSMIN_IS_POST_CHAR(c) ((U(c) <= 127) && \
    (rcssmin_charmask[U(c) & 0x7F] & RCSSMIN_POST_CHAR_BIT))


static const rchar pattern_url[] = {
    /*U('u'),*/ U('r'), U('l'), U('(')
};

static const rchar pattern_ie7[] = {
    /*U('>'),*/ U('/'), U('*'), U('*'), U('/')
};

static const rchar pattern_media[] = {
    U('m'), U('e'), U('d'), U('i'), U('a'),
    U('M'), U('E'), U('D'), U('I'), U('A')
};

static const rchar pattern_document[] = {
    U('d'), U('o'), U('c'), U('u'), U('m'), U('e'), U('n'), U('t'),
    U('D'), U('O'), U('C'), U('U'), U('M'), U('E'), U('N'), U('T')
};

static const rchar pattern_supports[] = {
    U('s'), U('u'), U('p'), U('p'), U('o'), U('r'), U('t'), U('s'),
    U('S'), U('U'), U('P'), U('P'), U('O'), U('R'), U('T'), U('S')
};

static const rchar pattern_keyframes[] = {
    U('k'), U('e'), U('y'), U('f'), U('r'), U('a'), U('m'), U('e'), U('s'),
    U('K'), U('E'), U('Y'), U('F'), U('R'), U('A'), U('M'), U('E'), U('S')
};

static const rchar pattern_vendor_o[] = {
    U('-'), U('o'), U('-'),
    U('-'), U('O'), U('-')
};

static const rchar pattern_vendor_moz[] = {
    U('-'), U('m'), U('o'), U('z'), U('-'),
    U('-'), U('M'), U('O'), U('Z'), U('-')
};

static const rchar pattern_vendor_webkit[] = {
    U('-'), U('w'), U('e'), U('b'), U('k'), U('i'), U('t'), U('-'),
    U('-'), U('W'), U('E'), U('B'), U('K'), U('I'), U('T'), U('-')
};

static const rchar pattern_vendor_ms[] = {
    U('-'), U('m'), U('s'), U('-'),
    U('-'), U('M'), U('S'), U('-')
};

static const rchar pattern_first[] = {
    U('f'), U('i'), U('r'), U('s'), U('t'), U('-'), U('l'),
    U('F'), U('I'), U('R'), U('S'), U('T'), U('-'), U('L')
};

static const rchar pattern_line[] = {
    U('i'), U('n'), U('e'),
    U('I'), U('N'), U('E'),
};

static const rchar pattern_letter[] = {
    U('e'), U('t'), U('t'), U('e'), U('r'),
    U('E'), U('T'), U('T'), U('E'), U('R')
};

static const rchar pattern_data[] = {
    U('d'), U('a'), U('t'), U('a'), U(':'),
    U('D'), U('A'), U('T'), U('A'), U(':')
};

static const rchar pattern_macie5_init[] = {
    U('/'), U('*'), U('\\'), U('*'), U('/')
};

static const rchar pattern_macie5_exit[] = {
    U('/'), U('*'), U('*'), U('/')
};

/*
 * Match a pattern (and copy immediately to target)
 *
 * Returns: 1 if the pattern was fully matched and copied, 0 otherwise.
 */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
static int
copy_match(const rchar *pattern, const rchar *psentinel,
           const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;
    rchar c;

    while (pattern < psentinel
           && source < ctx->sentinel && target < ctx->tsentinel
           && ((c = *source++) == *pattern++))
        *target++ = c;

    *source_ = source;
    *target_ = target;

    return (pattern == psentinel);
}
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

#define MATCH(PAT, source, target, ctx) (                              \
    copy_match(pattern_##PAT,                                          \
               pattern_##PAT + sizeof(pattern_##PAT) / sizeof(rchar),  \
               source, target, ctx)                                    \
)


/*
 * Match a pattern (and copy immediately to target) - CI version
 *
 * Returns: 1 if the pattern was fully matched and copied, 0 otherwise.
 */
static int
copy_imatch(const rchar *pattern, const rchar *psentinel,
            const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_, *pstart = pattern;
    rchar *target = *target_;
    rchar c;

    while (pattern < psentinel
           && source < ctx->sentinel && target < ctx->tsentinel
           && ((c = *source++) == *pattern
               || c == pstart[(pattern - pstart) + (psentinel - pstart)])) {
        ++pattern;
        *target++ = c;
    }

    *source_ = source;
    *target_ = target;

    return (pattern == psentinel);
}

#define IMATCH(PAT, source, target, ctx) (                                  \
    copy_imatch(pattern_##PAT,                                              \
                pattern_##PAT + sizeof(pattern_##PAT) / sizeof(rchar) / 2,  \
                source, target, ctx)                                        \
)


/*
 * Copy characters
 *
 * Returns: 1 if the source was fully copied, 0 otherwise.
 */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
static int
copy(const rchar *source, const rchar *sentinel, rchar **target_,
     rcssmin_ctx_t *ctx)
{
    rchar *target = *target_;

    while (source < sentinel && target < ctx->tsentinel)
        *target++ = *source++;

    *target_ = target;

    return (source == sentinel);
}
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

#define COPY_PAT(PAT, target, ctx) (                             \
    copy(pattern_##PAT,                                          \
         pattern_##PAT + sizeof(pattern_##PAT) / sizeof(rchar),  \
         target, ctx)                                            \
)


/*
 * The ABORT macros work with known local variables!
 */
#define ABORT_(RET) do {                                         \
    if (source < ctx->sentinel && !(target < ctx->tsentinel)) {  \
        *source_ = source;                                       \
        *target_ = target;                                       \
    }                                                            \
    return RET;                                                  \
} while(0)


#define CRAPPY_C90_COMPATIBLE_EMPTY
#define ABORT ABORT_(CRAPPY_C90_COMPATIBLE_EMPTY)
#define RABORT(RET) ABORT_((RET))


/*
 * Copy escape
 *
 * source: after the initial \\ char
 */
static void
copy_escape(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_, *hsentinel;
    rchar *target = *target_;
    rchar c;

    *target++ = U('\\');
    *target_ = target;

    if (source < ctx->sentinel && target < ctx->tsentinel) {
        c = *source++;
        if (RCSSMIN_IS_ESC(c)) {
            *target++ = c;
        }
        else if (RCSSMIN_IS_HEX(c)) {
            *target++ = c;

            /* 6 hex chars max, one we got already */
            if (ctx->sentinel - source > 5)
                hsentinel = source + 5;
            else
                hsentinel = ctx->sentinel;

            while (source < hsentinel && target < ctx->tsentinel
                   && (c = *source, RCSSMIN_IS_HEX(c))) {
                ++source;
                *target++ = c;
            }

            /* One optional space after */
            if (source < ctx->sentinel && target < ctx->tsentinel) {
                if (source == hsentinel)
                    c = *source;
                if (RCSSMIN_IS_SPACE(c)) {
                    ++source;
                    *target++ = U(' ');
                    if (c == U('\r') && source < ctx->sentinel
                        && *source == U('\n'))
                        ++source;
                }
            }
        }
    }

    *target_ = target;
    *source_ = source;
}


/*
 * Copy string
 *
 * source: after the initial quote (either " or ')
 */
static void
copy_string(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;
    rchar c, quote = source[-1];

    *target++ = quote;
    *target_ = target;

    while (source < ctx->sentinel && target < ctx->tsentinel) {
        c = *target++ = *source++;
        if (RCSSMIN_IS_STRING_DULL(c))
            continue;

        switch (c) {
        case U('\''): case U('"'):
            if (c == quote) {
                *target_ = target;
                *source_ = source;
                return;
            }
            continue;

        case U('\\'):
            if (source < ctx->sentinel && target < ctx->tsentinel) {
                c = *source++;
                switch (c) {
                case U('\r'):
                    if (source < ctx->sentinel && *source == U('\n'))
                        ++source;
                    /* fall through */

                case U('\n'): case U('\f'):
                    --target;
                    break;

                default:
                    *target++ = c;
                }
            }
            continue;
        }
        break; /* forbidden characters */
    }

    ABORT;
}


/*
 * Copy quoted URI string
 *
 * source: after the initial quote (either " or ')
 *
 * whitespaces inside ``url()`` definitions are stripped, except if it's a
 * quoted non-base64 data url (e.g. SVG)
 *
 * Returns: 0 if the string was fully matched and copied, -1 and rollback
 *          otherwise.
 *
 */
static int
copy_uri_string(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;
    int is_spacy_data = 0;
    rchar c, quote = source[-1];

    *target++ = quote;
    *target_ = target;

    if (!IMATCH(data, &source, &target, ctx)) {
        --source;
    }
    else {
        while (source < ctx->sentinel && target < ctx->tsentinel) {
            c = *source++;
            if (c <= U(' ') || c == U('\\') || c == U('"') || c == U('\'')) {
                /* no match */
                --source;
                break;
            }
            *target++ = c;

            if (c == U(',')) {
                /* no need to check for boundaries below. The initial imatch
                 * will block us from crossing them */
                if (!(   target[-2] == U('4')
                      && target[-3] == U('6')
                      && (target[-4] == U('e') || target[-4] == U('E'))
                      && (target[-5] == U('s') || target[-5] == U('S'))
                      && (target[-6] == U('a') || target[-6] == U('A'))
                      && (target[-7] == U('b') || target[-7] == U('B'))
                      && target[-8] == U(';')))
                    is_spacy_data = 1;
                break;
            }
        }
    }

    while (source < ctx->sentinel && target < ctx->tsentinel) {
        c = *source++;
        if (RCSSMIN_IS_SPACE(c) && !is_spacy_data)
            continue;
        *target++ = c;
        if (RCSSMIN_IS_STRING_DULL(c))
            continue;

        switch (c) {
        case U('\''): case U('"'):
            if (c == quote) {
                *target_ = target;
                *source_ = source;
                return 0;
            }
            continue;

        case U('\\'):
            if (source < ctx->sentinel && target < ctx->tsentinel) {
                c = *source;
                switch (c) {
                case U('\r'):
                    if ((source + 1) < ctx->sentinel && source[1] == U('\n'))
                        ++source;
                    /* fall through */

                case U('\n'): case U('\f'):
                    --target;
                    ++source;
                    break;

                default:
                    --target;
                    copy_escape(&source, &target, ctx);
                }
            }
            continue;
        }

        break; /* forbidden characters */
    }

    RABORT(-1);
}


/*
 * Copy URI (unquoted)
 *
 * source: after the initial "url(" pattern
 *
 * Returns: 0 if the string was fully matched and copied, -1 and rollback
 *          otherwise.
 */
static int
copy_uri_unquoted(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;
    rchar c;

    *target++ = source[-1];
    *target_ = target;

    while (source < ctx->sentinel && target < ctx->tsentinel) {
        c = *source++;
        if (RCSSMIN_IS_SPACE(c))
            continue;
        *target++ = c;
        if (RCSSMIN_IS_URI_DULL(c))
            continue;

        switch (c) {

        case U(')'):
            *target_ = target - 1;
            *source_ = source - 1;
            return 0;

        case U('\\'):
            if (source < ctx->sentinel && target < ctx->tsentinel) {
                c = *source;
                switch (c) {
                case U('\r'):
                    if ((source + 1) < ctx->sentinel && source[1] == U('\n'))
                        ++source;
                    /* fall through */

                case U('\n'): case U('\f'):
                    --target;
                    ++source;
                    break;

                default:
                    --target;
                    copy_escape(&source, &target, ctx);
                }
            }
            continue;
        }

        break; /* forbidden characters */
    }

    RABORT(-1);
}


/*
 * Copy url(...)
 *
 * source: after the initial "u"
 */
static void
copy_url(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;
    rchar c;

    *target++ = U('u');
    *target_ = target;

    /* Must not be inside an identifier */
    if ((source != ctx->start + 1) && RCSSMIN_IS_NMCHAR(source[-2]))
        return;

    if (!MATCH(url, &source, &target, ctx)
        || !(source < ctx->sentinel && target < ctx->tsentinel))
        ABORT;

    while (source < ctx->sentinel && RCSSMIN_IS_SPACE(*source))
        ++source;

    if (!(source < ctx->sentinel))
        ABORT;

    c = *source++;
    switch (c) {
    case U('"'): case U('\''):
        if (copy_uri_string(&source, &target, ctx) == -1)
            ABORT;

        while (source < ctx->sentinel && RCSSMIN_IS_SPACE(*source))
            ++source;
        break;

    default:
        if (copy_uri_unquoted(&source, &target, ctx) == -1)
            ABORT;
    }

    if (!(source < ctx->sentinel && target < ctx->tsentinel))
        ABORT;

    if ((*target++ = *source++) != U(')'))
        ABORT;

    *target_ = target;
    *source_ = source;
}


/*
 * Copy @-group
 *
 * source: after the initial "@"
 */
static void
copy_at_group(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;

    *target++ = U('@');
    *target_ = target;

/* Reset to beginning before matching */
#define REMATCH(what) ( \
    source = *source_, \
    target = *target_, \
    IMATCH(what, &source, &target, ctx) \
)
/* Shortcut, simply copy-match */
#define CMATCH(what) IMATCH(what, &source, &target, ctx)

    if ((  !CMATCH(media)
        && !REMATCH(supports)
        && !REMATCH(document)
        && !REMATCH(keyframes)
        && !(REMATCH(vendor_webkit) && CMATCH(keyframes))
        && !(REMATCH(vendor_moz) && CMATCH(keyframes))
        && !(REMATCH(vendor_o) && CMATCH(keyframes))
        && !(REMATCH(vendor_ms) && CMATCH(keyframes)))
        || !(source < ctx->sentinel && target < ctx->tsentinel)
        || RCSSMIN_IS_NMCHAR(*source))
        ABORT;

#undef CMATCH
#undef REMATCH

    ++ctx->at_group;

    *target_ = target;
    *source_ = source;
}


/*
 * Skip space (and any comment) in source. Source is not modified.
 * IOW: find the position after current space and comment run.
 *
 * source: start scanning
 *
 * Returns: 1 if spaces where found, 0 otherwise.
 *          `end` will be pointer after the space-comment (if any, source
 *          otherwise).
 */
static int
skip_space(const rchar *source, rcssmin_ctx_t *ctx, const rchar **end)
{
    const rchar *begin = source;
    int res, found_space = 0;
    rchar c;

    *end = begin;

    while (source < ctx->sentinel) {
        c = *source;
        if (RCSSMIN_IS_SPACE(c)) {
            found_space = 1;
            ++source;
            continue;
        }
        else if (c == U('/')) {
            ++source;
            if (!(source < ctx->sentinel && *source == U('*'))) {
                --source;
                break;
            }
            ++source;
            res = 0;
            while (source < ctx->sentinel) {
                c = *source++;
                if (c != U('*'))
                    continue;
                if (!(source < ctx->sentinel))
                    return found_space;
                if (*source != U('/'))
                    continue;

                /* Comment complete */
                ++source;
                res = 1;
                break;
            }
            if (!res)
                return found_space;

            continue;
        }

        break;
    }

    *end = source;
    return found_space;
}


/*
 * Copy space + comments to target:
 *   - if need_space is NEED_SPACE_MAYBE, a single space might appear in the
 *     target, depending on what comes before and after
 *   - bang comments are copied if this is requested
 *
 * source: *after* the first scanned character (WS or /)
 */
static void
copy_space(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx,
           need_space_flag need_space)
{
    const rchar *source = *source_, *end, *comment;
    rchar *target = *target_;
    int res;
    rchar c;

    --source;
    if (need_space == NEED_SPACE_MAYBE
        && source > ctx->start
        && !RCSSMIN_IS_PRE_CHAR(source[-1])
        && (skip_space(source, ctx, &end), end) < ctx->sentinel
        && (!RCSSMIN_IS_POST_CHAR(*end)
            || (*end == U(':') && !ctx->in_rule && !ctx->at_group))) {

        if (!(target < ctx->tsentinel))
            ABORT;
        *target++ = U(' ');
    }

    while (source < ctx->sentinel) {
        switch (c = *source) {

        /* comment */
        case U('/'):
            comment = source++;
            if (!((source < ctx->sentinel && *source == U('*')))) {
                --source;
                break;
            }
            ++source;
            res = 0;
            while (source < ctx->sentinel) {
                c = *source++;
                if (c != U('*'))
                    continue;
                if (!(source < ctx->sentinel))
                    ABORT;
                if (*source != U('/'))
                    continue;

                /* Comment complete */
                ++source;
                res = 1;

                if (ctx->keep_bang_comments && comment[2] == U('!')) {
                    ctx->in_macie5 = (source[-3] == U('\\'));
                    if (!copy(comment, source, &target, ctx))
                        ABORT;
                }
                else if (source[-3] == U('\\')) {
                    if (!ctx->in_macie5) {
                        if (!COPY_PAT(macie5_init, &target, ctx))
                            ABORT;
                    }
                    ctx->in_macie5 = 1;
                }
                else if (ctx->in_macie5) {
                    if (!COPY_PAT(macie5_exit, &target, ctx))
                        ABORT;
                    ctx->in_macie5 = 0;
                }
                /* else don't copy anything */
                break;
            }
            if (!res)
                ABORT;
            continue;

        /* space */
        case U(' '): case U('\t'): case U('\r'): case U('\n'): case U('\f'):
            ++source;
            continue;
        }

        break;
    }

    *source_ = source;
    *target_ = target;
}


/*
 * Copy space if it's comment start ("/" followed by "*")
 *   - if need_space is NEED_SPACE_MAYBE, a single space might appear in the
 *     target, depending on what comes before and after
 *   - bang comments are copied if this is requested
 *
 * The initial slash is copied either way.
 *
 * source: after the initial "/"
 *
 * Returns: 0 if it was indeed a comment, -1 otherwise.
 */
static int
copy_space_comment(const rchar **source_, rchar **target_,
                   rcssmin_ctx_t *ctx, need_space_flag need_space)
{
    const rchar *source = *source_;
    rchar *target = *target_;

    if (source < ctx->sentinel && *source == U('*')) {
        copy_space(source_, target_, ctx, need_space);
        if (*source_ > source)
            return 0;
    }
    if (!(target < ctx->tsentinel))
        RABORT(-1);

    *target++ = source[-1];

    /* *source_ = source; <-- unchanged */
    *target_ = target;

    return -1;
}


/*
 * Copy space-comment if exists (only bang comments will be copied if
 * requested)
 *
 * source: start scanning
 *
 * Returns: 0 if it was a valid space or comment, -1 otherwise.
 */
static int
copy_space_optional(const rchar **source_, rchar **target_,
                    rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;

    if (!(source < ctx->sentinel))
        return -1;

    if (*source == U('/')) {
        *source_ = source + 1;
        return copy_space_comment(source_, target_, ctx, NEED_SPACE_NEVER);
    }
    else if (RCSSMIN_IS_SPACE(*source)) {
        *source_ = source + 1;
        copy_space(source_, target_, ctx, NEED_SPACE_NEVER);
        return 0;
    }

    return -1;
}


/*
 * Copy :first-line|letter. If the following character (after comments and
 * spaces) is a "{" or ",", a space is inserted to the target.
 *
 * source: after the initial ":"
 */
static void
copy_first(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_, *next, *source_fork;
    rchar *target = *target_, *target_fork;

    *target++ = U(':');
    *target_ = target;

    if (!IMATCH(first, &source, &target, ctx)
        || !(source < ctx->sentinel && target < ctx->tsentinel))
        ABORT;

    source_fork = source;
    target_fork = target;

    if (!IMATCH(line, &source, &target, ctx)) {
        source = source_fork;
        target = target_fork;

        if (!IMATCH(letter, &source, &target, ctx)
            || !(source < ctx->sentinel && target < ctx->tsentinel))
            ABORT;
    }

    skip_space(source, ctx, &next);
    if (!(next < ctx->sentinel && target < ctx->tsentinel
        && (*next == U('{') || *next == U(','))))
        ABORT;

    *target++ = U(' ');
    *target_ = target;
    *source_ = source;
    (void)copy_space_optional(source_, target_, ctx);
}


/*
 * Copy & (nesting selector). Make sure to keep a space if the following
 * space-comment contains one.
 *
 * source: after the initial "&"
 */
static void
copy_nesting(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_, *next;
    rchar *target = *target_;

    *target++ = U('&');
    *target_ = target;

    if (!(source < ctx->sentinel && target < ctx->tsentinel))
        ABORT;

    if (skip_space(source, ctx, &next) && next < ctx->sentinel) {
        *target++ = U(' ');
        *target_ = target;
    }

    (void)copy_space_optional(source_, target_, ctx);
}


/*
 * Copy IE7 hack (">" followed by an empty comment)
 *
 * source: after the initial ">"
 */
static void
copy_ie7hack(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_;
    rchar *target = *target_;

    *target++ = U('>');
    *target_ = target;

    if (ctx->in_rule || ctx->at_group)
        return; /* abort */

    if (!MATCH(ie7, &source, &target, ctx))
        ABORT;

    ctx->in_macie5 = 0;

    *target_ = target;
    *source_ = source;

    (void)copy_space_optional(source_, target_, ctx);
}


/*
 * Copy semicolon; miss out duplicates or even this one (before '}')
 *
 * source: after the semicolon
 */
static void
copy_semicolon(const rchar **source_, rchar **target_, rcssmin_ctx_t *ctx)
{
    const rchar *source = *source_, *begin, *end;
    rchar *target = *target_;

    begin = source;
    while (source < ctx->sentinel) {
        skip_space(source, ctx, &end);
        if (!(end < ctx->sentinel)) {
            if (!(target < ctx->tsentinel))
                ABORT;
            *target++ = U(';');
            break;
        }
        switch (*end) {
        case U(';'):
            source = end + 1;
            continue;

        case U('}'):
            if (ctx->in_rule)
                break;

            /* fall through */
        default:
            if (!(target < ctx->tsentinel))
                ABORT;
            *target++ = U(';');
            break;
        }

        break;
    }

    source = begin;
    *target_ = target;
    while (source < ctx->sentinel) {
        if (*source == U(';')) {
            ++source;
            continue;
        }

        if (copy_space_optional(&source, target_, ctx) == 0)
            continue;

        break;
    }

    *source_ = source;
}


/*
 * Main function
 *
 * The return value determines the result length (kept in the target buffer).
 * However, if the target buffer is too small, the return value is greater
 * than tlength. The difference to tlength is the number of unconsumed source
 * characters at the time the buffer was full. In this case you should resize
 * the target buffer to the return value and call rcssmin again. Repeat as
 * often as needed.
 */
static Py_ssize_t
rcssmin(const rchar *source, rchar *target, Py_ssize_t slength,
        Py_ssize_t tlength, int keep_bang_comments)
{
    rcssmin_ctx_t ctx_, *ctx = &ctx_;
    const rchar *tstart = target;
    rchar c;

    ctx->start = source;
    ctx->sentinel = source + slength;
    ctx->tsentinel = target + tlength;
    ctx->at_group = 0;
    ctx->in_macie5 = 0;
    ctx->in_rule = 0;
    ctx->keep_bang_comments = keep_bang_comments;

    while (source < ctx->sentinel && target < ctx->tsentinel) {
        c = *source++;
        if (RCSSMIN_IS_DULL(c)) {
            *target++ = c;
            continue;
        }
        else if (RCSSMIN_IS_SPACE(c)) {
            copy_space(&source, &target, ctx, NEED_SPACE_MAYBE);
            continue;
        }

        switch (c) {

        /* Escape */
        case U('\\'):
            copy_escape(&source, &target, ctx);
            continue;

        /* String */
        case U('"'): case U('\''):
            copy_string(&source, &target, ctx);
            continue;

        /* URL */
        case U('u'):
            copy_url(&source, &target, ctx);
            continue;

        /* Nesting selector */
        case U('&'):
            copy_nesting(&source, &target, ctx);
            continue;

        /* IE7hack */
        case U('>'):
            copy_ie7hack(&source, &target, ctx);
            continue;

        /* @-group */
        case U('@'):
            copy_at_group(&source, &target, ctx);
            continue;

        /* ; */
        case U(';'):
            copy_semicolon(&source, &target, ctx);
            continue;

        /* :first-line|letter followed by [{,] */
        /* (apparently needed for IE6) */
        case U(':'):
            copy_first(&source, &target, ctx);
            continue;

        /* { */
        case U('{'):
            if (ctx->at_group)
                --ctx->at_group;
            else
                ++ctx->in_rule;
            *target++ = c;
            continue;

        /* } */
        case U('}'):
            if (ctx->in_rule)
                --ctx->in_rule;
            *target++ = c;
            continue;

        /* space starting with comment */
        case U('/'):
            (void)copy_space_comment(&source, &target, ctx, NEED_SPACE_MAYBE);
            continue;

        /* Fallback: copy character. Better safe than sorry. Should not be
         * reached, though */
        default:
            *target++ = c;
            continue;
        }
    }

    return
        (Py_ssize_t)(target - tstart) + (Py_ssize_t)(ctx->sentinel - source);
}


PyDoc_STRVAR(rcssmin_cssmin__doc__,
"cssmin(style, keep_bang_comments=False)\n\
\n\
Minify CSS.\n\
\n\
:Note: This is a hand crafted C implementation built on the regex\n\
       semantics.\n\
\n\
Parameters:\n\
  style (str):\n\
    CSS to minify\n\
\n\
  keep_bang_comments (bool):\n\
    Keep comments starting with an exclamation mark? (``/*!...*/``)\n\
\n\
Returns:\n\
  str: Minified style");

static PyObject *
rcssmin_cssmin(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *style, *keep_bang_comments_ = NULL, *result;
    static char *kwlist[] = {"style", "keep_bang_comments", NULL};
    Py_ssize_t rlength, slength, length;
    int keep_bang_comments;
#ifdef EXT2
    int uni;
#endif
#ifdef EXT3
    int bytes;
    rchar *bytestyle;
#endif

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
                                     &style, &keep_bang_comments_))
        LCOV_EXCL_LINE_RETURN(NULL);

    if (!keep_bang_comments_)
        keep_bang_comments = 0;
    else {
        keep_bang_comments = PyObject_IsTrue(keep_bang_comments_);
        if (keep_bang_comments == -1)
            return NULL;
    }

#ifdef EXT2
    if (PyUnicode_Check(style)) {
        if (!(style = PyUnicode_AsUTF8String(style)))
            LCOV_EXCL_LINE_RETURN(NULL);
        uni = 1;
    }
    else if (!PyString_Check(style)) {
        PyErr_SetString(PyExc_TypeError, "Unexpected type");
        return NULL;
    }
    else {
        if (!(style = PyObject_Str(style)))
            LCOV_EXCL_LINE_RETURN(NULL);
        uni = 0;
    }

    /* Loop until the target buffer is big enough */
    rlength = slength = PyString_GET_SIZE(style);
    while (1) {
        if (!(result = PyString_FromStringAndSize(NULL, rlength))) {
            LCOV_EXCL_START

            Py_DECREF(style);
            return NULL;

            LCOV_EXCL_STOP
        }
        Py_BEGIN_ALLOW_THREADS
        length = rcssmin((rchar *)PyString_AS_STRING(style),
                         (rchar *)PyString_AS_STRING(result),
                         slength, rlength, keep_bang_comments);
        Py_END_ALLOW_THREADS

        if (length > rlength) {
            Py_DECREF(result);
            rlength = length;
            continue; /* Try again with a bigger buffer */
        }
        break;
    }

    Py_DECREF(style);
    if (length < 0) {
        LCOV_EXCL_START

        Py_DECREF(result);
        return NULL;

        LCOV_EXCL_STOP
    }

    if (length != rlength && _PyString_Resize(&result, length) == -1)
        LCOV_EXCL_LINE_RETURN(NULL);

    if (uni) {
        style = PyUnicode_DecodeUTF8(PyString_AS_STRING(result),
                                     PyString_GET_SIZE(result), "strict");
        Py_DECREF(result);
        return style;
    }

    return result;

#else  /* EXT3 */

    if (PyUnicode_Check(style)) {
        bytes = 0;
        style = PyUnicode_AsUTF8String(style);
        bytestyle = (rchar *)PyBytes_AS_STRING(style);
        slength = PyBytes_GET_SIZE(style);
    }
    else if (PyBytes_Check(style)) {
        bytes = 1;
        Py_INCREF(style);
        bytestyle = (rchar *)PyBytes_AS_STRING(style);
        slength = PyBytes_GET_SIZE(style);
    }
    else if (PyByteArray_Check(style)) {
        bytes = 2;
        Py_INCREF(style);
        bytestyle = (rchar *)PyByteArray_AS_STRING(style);
        slength = PyByteArray_GET_SIZE(style);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Unexpected type");
        return NULL;
    }

    /* Loop until the target buffer is big enough */
    rlength = slength;
    while (1) {
        if (!(result = PyBytes_FromStringAndSize(NULL, rlength))) {
            LCOV_EXCL_START

            Py_DECREF(style);
            return NULL;

            LCOV_EXCL_STOP
        }
        Py_BEGIN_ALLOW_THREADS
        length = rcssmin(bytestyle, (rchar *)PyBytes_AS_STRING(result),
                         slength, rlength, keep_bang_comments);
        Py_END_ALLOW_THREADS

        if (length > rlength) {
            Py_DECREF(result);
            rlength = length;
            continue; /* Try again with a bigger buffer */
        }
        break;
    }

    Py_DECREF(style);
    if (length < 0) {
        LCOV_EXCL_START

        Py_DECREF(result);
        return NULL;

        LCOV_EXCL_STOP
    }

    if (!bytes) {
        style = PyUnicode_DecodeUTF8(PyBytes_AS_STRING(result), length,
                                     "strict");
        Py_DECREF(result);
        return style;
    }
    if (bytes == 1) {
        if (length != slength) {
            _PyBytes_Resize(&result, length);
        }
        return result;
    }
    /* bytes == 2: bytearray */
    style = PyByteArray_FromStringAndSize(PyBytes_AS_STRING(result), length);
    Py_DECREF(result);
    return style;
#endif
}

/* ------------------------ BEGIN MODULE DEFINITION ------------------------ */

EXT_METHODS = {
    {"cssmin",
        EXT_CFUNC(rcssmin_cssmin), METH_VARARGS | METH_KEYWORDS,
        rcssmin_cssmin__doc__},

    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(EXT_DOCS_VAR,
"C implementation of rcssmin\n\
===========================\n\
\n\
C implementation of rcssmin.");


EXT_DEFINE(EXT_MODULE_NAME, EXT_METHODS_VAR, EXT_DOCS_VAR);

EXT_INIT_FUNC {
    PyObject *m;

    /* Create the module and populate stuff */
    if (!(m = EXT_CREATE(&EXT_DEFINE_VAR)))
        EXT_INIT_ERROR(LCOV_EXCL_LINE(NULL));

    EXT_ADD_UNICODE(m, "__author__", "Andr\xe9 Malo", "latin-1");
    EXT_ADD_STRING(m, "__version__", STRINGIFY(EXT_VERSION));

    EXT_INIT_RETURN(m);
}

/* ------------------------- END MODULE DEFINITION ------------------------- */
