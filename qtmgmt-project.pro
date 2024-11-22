TEMPLATE = subdirs

include(qtmgmt-includes.pri)

SUBDIRS += \
    qtmgmt \
    mcumgr

qtmgmt.depends += mcumgr
