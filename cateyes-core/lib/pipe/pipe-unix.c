#include "pipe-glue.h"

#include <unistd.h>

#if defined (HAVE_ANDROID)
# define CATEYES_TEMP_PATH "/data/local/tmp"
#elif defined (HAVE_QNX)
# define CATEYES_TEMP_PATH "/fs/tmpfs"
#else
# define CATEYES_TEMP_PATH "/tmp"
#endif

typedef struct _CateyesPipeTransportBackend CateyesPipeTransportBackend;

struct _CateyesPipeTransportBackend
{
  gchar * path;
};

static gchar * cateyes_pipe_generate_name (void);

static gchar * temp_directory = NULL;

static const gchar *
cateyes_pipe_transport_get_temp_directory (void)
{
  if (temp_directory != NULL)
    return temp_directory;
  else
    return CATEYES_TEMP_PATH;
}

void
cateyes_pipe_transport_set_temp_directory (const gchar * path)
{
  g_free (temp_directory);
  temp_directory = g_strdup (path);
}

void *
_cateyes_pipe_transport_create_backend (gchar ** local_address, gchar ** remote_address, GError ** error)
{
  CateyesPipeTransportBackend * backend;

  backend = g_slice_new (CateyesPipeTransportBackend);
  backend->path = cateyes_pipe_generate_name ();

  *local_address = g_strdup_printf ("pipe:role=server,path=%s", backend->path);
  *remote_address = g_strdup_printf ("pipe:role=client,path=%s", backend->path);

  return backend;
}

void
_cateyes_pipe_transport_destroy_backend (void * opaque_backend)
{
  CateyesPipeTransportBackend * backend = opaque_backend;

  unlink (backend->path);
  g_free (backend->path);

  g_slice_free (CateyesPipeTransportBackend, backend);
}

static gchar *
cateyes_pipe_generate_name (void)
{
  GString * s;
  guint i;

  s = g_string_new (cateyes_pipe_transport_get_temp_directory ());
  g_string_append (s, "/pipe-");
  for (i = 0; i != 16; i++)
    g_string_append_printf (s, "%02x", g_random_int_range (0, 255));

  return g_string_free (s, FALSE);
}
