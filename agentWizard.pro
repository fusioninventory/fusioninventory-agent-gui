#-------------------------------------------------
#
# Project created by QtCreator 2011-04-20T16:51:02
#
#-------------------------------------------------

QT       += core gui network

TARGET = agentWizard
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
    config.cpp \
    console.cpp

HEADERS  += dialog.h \
    config.h \
    console.h

FORMS    += dialog.ui \
    console.ui

RESOURCES += \
    FusionInventory.qrc

PRE_TARGETDEPS += FusionInventory.o

#Copy the .exe file into the build directory
EmbedBlobExe_FILES = FusionInventory.exe
EmbedBlobExe.input = EmbedBlobExe_FILES
EmbedBlobExe.output = ${QMAKE_FILE_BASE}.exe
unix:EmbedBlobExe.commands = cp -a ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT}
win32:EmbedBlobExe.commands = copy ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT}

#Create a .o binary file from the corresponding .S assembly
EmbedBlobObj_FILES = FusionInventory.S
EmbedBlobObj.input = EmbedBlobObj_FILES
EmbedBlobObj.depends = ${QMAKE_FILE_BASE}.exe
EmbedBlobObj.output = ${QMAKE_FILE_BASE}.o
EmbedBlobObj.commands = as -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}

QMAKE_EXTRA_COMPILERS += EmbedBlobExe EmbedBlobObj
