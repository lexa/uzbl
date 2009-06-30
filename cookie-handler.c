/* -*- c-basic-offset: 4; -*- */
/*
 * Adapted from soup-cookie-jar.c
 *
 * Copyright (C) 2008 Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <libsoup/soup-cookie.h>
#include "cookie-handler.h"
#include <libsoup/soup-date.h>
//#include <libsoup/soup-marshal.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-session-feature.h>
#include <libsoup/soup-uri.h>

/**
 * SECTION:soup-cookie-handler
 * @short_description: Automatic cookie handling for #SoupSession
 *
 * A #CookieHandler stores #SoupCookie<!-- -->s and arrange for them
 * to be sent with the appropriate #SoupMessage<!-- -->s.
 * #CookieHandler implements #SoupSessionFeature, so you can add a
 * cookie handler to a session with soup_session_add_feature() or
 * soup_session_add_feature_by_type().
 *
 * Note that the base #CookieHandler class does not support any form
 * of long-term cookie persistence.
 **/

static void cookie_handler_session_feature_init (SoupSessionFeatureInterface *feature_interface, gpointer interface_data);
static void request_queued (SoupSessionFeature *feature, SoupSession *session,
                            SoupMessage *msg);
static void request_started (SoupSessionFeature *feature, SoupSession *session,
                             SoupMessage *msg, SoupSocket *socket);
static void request_unqueued (SoupSessionFeature *feature, SoupSession *session,
                              SoupMessage *msg);

G_DEFINE_TYPE_WITH_CODE (CookieHandler, cookie_handler, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (SOUP_TYPE_SESSION_FEATURE,
                         cookie_handler_session_feature_init))

enum {
    CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
    PROP_0,

    PROP_HANDLER,

    LAST_PROP
};

typedef struct {
    gboolean constructed;
    char *handler;
    GHashTable *domains;
} CookieHandlerPrivate;
#define COOKIE_HANDLER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUP_TYPE_COOKIE_HANDLER, CookieHandlerPrivate))

static void set_property (GObject *object, guint prop_id,
                          const GValue *value, GParamSpec *pspec);
static void get_property (GObject *object, guint prop_id,
                          GValue *value, GParamSpec *pspec);

static void
cookie_handler_init (CookieHandler *handler) {
    CookieHandlerPrivate *priv = COOKIE_HANDLER_GET_PRIVATE (handler);

    priv->domains = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);
}

static void
constructed (GObject *object) {
    CookieHandlerPrivate *priv = COOKIE_HANDLER_GET_PRIVATE (object);

    priv->constructed = TRUE;
}

static void
finalize (GObject *object) {
    CookieHandlerPrivate *priv = COOKIE_HANDLER_GET_PRIVATE (object);
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, priv->domains);
    while (g_hash_table_iter_next (&iter, &key, &value))
        soup_cookies_free (value);
    g_hash_table_destroy (priv->domains);

    G_OBJECT_CLASS (cookie_handler_parent_class)->finalize (object);
}

static void
cookie_handler_class_init (CookieHandlerClass *handler_class) {
    GObjectClass *object_class = G_OBJECT_CLASS (handler_class);

    g_type_class_add_private (handler_class, sizeof (CookieHandlerPrivate));

    object_class->constructed = constructed;
    object_class->finalize = finalize;
    object_class->set_property = set_property;
    object_class->get_property = get_property;

    /**
     * CookieHandler::changed
     * @handler: the #CookieHandler
     * @old_cookie: the old #SoupCookie value
     * @new_cookie: the new #SoupCookie value
     *
     * Emitted when @handler changes. If a cookie has been added,
     * @new_cookie will contain the newly-added cookie and
     * @old_cookie will be %NULL. If a cookie has been deleted,
     * @old_cookie will contain the to-be-deleted cookie and
     * @new_cookie will be %NULL. If a cookie has been changed,
     * @old_cookie will contain its old value, and @new_cookie its
     * new value.
     **/
    signals[CHANGED] =
        g_signal_new ("changed",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (CookieHandlerClass, changed),
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 2, 
                      SOUP_TYPE_COOKIE | G_SIGNAL_TYPE_STATIC_SCOPE,
                      SOUP_TYPE_COOKIE | G_SIGNAL_TYPE_STATIC_SCOPE);

    /**
     * COOKIE_HANDLER_HANDLER:
     *
     * Alias for the #CookieHandler:read-only property. (Whether
     * or not the cookie handler is read-only.)
     **/
    g_object_class_install_property (
        object_class, PROP_HANDLER,
        g_param_spec_string (COOKIE_HANDLER_HANDLER,
                             "Handler",
                             "The programme to handle cookies",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
cookie_handler_session_feature_init (SoupSessionFeatureInterface *feature_interface, gpointer interface_data) {
    (void) interface_data;
    feature_interface->request_queued = request_queued;
    feature_interface->request_started = request_started;
    feature_interface->request_unqueued = request_unqueued;
}

static void
set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    CookieHandlerPrivate *priv =
        COOKIE_HANDLER_GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_HANDLER:
        priv->handler = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    CookieHandlerPrivate *priv =
        COOKIE_HANDLER_GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_HANDLER:
        g_value_set_string (value, priv->handler);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * cookie_handler_new:
 *
 * Creates a new #CookieHandler. The base #CookieHandler class does
 * not support persistent storage of cookies; use a subclass for that.
 *
 * Returns: a new #CookieHandler
 *
 * Since: 2.24
 **/
CookieHandler *
cookie_handler_new (const char *handler) {
    g_return_val_if_fail (handler != NULL, NULL);

    return g_object_new (SOUP_TYPE_COOKIE_HANDLER,
                         COOKIE_HANDLER_HANDLER, handler,
                         NULL);
}

static void
cookie_handler_changed (CookieHandler *handler, SoupCookie *old, SoupCookie *new) {
    CookieHandlerPrivate *priv = COOKIE_HANDLER_GET_PRIVATE (handler);

    if (!priv->handler || !priv->constructed)
        return;

    g_signal_emit (handler, signals[CHANGED], 0, old, new);
}

/**
 * cookie_handler_get_cookies:
 * @handler: a #CookieHandler
 * @uri: a #SoupURI
 * @for_http: whether or not the return value is being passed directly
 * to an HTTP operation
 *
 * Retrieves (in Cookie-header form) the list of cookies that would
 * be sent with a request to @uri.
 *
 * If @for_http is %TRUE, the return value will include cookies marked
 * "HttpOnly" (that is, cookies that the server wishes to keep hidden
 * from client-side scripting operations such as the JavaScript
 * document.cookies property). Since #CookieHandler sets the Cookie
 * header itself when making the actual HTTP request, you should
 * almost certainly be setting @for_http to %FALSE if you are calling
 * this.
 *
 * Return value: the cookies, in string form, or %NULL if there are no
 * cookies for @uri.
 *
 * Since: 2.24
 **/
char *
cookie_handler_get_cookies (CookieHandler *handler, SoupURI *uri, gboolean for_http) {
    (void) for_http;
    CookieHandlerPrivate *priv;
    char *result;

    g_return_val_if_fail (SOUP_IS_COOKIE_HANDLER (handler), NULL);
    priv = COOKIE_HANDLER_GET_PRIVATE (handler);

    GString *s = g_string_new ("");
    g_string_printf(s, "GET '%s' '%s'", uri->host, uri->path);
    //TODO:
    //g_string_printf(s, "GET '%s' '%s' '%s'", uri->host, uri->path, soup_uri_to_string(uri,FALSE));
    run_handler(priv->handler, s->str);

    if(uzbl.comm.sync_stdout && strcmp (uzbl.comm.sync_stdout, "") != 0) {
        char *p = strchr(uzbl.comm.sync_stdout, '\n' );
        if ( p != NULL ) *p = '\0';
        result = (char*) g_strdup(uzbl.comm.sync_stdout);
    } else
        result = NULL;
    if (uzbl.comm.sync_stdout)
        uzbl.comm.sync_stdout = strfree(uzbl.comm.sync_stdout);

    g_string_free(s, TRUE);
    return result;
}


static void
process_set_cookie_header (SoupMessage *msg, gpointer user_data) {
    (void) user_data;
    char *cookie;
    GSList *ck;
    for (ck = soup_cookies_from_response(msg); ck; ck = ck->next){
        cookie = soup_cookie_to_set_cookie_header(ck->data);
        SoupURI * soup_uri = soup_message_get_uri(msg);
        GString *s = g_string_new ("");
        g_string_printf(s, "PUT '%s' '%s' '%s'", soup_uri->host, soup_uri->path, cookie);
        run_handler(uzbl.behave.cookie_handler, s->str);
        g_free (cookie);
        g_string_free(s, TRUE);
    }
    g_slist_free(ck);
}

static void
request_queued (SoupSessionFeature *feature, SoupSession *session, SoupMessage *msg) {
    (void) session;
    soup_message_add_header_handler (msg, "got-headers",
                                     "Set-Cookie",
                                     G_CALLBACK (process_set_cookie_header),
                                     feature);
}

static void
request_started (SoupSessionFeature *feature, SoupSession *session, SoupMessage *msg, SoupSocket *socket) {
    (void) feature;
    (void) session;
    (void) socket;
    CookieHandler *handler = COOKIE_HANDLER (feature);
    char *cookies;

    cookies = cookie_handler_get_cookies (handler, soup_message_get_uri (msg), TRUE);
    if (cookies) {
        soup_message_headers_replace (msg->request_headers,
                                      "Cookie", cookies);
        if (uzbl.state.verbose)
            printf("Cookies found: %s\n", cookies);
        g_free (cookies);
    } else {
        soup_message_headers_remove (msg->request_headers, "Cookie");
        if (uzbl.state.verbose)
            printf("No cookies found\n");
    }
}

static void
request_unqueued (SoupSessionFeature *feature, SoupSession *session, SoupMessage *msg) {
    (void) session;
    g_signal_handlers_disconnect_by_func (msg, process_set_cookie_header, feature);
}

/**
 * cookie_handler_all_cookies:
 * @handler: a #CookieHandler
 *
 * Constructs a #GSList with every cookie inside the @handler.
 * The cookies in the list are a copy of the original, so
 * you have to free them when you are done with them.
 *
 * Return value: a #GSList with all the cookies in the @handler.
 *
 * Since: 2.24
 **/
GSList *
cookie_handler_all_cookies (CookieHandler *handler) {
    CookieHandlerPrivate *priv;
    GHashTableIter iter;
    GSList *l = NULL;
    gpointer key, value;

    g_return_val_if_fail (SOUP_IS_COOKIE_HANDLER (handler), NULL);

    priv = COOKIE_HANDLER_GET_PRIVATE (handler);

    g_hash_table_iter_init (&iter, priv->domains);

    while (g_hash_table_iter_next (&iter, &key, &value)) {
        GSList *p, *cookies = value;
        for (p = cookies; p; p = p->next)
            l = g_slist_prepend (l, soup_cookie_copy (p->data));
    }

    return l;
}

/**
 * cookie_handler_delete_cookie:
 * @handler: a #CookieHandler
 * @cookie: a #SoupCookie
 *
 * Deletes @cookie from @handler, emitting the 'changed' signal.
 *
 * Since: 2.24
 **/
void
cookie_handler_delete_cookie (CookieHandler *handler, SoupCookie    *cookie) {
    CookieHandlerPrivate *priv;
    GSList *cookies, *p;
    char *domain;

    g_return_if_fail (SOUP_IS_COOKIE_HANDLER (handler));
    g_return_if_fail (cookie != NULL);

    priv = COOKIE_HANDLER_GET_PRIVATE (handler);

    domain = g_strdup (cookie->domain);

    cookies = g_hash_table_lookup (priv->domains, domain);
    if (cookies == NULL)
        return;

    for (p = cookies; p; p = p->next ) {
        SoupCookie *c = (SoupCookie*)p->data;
        if (soup_cookie_equal (cookie, c)) {
            cookies = g_slist_delete_link (cookies, p);
            g_hash_table_insert (priv->domains,
                         domain,
                         cookies);
            cookie_handler_changed (handler, c, NULL);
            soup_cookie_free (c);
            return;
        }
    }
}
/* vi: set et ts=4: */
