#include "bus.h"
#include "eim.h"
#include <fcitx/module/dbus/fcitx-dbus.h>

using namespace pinyin;

const char *introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node name=\"" FCITX_LIBPINYIN_PATH "\">\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" FCITX_LIBPINYIN_INTERFACE "\">\n"
    "    <method name=\"ImportDict\">"
    "    </method>"
    "    <method name=\"CleanUserData\">"
    "    </method>"
    "    <method name=\"CleanAllData\">"
    "    </method>"
    "  </interface>\n"
    "</node>\n";

static DBusHandlerResult dbusEventHandler(DBusConnection* conn, DBusMessage* message, void* self) {
    FcitxLibPinyinBus* bus = (FcitxLibPinyinBus*) self;
    return bus->dbusEvent(conn, message);
}

FcitxLibPinyinBus::FcitxLibPinyinBus(struct _FcitxLibPinyinAddonInstance* libpinyin)
{
    FcitxInstance* instance = libpinyin->owner;
    DBusConnection *conn = FcitxDBusGetConnection(instance);
    DBusConnection *privconn = FcitxDBusGetPrivConnection(instance);
    if (conn == NULL && privconn == NULL) {
        FcitxLog(ERROR, "DBus Not initialized");
    }

    m_libpinyin = libpinyin;
    m_conn = conn;
    m_privconn = privconn;

    DBusObjectPathVTable fcitxIPCVTable = {NULL, &dbusEventHandler, NULL, NULL, NULL, NULL };

    if (conn) {
        dbus_connection_register_object_path(conn, FCITX_LIBPINYIN_PATH, &fcitxIPCVTable, this);
    }

    if (privconn) {
        dbus_connection_register_object_path(privconn, FCITX_LIBPINYIN_PATH, &fcitxIPCVTable, this);
    }

}

FcitxLibPinyinBus::~FcitxLibPinyinBus()
{
    if (m_conn) {
        dbus_connection_unregister_object_path(m_conn, FCITX_LIBPINYIN_PATH);
    }
    if (m_privconn) {
        dbus_connection_unregister_object_path(m_privconn, FCITX_LIBPINYIN_PATH);
    }
}

DBusHandlerResult FcitxLibPinyinBus::dbusEvent(DBusConnection* connection, DBusMessage* message)
{
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage *reply = NULL;
    boolean flush = false;
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(message);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(message, FCITX_LIBPINYIN_INTERFACE, "ImportDict")) {
        FcitxLibPinyinImport(m_libpinyin->pinyin);
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, FCITX_LIBPINYIN_INTERFACE, "CleanUserData")) {
        FcitxLibPinyinCleanData(m_libpinyin->pinyin, true);
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, FCITX_LIBPINYIN_INTERFACE, "CleanAllData")) {
        FcitxLibPinyinCleanData(m_libpinyin->pinyin, false);
        reply = dbus_message_new_method_return(message);
    }

    if (reply) {
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        if (flush) {
            dbus_connection_flush(connection);
        }
        result = DBUS_HANDLER_RESULT_HANDLED;
    }
    return result;
}
