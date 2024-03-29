/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2012 Giovanni Campagna <scampa.giovanni@gmail.com>
 *
 * The Control Center is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Control Center; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Thomas Wood <thos@gnome.org>
 */

#include <config.h>

#include <string.h>
#include <gio/gdesktopappinfo.h>

#include "cc-panel-loader.h"

#ifndef CC_PANEL_LOADER_NO_GTYPES

/* Extension points */
extern GType cc_background_panel_get_type (void);
#ifdef BUILD_BLUETOOTH
extern GType cc_bluetooth_panel_get_type (void);
#endif /* BUILD_BLUETOOTH */
extern GType cc_color_panel_get_type (void);
extern GType cc_date_time_panel_get_type (void);
extern GType cc_display_panel_get_type (void);
extern GType cc_info_panel_get_type (void);
extern GType cc_keyboard_panel_get_type (void);
extern GType cc_mouse_panel_get_type (void);
#ifdef BUILD_NETWORK
extern GType cc_network_panel_get_type (void);
#endif /* BUILD_NETWORK */
extern GType cc_notifications_panel_get_type (void);
extern GType cc_goa_panel_get_type (void);
extern GType cc_power_panel_get_type (void);
#ifdef BUILD_PRINTERS
extern GType cc_printers_panel_get_type (void);
#endif /* BUILD_PRINTERS */
extern GType cc_privacy_panel_get_type (void);
extern GType cc_region_panel_get_type (void);
extern GType cc_search_panel_get_type (void);
extern GType cc_sharing_panel_get_type (void);
extern GType cc_sound_panel_get_type (void);
extern GType cc_ua_panel_get_type (void);
extern GType cc_user_panel_get_type (void);
#ifdef BUILD_WACOM
extern GType cc_wacom_panel_get_type (void);
#endif /* BUILD_WACOM */

#define PANEL_TYPE(name, get_type) { name, get_type }

#else /* CC_PANEL_LOADER_NO_GTYPES */

#define PANEL_TYPE(name, get_type) { name }

#endif

static struct {
  const char *name;
#ifndef CC_PANEL_LOADER_NO_GTYPES
  GType (*get_type)(void);
#endif
} all_panels[] = {
  PANEL_TYPE("background",       cc_background_panel_get_type   ),
#ifdef BUILD_BLUETOOTH
  PANEL_TYPE("bluetooth",        cc_bluetooth_panel_get_type    ),
#endif
  PANEL_TYPE("color",            cc_color_panel_get_type        ),
  PANEL_TYPE("datetime",         cc_date_time_panel_get_type    ),
  PANEL_TYPE("display",          cc_display_panel_get_type      ),
  PANEL_TYPE("info",             cc_info_panel_get_type         ),
  PANEL_TYPE("keyboard",         cc_keyboard_panel_get_type     ),
  PANEL_TYPE("mouse",            cc_mouse_panel_get_type        ),
#ifdef BUILD_NETWORK
  PANEL_TYPE("network",          cc_network_panel_get_type      ),
#endif
  PANEL_TYPE("notifications",    cc_notifications_panel_get_type),
  PANEL_TYPE("online-accounts",  cc_goa_panel_get_type          ),
  PANEL_TYPE("power",            cc_power_panel_get_type        ),
#ifdef BUILD_PRINTERS
  PANEL_TYPE("printers",         cc_printers_panel_get_type     ),
#endif
  PANEL_TYPE("privacy",          cc_privacy_panel_get_type      ),
  PANEL_TYPE("region",           cc_region_panel_get_type       ),
  PANEL_TYPE("search",           cc_search_panel_get_type       ),
  PANEL_TYPE("sharing",          cc_sharing_panel_get_type      ),
  PANEL_TYPE("sound",            cc_sound_panel_get_type        ),
  PANEL_TYPE("universal-access", cc_ua_panel_get_type           ),
  PANEL_TYPE("user-accounts",    cc_user_panel_get_type         ),
#ifdef BUILD_WACOM
  PANEL_TYPE("wacom",            cc_wacom_panel_get_type        ),
#endif
};

GList *
cc_panel_loader_get_panels (void)
{
  GList *l = NULL;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (all_panels); i++)
    l = g_list_prepend (l, (gpointer) all_panels[i].name);

  return g_list_reverse (l);
}

static int
parse_categories (GDesktopAppInfo *app)
{
  const char *categories;
  char **split;
  int retval;
  int i;

  categories = g_desktop_app_info_get_categories (app);
  split = g_strsplit (categories, ";", -1);

  retval = -1;

  for (i = 0; split[i]; i++)
    {
      if (strcmp (split[i], "HardwareSettings") == 0)
        retval = CC_CATEGORY_HARDWARE;
      else if (strcmp (split[i], "X-GNOME-PersonalSettings") == 0)
        retval = CC_CATEGORY_PERSONAL;
      else if (strcmp (split[i], "X-GNOME-SystemSettings") == 0)
        retval = CC_CATEGORY_SYSTEM;
    }

  if (retval < 0)
    {
      g_warning ("Invalid categories %s for panel %s",
                 categories, g_app_info_get_id (G_APP_INFO (app)));
    }

  g_strfreev (split);
  return retval;
}

void
cc_panel_loader_fill_model (CcShellModel *model)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (all_panels); i++)
    {
      GDesktopAppInfo *app;
      char *desktop_name;
      int category;

      desktop_name = g_strconcat ("gnome-", all_panels[i].name,
                                  "-panel.desktop", NULL);
      app = g_desktop_app_info_new (desktop_name);
      g_free (desktop_name);

      if (app == NULL)
        {
          g_warning ("Ignoring broken panel %s (missing desktop file)",
                     all_panels[i].name);
          continue;
        }

      category = parse_categories (app);
      if (G_UNLIKELY (category < 0))
        continue;

      cc_shell_model_add_item (model, category, G_APP_INFO (app), all_panels[i].name);
      g_object_unref (app);
    }
}

#ifndef CC_PANEL_LOADER_NO_GTYPES

static GHashTable *panel_types;

static void
ensure_panel_types (void)
{
  int i;

  if (G_LIKELY (panel_types != NULL))
    return;

  panel_types = g_hash_table_new (g_str_hash, g_str_equal);
  for (i = 0; i < G_N_ELEMENTS (all_panels); i++)
    g_hash_table_insert (panel_types, (char*)all_panels[i].name, all_panels[i].get_type);
}

CcPanel *
cc_panel_loader_load_by_name (CcShell     *shell,
                              const char  *name,
                              GVariant    *parameters)
{
  GType (*get_type) (void);

  ensure_panel_types ();

  get_type = g_hash_table_lookup (panel_types, name);
  g_return_val_if_fail (get_type != NULL, NULL);

  return g_object_new (get_type (),
                       "shell", shell,
                       "parameters", parameters,
                       NULL);
}

#endif /* CC_PANEL_LOADER_NO_GTYPES */
