TEMPLATE = subdirs
CONFIG += ordered

WITH_BLACKMISC = ON
WITH_BLACKCORE = ON
WITH_BLACKD = ON
WITH_BLACKBOX = ON
WITH_SAMPLES = ON
WITH_UNITTESTS = ON

#WITH_DRIVER_FSX = ON
#WITH_DRIVER_FS9 = ON
#WITH_DRIVER_XPLANE = ON

equals(WITH_BLACKMISC, ON) {
    SUBDIRS += src/blackmisc
}

equals(WITH_BLACKCORE, ON) {
    SUBDIRS += src/blackcore
}

equals(WITH_BLACKD, ON) {
    SUBDIRS += src/blackd
}

equals(WITH_BLACKBOX, ON) {
    SUBDIRS += src/blackbox
}

equals(WITH_DRIVER_FSX, ON) {
    SUBDIRS += src/driver/fsx/driver_fsx.pro
}

equals(WITH_DRIVER_FS9, ON) {
    SUBDIRS += src/driver/fs9/driver_fs9.pro
}

equals(WITH_DRIVER_XPLANE, ON) {
    SUBDIRS += src/driver/xplane/driver_xplane.pro
}

equals(WITH_SAMPLES, ON) {
    SUBDIRS += samples/com_client/sample_com_client.pro
    SUBDIRS += samples/com_server/sample_com_server.pro
    SUBDIRS += samples/config/sample_config.pro
    SUBDIRS += samples/geodetic2ecef/sample_geodetic2ecef.pro
    SUBDIRS += samples/interpolator/sample_interpolator.pro
    SUBDIRS += samples/logging/sample_logging.pro
    SUBDIRS += samples/blackmiscquantities/sample_quantities_avionics.pro
}

equals(WITH_UNITTESTS, ON) {
    SUBDIRS += tests/blackmisc/test_blackmisc.pro
}
