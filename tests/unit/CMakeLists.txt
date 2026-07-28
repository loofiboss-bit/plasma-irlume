# SPDX-License-Identifier: BSD-2-Clause

add_executable(test_systemstate test_systemstate.cpp)

target_link_libraries(test_systemstate
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME systemstate COMMAND test_systemstate)
set_tests_properties(systemstate PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_irlumebackend test_irlumebackend.cpp)

target_link_libraries(test_irlumebackend
    irlume_adapter
    Qt6::Core
    Qt6::Test
)

add_test(NAME irlumebackend COMMAND test_irlumebackend)
set_tests_properties(irlumebackend PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_refreshcoordinator test_refreshcoordinator.cpp)

target_link_libraries(test_refreshcoordinator
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME refreshcoordinator COMMAND test_refreshcoordinator)

add_executable(fake_preview_worker fake_preview_worker.cpp)
target_link_libraries(fake_preview_worker preview_protocol Qt6::Core Qt6::Gui)

add_executable(test_previewprotocol test_previewprotocol.cpp)
target_link_libraries(test_previewprotocol preview_protocol Qt6::Core Qt6::Test)
add_test(NAME previewprotocol COMMAND test_previewprotocol)

add_executable(test_cameraprovider test_cameraprovider.cpp)
target_link_libraries(test_cameraprovider camera_provider Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME cameraprovider COMMAND test_cameraprovider)

add_executable(test_previewworker test_previewworker.cpp)
target_compile_definitions(test_previewworker PRIVATE
    IRLUME_PREVIEW_WORKER_PATH="$<TARGET_FILE:plasma-irlume-camera-preview-worker>"
)
target_link_libraries(test_previewworker preview_protocol Qt6::Core Qt6::Test)
add_dependencies(test_previewworker plasma-irlume-camera-preview-worker)
add_test(NAME previewworker COMMAND test_previewworker)
set_tests_properties(previewworker PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_camerapreviewsession test_camerapreviewsession.cpp)
target_compile_definitions(test_camerapreviewsession PRIVATE
    IRLUME_FAKE_PREVIEW_WORKER_PATH="$<TARGET_FILE:fake_preview_worker>"
)
target_link_libraries(test_camerapreviewsession irlume_backend Qt6::Core Qt6::Gui Qt6::Test)
add_dependencies(test_camerapreviewsession fake_preview_worker)
add_test(NAME camerapreviewsession COMMAND test_camerapreviewsession)

add_executable(test_profilemodel test_profilemodel.cpp)

target_link_libraries(test_profilemodel
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME profilemodel COMMAND test_profilemodel)
set_tests_properties(profilemodel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_authconfiguration test_authconfiguration.cpp)

target_link_libraries(test_authconfiguration
    irlume_backend
    Qt6::Core
    Qt6::Test
)

add_test(NAME authconfiguration COMMAND test_authconfiguration)

add_executable(test_supportreport test_supportreport.cpp)

target_compile_definitions(test_supportreport PRIVATE
    IRLUME_FAKE_PREVIEW_WORKER_PATH="$<TARGET_FILE:fake_preview_worker>"
)
target_link_libraries(test_supportreport
    irlume_backend
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)
add_dependencies(test_supportreport fake_preview_worker)

add_test(NAME supportreport COMMAND test_supportreport)
set_tests_properties(supportreport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_qmlpages
    test_qmlpages.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/camerapreviewitem.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/camerapreviewitem.h
)

target_include_directories(test_qmlpages PRIVATE
    ${PROJECT_SOURCE_DIR}/src/kcm
)

target_compile_definitions(test_qmlpages PRIVATE
    IRLUME_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    IRLUME_FAKE_PREVIEW_WORKER_PATH="$<TARGET_FILE:fake_preview_worker>"
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
add_dependencies(test_qmlpages fake_preview_worker)

add_test(NAME qmlpages COMMAND test_qmlpages)
set_tests_properties(qmlpages PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_irlumekcm
    test_irlumekcm.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/camerapreviewitem.cpp
    ${PROJECT_SOURCE_DIR}/src/kcm/camerapreviewitem.h
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

add_executable(camera_hardware_check camera_hardware_check.cpp)
target_link_libraries(camera_hardware_check irlume_backend Qt6::Core Qt6::Gui)
add_dependencies(camera_hardware_check plasma-irlume-camera-preview-worker)
