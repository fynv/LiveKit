set(PYTHON_TEST
sign_red.png
sign_green.png

test_image.py
test_camera.py
test_window_capture.py
test_window_record.py
test_compositor.py
test_ocv_pull.py
test_ocv_push.py
)

install(FILES ${PYTHON_TEST} DESTINATION test_python)
