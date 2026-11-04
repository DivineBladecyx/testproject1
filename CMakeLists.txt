cmake_minimum_required(VERSION 3.16)

project(SIMM VERSION 0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    WebSockets
    Quick
    Qml
    QuickWidgets
    Svg
)

set(SIMM_SHARED_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/app
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/auth
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/network
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/data
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/session
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/audit
)

qt_add_library(simm_shared_core STATIC
    shared/app/clientbootstrap.cpp
    shared/app/clientbootstrap.h
    shared/app/startupstate.h
    shared/app/logoutstate.h
    shared/app/windowlifecycle.h
    shared/app/singleinstance.h

    shared/auth/logindialog.cpp
    shared/auth/logindialog.h
    shared/auth/logindialog.ui

    shared/network/websocketclient.cpp
    shared/network/websocketclient.h

    shared/theme/thememanager.cpp
    shared/theme/thememanager.h

    shared/data/DataProvider.h
    shared/data/telemetryvalue.h
    shared/data/roomcapabilities.h
    shared/session/tokenmanager.h

    shared/audit/auditlogger.cpp
    shared/audit/auditlogger.h

    resources.qrc
)

target_include_directories(simm_shared_core PUBLIC
    ${SIMM_SHARED_INCLUDE_DIRS}
)

target_link_libraries(simm_shared_core PUBLIC
    Qt6::Widgets
    Qt6::WebSockets
    Qt6::Svg
)

qt_add_executable(simm-treatment
    MANUAL_FINALIZATION
    apps/treatment/main.cpp
    apps/treatment/treatmentwindow.cpp
    apps/treatment/treatmentwindow.h
    apps/treatment/treatmentwindow.ui
    apps/treatment/spotmapwidget.cpp
    apps/treatment/spotmapwidget.h

    apps/treatment/todayschedule.cpp
    apps/treatment/todayschedule.h
    apps/treatment/todayschedule.ui

    apps/treatment/stepprogressmanager.cpp
    apps/treatment/stepprogressmanager.h

    apps/treatment/bedcontroller.cpp
    apps/treatment/bedcontroller.h

    models.qrc
)

target_include_directories(simm-treatment PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/treatment
)

target_link_libraries(simm-treatment PRIVATE
    simm_shared_core
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickWidgets
)

set_target_properties(simm-treatment PROPERTIES
    OUTPUT_NAME simm-treatment
)

qt_finalize_executable(simm-treatment)

qt_add_executable(simm-qa
    MANUAL_FINALIZATION
    apps/qa/main.cpp
    apps/qa/qawindow.cpp
    apps/qa/qawindow.h
    apps/qa/qawindow.ui
)

target_include_directories(simm-qa PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/qa
)

target_link_libraries(simm-qa PRIVATE
    simm_shared_core
)

set_target_properties(simm-qa PROPERTIES
    OUTPUT_NAME simm-qa
)

qt_finalize_executable(simm-qa)

qt_add_executable(SIMM
    MANUAL_FINALIZATION
    apps/launcher/main.cpp

    apps/treatment/treatmentwindow.cpp
    apps/treatment/treatmentwindow.h
    apps/treatment/treatmentwindow.ui
    apps/treatment/spotmapwidget.cpp
    apps/treatment/spotmapwidget.h
    apps/treatment/todayschedule.cpp
    apps/treatment/todayschedule.h
    apps/treatment/todayschedule.ui
    apps/treatment/stepprogressmanager.cpp
    apps/treatment/stepprogressmanager.h
    apps/treatment/bedcontroller.cpp
    apps/treatment/bedcontroller.h

    apps/qa/qawindow.cpp
    apps/qa/qawindow.h
    apps/qa/qawindow.ui

    models.qrc
)

target_include_directories(SIMM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/treatment
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/qa
)

target_link_libraries(SIMM PRIVATE
    simm_shared_core
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickWidgets
)

set_target_properties(SIMM PROPERTIES
    OUTPUT_NAME SIMM
)
set_target_properties(SIMM PROPERTIES WIN32_EXECUTABLE TRUE)
if(WIN32)
    target_sources(SIMM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/simm.rc"
    )
    set_source_files_properties(resources/simm.rc PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
endif()
qt_finalize_executable(SIMM)

foreach(SIMM_TARGET simm-treatment simm-qa SIMM)
    add_custom_command(TARGET ${SIMM_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${SIMM_TARGET}>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config/app_config.ini"
            "$<TARGET_FILE_DIR:${SIMM_TARGET}>/config/app_config.ini"
        COMMENT "Copying SIMM runtime configuration for ${SIMM_TARGET}"
    )
endforeach()
