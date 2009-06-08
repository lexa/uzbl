/* -*- c-basic-offset: 4; -*- */
/*
 * Copyright (C) 2008 Red Hat, Inc.
 */

#ifndef COOKIE_HANDLER_H
#define COOKIE_HANDLER_H 1

#include <libsoup/soup-types.h>

G_BEGIN_DECLS

#define SOUP_TYPE_COOKIE_HANDLER            (cookie_handler_get_type ())
#define COOKIE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUP_TYPE_COOKIE_HANDLER, CookieHandler))
#define COOKIE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOUP_TYPE_COOKIE_HANDLER, CookieHandlerClass))
#define SOUP_IS_COOKIE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUP_TYPE_COOKIE_HANDLER))
#define SOUP_IS_COOKIE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), SOUP_TYPE_COOKIE_HANDLER))
#define COOKIE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUP_TYPE_COOKIE_HANDLER, CookieHandlerClass))

typedef struct {
	GObject parent;

} CookieHandler;

typedef struct {
	GObjectClass parent_class;

	void (*save)    (CookieHandler *handler);

	/* signals */
	void (*changed) (CookieHandler *handler,
			 SoupCookie    *old_cookie,
			 SoupCookie    *new_cookie);

	/* Padding for future expansion */
	void (*_libsoup_reserved1) (void);
	void (*_libsoup_reserved2) (void);
	void (*_libsoup_reserved3) (void);
} CookieHandlerClass;

#define COOKIE_HANDLER_HANDLER "handler"

GType          cookie_handler_get_type      (void);

CookieHandler *cookie_handler_new           (const char *handler);

char          *cookie_handler_get_cookies   (CookieHandler *handler,
					      SoupURI       *uri,
					      gboolean       for_http);
void           cookie_handler_set_cookie    (CookieHandler *handler,
					      SoupURI       *uri,
					      const char    *cookie);

void           cookie_handler_add_cookie    (CookieHandler *handler,
					      SoupCookie    *cookie);
void           cookie_handler_delete_cookie (CookieHandler *handler,
					      SoupCookie    *cookie);

GSList        *cookie_handler_all_cookies   (CookieHandler *handler);


G_END_DECLS

#endif /* COOKIE_HANDLER_H */
/* vi: set et ts=4: */
