if(CONFIG_SHIELD_TORABO_TSUKI_LP_LEFT OR CONFIG_SHIELD_TORABO_TSUKI_LP_RIGHT)
  zephyr_library()
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
  zephyr_library_sources(src/board.c src/mini_trackpad_init_reg.c)
  # auto_mouse_layer.c を app library に追加することで
  # ZMK本体の input_listener.c と同じリンク順序にする
  # これにより INPUT_CALLBACK_DEFINE のコールバックが input_listener.c より先に実行される
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/auto_mouse_layer.c)
endif()
