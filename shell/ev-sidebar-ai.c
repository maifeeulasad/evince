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
static void ev_sidebar_ai_on_ask_clicked(GtkButton *button, EvSidebarAI *self);
gchar *escape_newlines(const gchar *input);

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WIDGET
};

G_DEFINE_TYPE_WITH_CODE(
    EvSidebarAI,
    ev_sidebar_ai,
    GTK_TYPE_BOX,
    G_IMPLEMENT_INTERFACE(EV_TYPE_SIDEBAR_PAGE, ev_sidebar_ai_sidebar_page_iface_init))

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

  self->page_list = gtk_list_box_new();
  gtk_widget_set_vexpand(self->page_list, TRUE);
  gtk_box_pack_start(GTK_BOX(self), self->page_list, TRUE, TRUE, 0);

  self->model_combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->model_combo), "deepseek-r1:8b");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(self->model_combo), "deepseek-r1:1.5b");
  gtk_combo_box_set_active(GTK_COMBO_BOX(self->model_combo), 0);
  gtk_box_pack_start(GTK_BOX(self), self->model_combo, FALSE, FALSE, 0);

  self->ask_button = gtk_button_new_with_label("Ask selected page");
  gtk_box_pack_start(GTK_BOX(self), self->ask_button, FALSE, FALSE, 0);
  g_signal_connect(self->ask_button, "clicked", G_CALLBACK(ev_sidebar_ai_on_ask_clicked), self);

  self->output_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->output_view), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(self->output_view, TRUE);
  gtk_box_pack_start(GTK_BOX(self), self->output_view, TRUE, TRUE, 0);

  gtk_widget_show_all(GTK_WIDGET(self));
}

static gboolean
ev_sidebar_ai_support_document(EvSidebarPage *page, EvDocument *document)
{
  return EV_IS_DOCUMENT_TEXT(document);
}

static void
ev_sidebar_ai_process_document(EvSidebarAI *self, EvDocument *document)
{
  gtk_container_foreach(GTK_CONTAINER(self->page_list), (GtkCallback)gtk_widget_destroy, NULL);

  int n_pages = ev_document_get_n_pages(document);
  for (int i = 0; i < n_pages; i++)
  {
    EvPage *page = ev_document_get_page(document, i);

    ev_document_doc_mutex_lock();
    gchar *text = ev_document_text_get_text(EV_DOCUMENT_TEXT(document), page);
    ev_document_doc_mutex_unlock();

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gchar *label = g_strdup_printf("Page %d: %.40s...", i + 1, text ? text : "");
    GtkWidget *label_widget = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(label_widget), 0.0);
    gtk_box_pack_start(GTK_BOX(row), label_widget, FALSE, FALSE, 0);

    gtk_list_box_insert(GTK_LIST_BOX(self->page_list), row, -1);

    g_free(label);
    g_free(text);
    g_object_unref(page);
  }

  gtk_widget_show_all(self->page_list);
}

static void
ev_sidebar_ai_document_changed_cb(EvDocumentModel *model,
                                  GParamSpec *pspec,
                                  EvSidebarAI *self)
{
  EvDocument *document = ev_document_model_get_document(model);
  if (document && EV_IS_DOCUMENT_TEXT(document))
  {
    ev_sidebar_ai_process_document(self, document);
  }
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
  if (document && EV_IS_DOCUMENT_TEXT(document))
  {
    ev_sidebar_ai_process_document(self, document);
  }
}

GtkWidget *
ev_sidebar_ai_new(void)
{
  return g_object_new(EV_TYPE_SIDEBAR_AI, NULL);
}

gchar *escape_newlines(const gchar *input)
{
  GString *result = g_string_new(NULL);

  for (const gchar *p = input; *p; ++p)
  {
    if (*p == '\n')
    {
      g_string_append(result, "\\n");
    }
    else
    {
      g_string_append_c(result, *p);
    }
  }

  return g_string_free(result, FALSE);
}

static void
ev_sidebar_ai_on_ask_clicked(GtkButton *button, EvSidebarAI *self)
{
  GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->page_list));
  if (!selected)
    return;

  int page_index = gtk_list_box_row_get_index(selected);

  EvDocument *doc = ev_document_model_get_document(self->model);
  EvPage *page = ev_document_get_page(doc, page_index);

  ev_document_doc_mutex_lock();
  gchar *text = ev_document_text_get_text(EV_DOCUMENT_TEXT(doc), page);
  ev_document_doc_mutex_unlock();

  const gchar *model = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->model_combo));
  if (!model)
    model = "deepseek-r1:1.5b";

  gchar *escaped = escape_newlines(text ? text : "");
  gchar *escaped_prompt = g_strescape(escaped, NULL);

  gchar *command = g_strdup_printf(
      "curl -s -H \"Content-Type: application/json\" localhost:11434/api/generate -d \"{\\\"model\\\": \\\"%s\\\", \\\"prompt\\\": \\\"Summarize: %s\\\", \\\"stream\\\": true}\"",
      model, escaped_prompt);

  FILE *fp = popen(command, "r");
  if (fp)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->output_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      line[strcspn(line, "\n")] = 0;

      char *start = strstr(line, "\"response\":\"");
      if (start)
      {
        start += strlen("\"response\":\"");
        char *end = strchr(start, '"');
        if (end)
          *end = '\0';

        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, start, -1);
      }
    }
    pclose(fp);
  }

  g_free(command);
  g_free(escaped_prompt);
  g_free(escaped);
  g_free(text);
  g_object_unref(page);
}

static void
ev_sidebar_ai_sidebar_page_iface_init(EvSidebarPageInterface *iface)
{
  iface->set_model = ev_sidebar_ai_set_model;
  iface->support_document = ev_sidebar_ai_support_document;
}