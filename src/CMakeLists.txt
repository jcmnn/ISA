cmake_minimum_required(VERSION 3.1)

project(ISA)

set(I_SOURCE
	main.cpp

        isa.h
        isa.cpp
        logmodel.h
        logmodel.cpp
        log.h
        log.cpp

        section.h
        section.cpp

        ui/mainwindow.h
        ui/mainwindow.cpp

        ui/widgets/logview.h
        ui/widgets/logview.cpp

        ui/widgets/peinfowidget.h
        ui/widgets/peinfowidget.cpp

        files/pefile.h
        files/pefile.cpp
)

set(I_UIS
    ui/mainwindow.ui
    ui/widgets/peinfowidget.ui
    )

set(I_QRCS
    resources/style.qrc
    )

qt5_wrap_ui(I_UI ${I_UIS})
qt5_add_resources(I_RESOURCES ${I_QRCS})

set(CMAKE_AUTOMOC ON)

add_executable(ISA ${I_SOURCE} ${I_UI} ${I_RESOURCES})
target_link_libraries(ISA capstone-static Qt5::Widgets)
