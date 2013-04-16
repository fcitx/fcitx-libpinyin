#ifndef FCITX_LIBPINYIN_BUS_H
#define FCITX_LIBPINYIN_BUS_H

#include <dbus/dbus.h>
#include <fcitx/instance.h>

class FcitxLibPinyinBus {
public:
    FcitxLibPinyinBus(struct _FcitxLibPinyinAddonInstance* libpinyin);
    virtual ~FcitxLibPinyinBus();

    DBusHandlerResult dbusEvent(DBusConnection* connection, DBusMessage* message);
private:
    DBusConnection* m_privconn;
    DBusConnection* m_conn;
    _FcitxLibPinyinAddonInstance* m_libpinyin;
};

#endif // FCITX_LIBPINYIN_BUS_H
