/*
 * Code from: http://audacious-media-player.org/plugins
 * Original Source Code: http://hg.atheme.org/audacious-plugins/audacious-plugins/file/262b7a484275/src/gnomeshortcuts/gnomeshortcuts.c
 * By John Lindgren <john.lindgren@tds.net>
 * 
 * Updated by Paul Rouget <paul@mozilla.com>
 * Removed all the Audacious references and add support for Javascript Callbacks
 */

#include <string.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>
#include <glib-object.h>

#define MEDIA_KEY_MUTE 1
#define MEDIA_KEY_PLAY 2
#define MEDIA_KEY_PAUSE 3
#define MEDIA_KEY_STOP 4
#define MEDIA_KEY_PREVIOUS 5
#define MEDIA_KEY_NEXT 6

static DBusGProxy *media_player_keys_proxy = NULL;

void (*gJSCallback)(int);

#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)

static void hotkey_marshal_VOID__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data) {

    typedef void (*GMarshalFunc_VOID__STRING_STRING) (gpointer     data1,
                                                      gpointer     arg_1,
                                                      gpointer     arg_2);
    register GMarshalFunc_VOID__STRING_STRING callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1;

    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
    } else {
        data1 = g_value_peek_pointer (param_values + 0);
    }
    callback = (GMarshalFunc_VOID__STRING_STRING) (marshal_data ? marshal_data : cc->callback);

    callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2));
}

static void on_media_player_key_pressed (DBusGProxy *proxy, const gchar *application, const gchar *key) {
    if (strcmp ("Firefox", application) == 0) {
        if (strcmp ("Mute", key) == 0) {
            gJSCallback(MEDIA_KEY_MUTE);
            return;
        }
        if (strcmp ("Play", key) == 0) {
            gJSCallback(MEDIA_KEY_PLAY);
            return;
        }
        if (strcmp ("Pause", key) == 0) {
            gJSCallback(MEDIA_KEY_PAUSE);
            return;
        }
        if (strcmp ("Stop", key) == 0) {
            gJSCallback(MEDIA_KEY_STOP);
            return;
        }
        if (strcmp ("Previous", key) == 0) {
            gJSCallback(MEDIA_KEY_PREVIOUS);
            return;
        }
        if (strcmp ("Next", key) == 0) {
            gJSCallback(MEDIA_KEY_NEXT);
            return;
        }
    }
}

void uninit(){
    GError *error = NULL;
    if (media_player_keys_proxy == NULL) return;

    dbus_g_proxy_disconnect_signal (media_player_keys_proxy, "MediaPlayerKeyPressed",
            G_CALLBACK (on_media_player_key_pressed), NULL);

    dbus_g_proxy_call (media_player_keys_proxy,
            "ReleaseMediaPlayerKeys", &error,
            G_TYPE_STRING, "Firefox",
            G_TYPE_INVALID, G_TYPE_INVALID);
    if (error != NULL) {
        g_warning ("Could not release media player keys: %s", error->message);
        g_error_free (error);
    }
    g_object_unref(media_player_keys_proxy);
    media_player_keys_proxy = NULL;
}

void init (void (*callback)(int)) {
    gJSCallback = callback;
    DBusGConnection *bus;
    GError *error = NULL;
    dbus_g_thread_init();

    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if ((bus == NULL) || error) {
        g_warning ("Error connecting to DBus: %s", error->message);
    } else {
        media_player_keys_proxy = dbus_g_proxy_new_for_name (bus,
                "org.gnome.SettingsDaemon",
                "/org/gnome/SettingsDaemon/MediaKeys",
                "org.gnome.SettingsDaemon.MediaKeys");
        if (media_player_keys_proxy == NULL) return;

        dbus_g_proxy_call (media_player_keys_proxy,
                "GrabMediaPlayerKeys", &error,
                G_TYPE_STRING, "Firefox",
                G_TYPE_UINT, 0,
                G_TYPE_INVALID,
                G_TYPE_INVALID);
        if (error != NULL) {
            g_error_free (error);
            error = NULL;
            g_object_unref(media_player_keys_proxy);
            media_player_keys_proxy = NULL;
            media_player_keys_proxy = dbus_g_proxy_new_for_name (bus,
                    "org.gnome.SettingsDaemon",
                    "/org/gnome/SettingsDaemon",
                    "org.gnome.SettingsDaemon");
            if (media_player_keys_proxy == NULL) return;

            dbus_g_proxy_call (media_player_keys_proxy,
                    "GrabMediaPlayerKeys", &error,
                    G_TYPE_STRING, "Firefox",
                    G_TYPE_UINT, 0,
                    G_TYPE_INVALID,
                    G_TYPE_INVALID);
            if (error != NULL) {
                g_warning ("Could not grab media player keys: %s", error->message);
                g_error_free (error);
                g_object_unref(media_player_keys_proxy);
                media_player_keys_proxy = NULL;
                return;
            }
        }

        dbus_g_object_register_marshaller (hotkey_marshal_VOID__STRING_STRING,
                G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

        dbus_g_proxy_add_signal (media_player_keys_proxy, "MediaPlayerKeyPressed",
                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

        dbus_g_proxy_connect_signal (media_player_keys_proxy, "MediaPlayerKeyPressed",
                G_CALLBACK (on_media_player_key_pressed), NULL, NULL);
    }
}
