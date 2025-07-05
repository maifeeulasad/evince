#pragma once

#include <gtk/gtk.h>
#include "ev-sidebar-page.h"

G_BEGIN_DECLS

#define EV_TYPE_SIDEBAR_AI (ev_sidebar_ai_get_type())
G_DECLARE_FINAL_TYPE(EvSidebarAI, ev_sidebar_ai, EV, SIDEBAR_AI, GtkBox)

GtkWidget *ev_sidebar_ai_new(void);

G_END_DECLS
