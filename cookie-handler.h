/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Red Hat, Inc.
 */

#ifndef SOUP_COOKIE_HANDLER_H
#define SOUP_COOKIE_HANDLER_H 1

#include <libsoup/soup-types.h>

G_BEGIN_DECLS

#define SOUP_TYPE_COOKIE_HANDLER            (soup_cookie_handler_get_type ())
#define SOUP_COOKIE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUP_TYPE_COOKIE_HANDLER, SoupCookieHandler))
#define SOUP_COOKIE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOUP_TYPE_COOKIE_HANDLER, SoupCookieHandlerClass))
#define SOUP_IS_COOKIE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUP_TYPE_COOKIE_HANDLER))
#define SOUP_IS_COOKIE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), SOUP_TYPE_COOKIE_HANDLER))
#define SOUP_COOKIE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUP_TYPE_COOKIE_HANDLER, SoupCookieHandlerClass))

typedef struct {
	GObject parent;

} SoupCookieHandler;

typedef struct {
	GObjectClass parent_class;

	void (*save)    (SoupCookieHandler *handler);

	/* signals */
	void (*changed) (SoupCookieHandler *handler,
			 SoupCookie    *old_cookie,
			 SoupCookie    *new_cookie);

	/* Padding for future expansion */
	void (*_libsoup_reserved1) (void);
	void (*_libsoup_reserved2) (void);
	void (*_libsoup_reserved3) (void);
} SoupCookieHandlerClass;

#define SOUP_COOKIE_HANDLER_HANDLER "handler"

GType          soup_cookie_handler_get_type      (void);

SoupCookieHandler *soup_cookie_handler_new           (const char *handler);

#ifndef LIBSOUP_DISABLE_DEPRECATED
void           soup_cookie_handler_save          (SoupCookieHandler *handler);
#endif

char          *soup_cookie_handler_get_cookies   (SoupCookieHandler *handler,
					      SoupURI       *uri,
					      gboolean       for_http);
void           soup_cookie_handler_set_cookie    (SoupCookieHandler *handler,
					      SoupURI       *uri,
					      const char    *cookie);

void           soup_cookie_handler_add_cookie    (SoupCookieHandler *handler,
					      SoupCookie    *cookie);
void           soup_cookie_handler_delete_cookie (SoupCookieHandler *handler,
					      SoupCookie    *cookie);

GSList        *soup_cookie_handler_all_cookies   (SoupCookieHandler *handler);


G_END_DECLS

#endif /* SOUP_COOKIE_HANDLER_H */
