# SPDX-License-Identifier: BSD-2-Clause

add_executable(test_systemstate test_systemstate.cpp)

target_link_libraries(test_systemstate
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME systemstate COMMAND test_systemstate)

add_executable(test_irlumebackend test_irlumebackend.cpp)

target_link_libraries(test_irlumebackend
    irlume_adapter
    Qt6::Core
    Qt6::Test
)

add_test(NAME irlumebackend COMMAND test_irlumebackend)

add_executable(test_refreshcoordinator test_refreshcoordinator.cpp)

target_link_libraries(test_refreshcoordinator
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME refreshcoordinator COMMAND test_refreshcoordinator)

add_executable(test_enrollmentsession test_enrollmentsession.cpp)

target_link_libraries(test_enrollmentsession
    irlume_adapter
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME enrollmentsession COMMAND test_enrollmentsession)

add_executable(test_profilemodel test_profilemodel.cpp)

target_link_libraries(test_profilemodel
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME profilemodel COMMAND test_profilemodel)

add_executable(test_authconfiguration test_authconfiguration.cpp)

target_link_libraries(test_authconfiguration
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME authconfiguration COMMAND test_authconfiguration)

add_executable(test_cameraconfiguration test_cameraconfiguration.cpp)

target_link_libraries(test_cameraconfiguration
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME cameraconfiguration COMMAND test_cameraconfiguration)

add_executable(test_supportreport test_supportreport.cpp)

target_link_libraries(test_supportreport
    irlume_backend
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME supportreport COMMAND test_supportreport)
set_tests_properties(supportreport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_qmlpages
    test_qmlpages.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/enrollmentpreviewitem.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/enrollmentpreviewitem.h
)

target_include_directories(test_qmlpages PRIVATE
    ${PROJECT_SOURCE_DIR}/src/kcm
)

target_compile_definitions(test_qmlpages PRIVATE
    IRLUME_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(test_qmlpages
    irlume_backend
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    KF6::I18nQml
)

add_test(NAME qmlpages COMMAND test_qmlpages)
set_tests_properties(qmlpages PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_irlumekcm
    test_irlumekcm.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/enrollmentpreviewitem.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/enrollmentpreviewitem.h
    ${PROJECT_SOURCE_DIR}/src/kcm/irlumekcm.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/irlumekcm.h
)

target_include_directories(test_irlumekcm PRIVATE
    ${PROJECT_SOURCE_DIR}/src/kcm
)

target_link_libraries(test_irlumekcm
    irlume_backend
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtilsQuick
)

add_test(NAME irlumekcm COMMAND test_irlumekcm)
set_tests_properties(irlumekcm PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
