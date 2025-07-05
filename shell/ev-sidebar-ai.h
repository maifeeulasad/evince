#pragma once

#include <gtk/gtk.h>
#include "ev-sidebar-page.h"
#include "ev-document-model.h"

G_BEGIN_DECLS

#define EV_TYPE_SIDEBAR_AI (ev_sidebar_ai_get_type())
G_DECLARE_FINAL_TYPE(EvSidebarAI, ev_sidebar_ai, EV, SIDEBAR_AI, GtkBox)

struct _EvSidebarAI
{
    GtkBox parent_instance;

    GtkWidget *page_list;
    GtkWidget *model_combo;
    GtkWidget *ask_button;
    GtkWidget *output_view;

    EvDocumentModel *model;
};

struct _EvSidebarAIClass
{
    GtkBoxClass parent_class;
};

GtkWidget *ev_sidebar_ai_new(void);

G_END_DECLS
