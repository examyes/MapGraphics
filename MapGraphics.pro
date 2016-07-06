TEMPLATE = subdirs

SUBDIRS += MapGraphics \
    TestApp \
    TileDownloader \

TestApp.depends += MapGraphics
TileDownloader.depends += MapGraphics
