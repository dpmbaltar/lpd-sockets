#include <glib.h>
#include <json-glib/json-glib.h>

#include "util.h"

JsonNode *parse_json(const char *data, int length, JsonParser *parser)
{
  GError *error = NULL;

  g_return_val_if_fail(data != NULL, NULL);

  json_parser_load_from_data(parser, data, length, &error);

  if (error != NULL) {
    g_print("Error al obtener JSON `%s`: %s\n", data, error->message);
    g_error_free(error);
    return NULL;
  }

  return json_parser_steal_root(parser);
}

GDate *parse_date(const char *data)
{
  GDate *date;

  g_return_val_if_fail(data != NULL, NULL);

  date = g_date_new();
  g_date_set_parse(date, data);

  if (!g_date_valid(date)) {
    g_date_free(date);
    date = NULL;
  }

  return date;
}
