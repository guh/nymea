include(../nymea.pri)

TARGET = nymea
TEMPLATE = lib

QT += network bluetooth
DEFINES += LIBNYMEA_LIBRARY

QMAKE_LFLAGS += -fPIC

target.path = /usr/lib/$$system('dpkg-architecture -q DEB_HOST_MULTIARCH')
INSTALLS += target

# Avahi libs
LIBS += -lavahi-common -lavahi-client

HEADERS += devicemanager.h \
        libnymea.h \
        typeutils.h \
        loggingcategories.h \
        nymeasettings.h \
        plugin/device.h \
        plugin/deviceplugin.h \
        plugin/devicedescriptor.h \
        plugin/devicepairinginfo.h \
        hardware/gpio.h \
        hardware/gpiomonitor.h \
        hardware/pwm.h \
        hardware/radio433/radio433.h \
        network/upnp/upnpdiscovery.h \
        network/upnp/upnpdevice.h \
        network/upnp/upnpdevicedescriptor.h \
        network/upnp/upnpdiscoveryreply.h \
        network/networkaccessmanager.h \
        network/oauth2.h \
        network/avahi/qtavahiservicebrowser.h \
        network/avahi/avahiserviceentry.h \
        #network/avahi/qtavahiclient.h \
        #network/avahi/qt-watch.h \
        #network/avahi/qtavahiservice.h \
        #network/avahi/qtavahiservice_p.h \
        #network/avahi/qtavahiservicebrowser_p.h \
        hardware/bluetoothlowenergy/bluetoothlowenergydevice.h \
        hardware/bluetoothlowenergy/bluetoothdiscoveryreply.h \
        hardware/bluetoothlowenergy/bluetoothlowenergymanager.h \
        coap/coap.h \
        coap/coappdu.h \
        coap/coapoption.h \
        coap/coaprequest.h \
        coap/coapreply.h \
        coap/coappdublock.h \
        coap/corelinkparser.h \
        coap/corelink.h \
        coap/coapobserveresource.h \
        types/deviceclass.h \
        types/action.h \
        types/actiontype.h \
        types/state.h \
        types/statetype.h \
        types/eventtype.h \
        types/event.h \
        types/eventdescriptor.h \
        types/vendor.h \
        types/paramtype.h \
        types/param.h \
        types/paramdescriptor.h \
        types/ruleaction.h \
        types/ruleactionparam.h \
        types/statedescriptor.h \
        types/interface.h \
        hardwareresource.h \
        plugintimer.h \
        hardwaremanager.h \
        nymeadbusservice.h \
        network/mqtt/mqttprovider.h \
        network/mqtt/mqttchannel.h

SOURCES += devicemanager.cpp \
        loggingcategories.cpp \
        nymeasettings.cpp \
        plugin/device.cpp \
        plugin/deviceplugin.cpp \
        plugin/devicedescriptor.cpp \
        plugin/devicepairinginfo.cpp \
        hardware/gpio.cpp \
        hardware/gpiomonitor.cpp \
        hardware/pwm.cpp \
        hardware/radio433/radio433.cpp \
        network/upnp/upnpdiscovery.cpp \
        network/upnp/upnpdevice.cpp \
        network/upnp/upnpdevicedescriptor.cpp \
        network/upnp/upnpdiscoveryreply.cpp \
        network/networkaccessmanager.cpp \
        network/oauth2.cpp \
        network/avahi/avahiserviceentry.cpp \
        network/avahi/qtavahiservicebrowser.cpp \
        #network/avahi/qt-watch.cpp \
        #network/avahi/qtavahiclient.cpp \
        #network/avahi/qtavahiservice.cpp \
        #network/avahi/qtavahiservice_p.cpp \
        #network/avahi/qtavahiservicebrowser_p.cpp \
        hardware/bluetoothlowenergy/bluetoothlowenergymanager.cpp \
        hardware/bluetoothlowenergy/bluetoothlowenergydevice.cpp \
        hardware/bluetoothlowenergy/bluetoothdiscoveryreply.cpp \
        coap/coap.cpp \
        coap/coappdu.cpp \
        coap/coapoption.cpp \
        coap/coaprequest.cpp \
        coap/coapreply.cpp \
        coap/coappdublock.cpp \
        coap/corelinkparser.cpp \
        coap/corelink.cpp \
        coap/coapobserveresource.cpp \
        types/deviceclass.cpp \
        types/action.cpp \
        types/actiontype.cpp \
        types/state.cpp \
        types/statetype.cpp \
        types/eventtype.cpp \
        types/event.cpp \
        types/eventdescriptor.cpp \
        types/vendor.cpp \
        types/paramtype.cpp \
        types/param.cpp \
        types/paramdescriptor.cpp \
        types/ruleaction.cpp \
        types/ruleactionparam.cpp \
        types/statedescriptor.cpp \
        types/interface.cpp \
        hardwareresource.cpp \
        plugintimer.cpp \
        hardwaremanager.cpp \
        nymeadbusservice.cpp \
        network/mqtt/mqttprovider.cpp \
        network/mqtt/mqttchannel.cpp


# install plugininfo python script for libnymea-dev
generateplugininfo.files = $$top_srcdir/plugins/nymea-generateplugininfo
generateplugininfo.path = /usr/bin

INSTALLS +=  generateplugininfo

# install header file with relative subdirectory
for(header, HEADERS) {
    path = /usr/include/nymea/$${dirname(header)}
    eval(headers_$${path}.files += $${header})
    eval(headers_$${path}.path = $${path})
    eval(INSTALLS *= headers_$${path})
}

RESOURCES += \
        interfaces/interfaces.qrc
