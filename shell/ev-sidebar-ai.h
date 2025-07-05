#pragma once

#include <gtk/gtk.h>
#include "ev-sidebar-page.h"
#include "ev-document-model.h"

G_BEGIN_DECLS

#define EV_TYPE_SIDEBAR_AI (ev_sidebar_ai_get_type())
G_DECLARE_FINAL_TYPE(EvSidebarAI, ev_sidebar_ai, EV, SIDEBAR_AI, GtkBox)

struct _EvSidebarAI {
  GtkBox parent_instance;

  GtkWidget *summary_view;
  GtkWidget *question_entry;
  GtkWidget *ask_button;

  EvDocumentModel *model;
};

struct _EvSidebarAIClass {
  GtkBoxClass parent_class;
};

GtkWidget *ev_sidebar_ai_new(void);

G_END_DECLS
