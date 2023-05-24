TEMPLATE = lib
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += static

TARGET = sqlite3async
VERSION = 1.0.0

SOURCES += \
        main.cpp \
    src/sqlitedatacenter.cpp \
    src/sqlite3.c

HEADERS += \
    include/sqlite3.h \
    include/sqlitedatacenter.h \
    include/concurrentqueue.h


INCLUDEPATH +=include

LIBS += -lrt -lpthread -lz

LIBS += -static-libstdc++ -ldl

############################################################
#DEFINES += HW_ARM_RZG2L        #RZG2L硬件
#DEFINES += HW_ARM_TI           #TI335X硬件
#DEFINES += HW_LINUX_X86         #X86 Linux

# 判断硬件平台
unix:{
contains(QT_ARCH, arm64){
    DEFINES += HW_ARM_RZG2L
    message("arm linux 64")
    suffix = lib_arm_rzg2l
#    LIBS += -L$$PWD/sqlite/lib_arm_rz -lsqlite3
}else{
contains(QT_ARCH, arm){
    message("arm linux 32")
    DEFINES += HW_ARM_TI
    suffix = lib_arm_ti
#    LIBS += -L$$PWD/sqlite/lib_arm -lsqlite3
}else{
    message("linux x86")
    DEFINES += HW_LINUX_X86
    suffix = lib_x86_linux
#    LIBS += -L$$PWD/sqlite/lib_pc -l:sqlite3.a
}
}
}
win32:{
    DEFINES += HW_WIN_X86
    suffix = lib_x86_win
}


GIT_BRANCH   = $$system(git rev-parse --abbrev-ref HEAD)
GIT_HASH     = $$system(git show --oneline --format=\"%ci-%cn\" -s HEAD)
GIT_HASH2     = $$system(git describe --tags --dirty)
#PC_VERSION = "$${GIT_BRANCH}-$${GIT_HASH}"
PC_VERSION = "$${GIT_HASH2}"
GIT_BRANCH_ = "$${GIT_BRANCH}"
GIT_HEAD = "$${GIT_HASH}"
DEFINES += GIT_COMMIT_VERSION=\"\\\"$$PC_VERSION\\\"\"
DEFINES += GIT_BRANCH_INFO=\"\\\"$$GIT_BRANCH_\\\"\"
DEFINES += GIT_HASH_INFO=\"\\\"$$GIT_HEAD\\\"\"

#message($${GIT_BRANCH})



CONFIG(debug, debug|release) {
suffix = $${suffix}_debug
TARGET=$${TARGET}d
}
else{
suffix = $${suffix}_release
}
DESTDIR = output/libs/$${suffix}
OBJECTS_DIR = temp/temp_$${suffix}
UI_DIR = temp/temp_$${suffix}
MOC_DIR = temp/temp_$${suffix}
RCC_DIR = temp/temp_$${suffix}


unix:QMAKE_POST_LINK += "mkdir output/include -p && cp include/*.h  output/include/ -r"
