# qtmgmt global include qmake project settings
# By default all components are built for Github qtmgmt releases

# Build for x86_64 and arm64 on mac
QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64

DEFINES += SKIPPLUGIN_LOGGER
DEFINES += PLUGIN_MCUMGR_TRANSPORT_UART

qtHaveModule(bluetooth) {
    # Requires qtconnectivity
    DEFINES += PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH
}

qtHaveModule(network) {
    # Requires qtnetwork
    DEFINES += PLUGIN_MCUMGR_TRANSPORT_UDP

    qtHaveModule(mqtt) {
	# Requires qtnetwork and qtmqtt
	DEFINES += PLUGIN_MCUMGR_TRANSPORT_LORAWAN
    }
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH) {
    ADDITIONAL_MODULES += "bluetooth"
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UDP) | contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    ADDITIONAL_MODULES += "network"
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    ADDITIONAL_MODULES += "mqtt"
}
