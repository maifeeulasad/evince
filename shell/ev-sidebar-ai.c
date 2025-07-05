#include "config.h"

#include <string.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "ev-sidebar-ai.h"
#include "ev-document.h"
#include "ev-document-text.h"

static void ev_sidebar_ai_sidebar_page_iface_init(EvSidebarPageInterface *iface);
static void ev_sidebar_ai_set_model(EvSidebarPage *page, EvDocumentModel *model);
static void ev_sidebar_ai_document_changed_cb(EvDocumentModel *model, GParamSpec *pspec, EvSidebarAI *self);

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WIDGET
};

G_DEFINE_TYPE_WITH_CODE(
  EvSidebarAI,
  ev_sidebar_ai,
  GTK_TYPE_BOX,
  G_IMPLEMENT_INTERFACE(EV_TYPE_SIDEBAR_PAGE, ev_sidebar_ai_sidebar_page_iface_init)
)

static void ev_sidebar_ai_process_document(EvSidebarAI *self, EvDocument *document);

static void
ev_sidebar_ai_set_property(GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  EvSidebarAI *self = EV_SIDEBAR_AI(object);

  switch (prop_id)
  {
  case PROP_MODEL:
    ev_sidebar_ai_set_model(EV_SIDEBAR_PAGE(self), g_value_get_object(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void
ev_sidebar_ai_get_property(GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  switch (prop_id)
  {
  case PROP_WIDGET:
    g_value_set_object(value, GTK_WIDGET(object));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void
ev_sidebar_ai_class_init(EvSidebarAIClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = ev_sidebar_ai_set_property;
  gobject_class->get_property = ev_sidebar_ai_get_property;

  g_object_class_install_property(
      gobject_class,
      PROP_MODEL,
      g_param_spec_object("model",
                          "Document model",
                          "Document model for the sidebar AI",
                          EV_TYPE_DOCUMENT_MODEL,
                          G_PARAM_WRITABLE));

  g_object_class_install_property(
      gobject_class,
      PROP_WIDGET,
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

static gboolean
ev_sidebar_ai_support_document(EvSidebarPage *page, EvDocument *document)
{
  return EV_IS_DOCUMENT_TEXT(document);
}

static void
ev_sidebar_ai_set_model(EvSidebarPage *page, EvDocumentModel *model)
{
  EvSidebarAI *self = EV_SIDEBAR_AI(page);

  if (self->model == model)
    return;

  if (self->model)
    g_object_unref(self->model);

  self->model = g_object_ref(model);

  g_signal_connect(model,
                   "notify::document",
                   G_CALLBACK(ev_sidebar_ai_document_changed_cb),
                   self);

  EvDocument *document = ev_document_model_get_document(model);
  if (document && EV_IS_DOCUMENT_TEXT(document)) {
    ev_sidebar_ai_process_document(self, document);
  }
}

static void
ev_sidebar_ai_sidebar_page_iface_init(EvSidebarPageInterface *iface)
{
  iface->set_model = ev_sidebar_ai_set_model;
  iface->support_document = ev_sidebar_ai_support_document;
}

GtkWidget *
ev_sidebar_ai_new(void)
{
  return g_object_new(EV_TYPE_SIDEBAR_AI, NULL);
}

static void
ev_sidebar_ai_process_document(EvSidebarAI *self, EvDocument *document)
{

  const gchar *uri = ev_document_get_uri(document);
  gchar *path = g_filename_from_uri(uri, NULL, NULL);
  gchar *basename = g_path_get_basename(path);
  gchar *name = g_strndup(basename, strlen(basename) - 4);

  int n_pages = ev_document_get_n_pages(document);

  for (int i = 0; i < n_pages; i++)
  {
    EvPage *page = ev_document_get_page(document, i);

    EvRectangle *areas = NULL;
    guint n_areas = 0;

    ev_document_doc_mutex_lock();
    gchar *text = ev_document_text_get_text(EV_DOCUMENT_TEXT(document), page);
    gboolean success = ev_document_text_get_text_layout(EV_DOCUMENT_TEXT(document), page, &areas, &n_areas);
    ev_document_doc_mutex_unlock();

    char json_filename[128];
    snprintf(json_filename, sizeof(json_filename), "tempdf/%s-page-%d.json", name, i + 1);
    FILE *json_file = g_fopen(json_filename, "w");
    if (!json_file)
    {
      g_warning("⚠️ Could not open %s", json_filename);
      continue;
    }

    fprintf(json_file, "{ \"page\": %d, \"lines\": [\n", i + 1);
    if (text && success && areas)
    {
      gchar **lines = g_strsplit(text, "\n", -1);
      for (guint j = 0; j < n_areas && lines[j]; j++)
      {
        EvRectangle rect = areas[j];
        fprintf(json_file, "{ \"line\": %d, \"x\": %.2f, \"y\": %.2f, \"text\": \"%s\" }%s\n",
                j + 1, rect.x1, rect.y1, lines[j],
                (j + 1 < n_areas && lines[j + 1]) ? "," : "");
      }
      g_strfreev(lines);
    }
    fprintf(json_file, "]}\n");
    fclose(json_file);

    g_print("📝 Wrote %s\n", json_filename);

    g_free(areas);
    g_free(text);
    g_object_unref(page);
  }

  g_free(name);
  g_free(basename);
  g_free(path);
}

static void
ev_sidebar_ai_document_changed_cb(EvDocumentModel *model,
                                  GParamSpec      *pspec,
                                  EvSidebarAI     *self)
{
  EvDocument *document = ev_document_model_get_document(model);
  if (document && EV_IS_DOCUMENT_TEXT(document)) {
    ev_sidebar_ai_process_document(self, document);
  }
}