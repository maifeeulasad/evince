#include "ev-sidebar-ai.h"

struct _EvSidebarAI
{
  GtkBox parent_instance;

  GtkWidget *summary_view;
  GtkWidget *question_entry;
  GtkWidget *ask_button;
};

static void ev_sidebar_ai_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);

static void ev_sidebar_ai_sidebar_page_init(EvSidebarPageInterface *iface);
static void ev_sidebar_ai_set_model(EvSidebarPage *page, EvDocumentModel *model);
static gboolean ev_sidebar_ai_support_document(EvSidebarPage *page, EvDocument *document);

G_DEFINE_TYPE_WITH_CODE(
    EvSidebarAI,
    ev_sidebar_ai,
    GTK_TYPE_BOX,
    G_IMPLEMENT_INTERFACE(EV_TYPE_SIDEBAR_PAGE, ev_sidebar_ai_sidebar_page_init))

static void
ev_sidebar_ai_class_init(EvSidebarAIClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->get_property = ev_sidebar_ai_get_property;

  g_object_class_install_property(
      object_class,
      1,
      g_param_spec_object("main-widget",
                          "Main Widget",
                          "The main widget for the sidebar page",
                          GTK_TYPE_WIDGET,
                          G_PARAM_READABLE));
}

static void
ev_sidebar_ai_init(EvSidebarAI *self)
{
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);

  self->summary_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->summary_view), GTK_WRAP_WORD);
  gtk_box_pack_start(GTK_BOX(self), self->summary_view, TRUE, TRUE, 0);

  self->question_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(self), self->question_entry, FALSE, FALSE, 0);

  self->ask_button = gtk_button_new_with_label("Ask");
  gtk_box_pack_start(GTK_BOX(self), self->ask_button, FALSE, FALSE, 0);

  gtk_widget_show_all(GTK_WIDGET(self));
}

// Interface vtable: hook required methods
static void
ev_sidebar_ai_sidebar_page_init(EvSidebarPageInterface *iface)
{
  iface->set_model = ev_sidebar_ai_set_model;
  iface->support_document = ev_sidebar_ai_support_document;
}

static void
ev_sidebar_ai_set_model(EvSidebarPage *page, EvDocumentModel *model)
{
  g_return_if_fail(EV_IS_DOCUMENT_MODEL(model));
  // For now: do nothing
}

static gboolean
ev_sidebar_ai_support_document(EvSidebarPage *page, EvDocument *document)
{
  // For now: support all docs
  return TRUE;
}

static void
ev_sidebar_ai_get_property(GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  switch (prop_id)
  {
  case 1:
    g_value_set_object(value, GTK_WIDGET(object));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

GtkWidget *
ev_sidebar_ai_new(void)
{
  return g_object_new(EV_TYPE_SIDEBAR_AI, NULL);
}
