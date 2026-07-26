# SPDX-License-Identifier: BSD-2-Clause

add_executable(test_systemstate test_systemstate.cpp)

target_link_libraries(test_systemstate
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME systemstate COMMAND test_systemstate)

add_executable(test_irlumeprocess test_irlumeprocess.cpp)

target_compile_definitions(test_irlumeprocess PRIVATE
    IRLUME_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(test_irlumeprocess
    irlume_adapter
    Qt6::Core
    Qt6::Test
)

add_test(NAME irlumeprocess COMMAND test_irlumeprocess)

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

add_executable(test_authhelper test_authhelper.cpp)

target_compile_definitions(test_authhelper PRIVATE
    IRLUME_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(test_authhelper
    irlume_auth_helper
    Qt6::Core
    Qt6::Test
)

add_test(NAME authhelper COMMAND test_authhelper)

add_executable(test_authconfiguration test_authconfiguration.cpp)

target_link_libraries(test_authconfiguration
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME authconfiguration COMMAND test_authconfiguration)

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
