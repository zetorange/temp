#include "cateyes-core.h"

#include <stdlib.h>
#include <string.h>

static void on_message (CateyesScript * script, const gchar * message, GBytes * data, gpointer user_data);
static void on_signal (int signo);
static gboolean stop (gpointer user_data);

static GMainLoop * loop = NULL;

int
main (int argc,
      char * argv[])
{
  guint target_pid;
  CateyesDeviceManager * manager;
  GError * error = NULL;
  CateyesDeviceList * devices;
  gint num_devices, i;
  CateyesDevice * local_device;
  CateyesSession * session;

  cateyes_init ();

  if (argc != 2 || (target_pid = atoi (argv[1])) == 0)
  {
    g_printerr ("Usage: %s <pid>\n", argv[0]);
    return 1;
  }

  loop = g_main_loop_new (NULL, TRUE);

  signal (SIGINT, on_signal);
  signal (SIGTERM, on_signal);

  manager = cateyes_device_manager_new ();

  devices = cateyes_device_manager_enumerate_devices_sync (manager, &error);
  g_assert (error == NULL);

  local_device = NULL;
  num_devices = cateyes_device_list_size (devices);
  for (i = 0; i != num_devices; i++)
  {
    CateyesDevice * device = cateyes_device_list_get (devices, i);

    g_print ("[*] Found device: \"%s\"\n", cateyes_device_get_name (device));

    if (cateyes_device_get_dtype (device) == CATEYES_DEVICE_TYPE_LOCAL)
      local_device = g_object_ref (device);

    g_object_unref (device);
  }
  g_assert (local_device != NULL);

  cateyes_unref (devices);
  devices = NULL;

  session = cateyes_device_attach_sync (local_device, target_pid, &error);
  if (error == NULL)
  {
    CateyesScript * script;

    g_print ("[*] Attached\n");

    script = cateyes_session_create_script_sync (session, "example",
        "Interceptor.attach(Module.findExportByName(null, 'open'), {\n"
        "  onEnter: function (args) {\n"
        "    console.log('[*] open(\"' + Memory.readUtf8String(args[0]) + '\")');\n"
        "  }\n"
        "});\n"
        "Interceptor.attach(Module.findExportByName(null, 'close'), {\n"
        "  onEnter: function (args) {\n"
        "    console.log('[*] close(' + args[0].toInt32() + ')');\n"
        "  }\n"
        "});",
        &error);
    g_assert (error == NULL);

    g_signal_connect (script, "message", G_CALLBACK (on_message), NULL);

    cateyes_script_load_sync (script, &error);
    g_assert (error == NULL);

    g_print ("[*] Script loaded\n");

    if (g_main_loop_is_running (loop))
      g_main_loop_run (loop);

    g_print ("[*] Stopped\n");

    cateyes_script_unload_sync (script, NULL);
    cateyes_unref (script);
    g_print ("[*] Unloaded\n");

    cateyes_session_detach_sync (session);
    cateyes_unref (session);
    g_print ("[*] Detached\n");
  }
  else
  {
    g_printerr ("Failed to attach: %s\n", error->message);
    g_error_free (error);
  }

  cateyes_unref (local_device);

  cateyes_device_manager_close_sync (manager);
  cateyes_unref (manager);
  g_print ("[*] Closed\n");

  g_main_loop_unref (loop);

  return 0;
}

static void
on_message (CateyesScript * script,
            const gchar * message,
            GBytes * data,
            gpointer user_data)
{
  JsonParser * parser;
  JsonObject * root;
  const gchar * type;

  parser = json_parser_new ();
  json_parser_load_from_data (parser, message, -1, NULL);
  root = json_node_get_object (json_parser_get_root (parser));

  type = json_object_get_string_member (root, "type");
  if (strcmp (type, "log") == 0)
  {
    const gchar * log_message;

    log_message = json_object_get_string_member (root, "payload");
    g_print ("%s\n", log_message);
  }
  else
  {
    g_print ("on_message: %s\n", message);
  }

  g_object_unref (parser);
}

static void
on_signal (int signo)
{
  g_idle_add (stop, NULL);
}

static gboolean
stop (gpointer user_data)
{
  g_main_loop_quit (loop);

  return FALSE;
}
