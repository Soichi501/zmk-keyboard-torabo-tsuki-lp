// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 sekigon-gonnoc

#include <zephyr/sys/util_macro.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_REGISTER(auto_mouse_layer, CONFIG_ZMK_LOG_LEVEL);

// オートマウスレイヤーとして使用するレイヤー番号
#define AUTO_MOUSE_LAYER 4

// トラックボール操作後、何も入力がない場合に元レイヤーへ戻るまでの時間 (ms)
#define AUTO_MOUSE_TIMEOUT_MS 1000

static struct k_work_delayable auto_mouse_deactivate_work;
static bool auto_mouse_active = false;

static void deactivate_auto_mouse_layer(struct k_work *work) {
    if (auto_mouse_active) {
        LOG_DBG("Auto mouse layer timeout - deactivating layer %d", AUTO_MOUSE_LAYER);
        zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
        auto_mouse_active = false;
    }
}

static void activate_auto_mouse_layer(void) {
    if (!auto_mouse_active) {
        LOG_DBG("Activating auto mouse layer %d", AUTO_MOUSE_LAYER);
        zmk_keymap_layer_activate(AUTO_MOUSE_LAYER);
        auto_mouse_active = true;
    }
    // タイムアウトをリセット
    k_work_reschedule(&auto_mouse_deactivate_work, K_MSEC(AUTO_MOUSE_TIMEOUT_MS));
}

// トラックボール入力を検知してオートマウスレイヤーを有効化
static void trackball_input_callback(struct input_event *evt) {
    activate_auto_mouse_layer();
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET_OR_NULL(DT_NODELABEL(trackball)), trackball_input_callback);

// 通常キー入力を検知してオートマウスレイヤーを即座に無効化
static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    // キーが押されたとき（離したときは無視）
    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (auto_mouse_active) {
        LOG_DBG("Key pressed - deactivating auto mouse layer immediately");
        k_work_cancel_delayable(&auto_mouse_deactivate_work);
        zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
        auto_mouse_active = false;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_position, position_state_changed_listener);
ZMK_SUBSCRIPTION(auto_mouse_position, zmk_position_state_changed);

static int auto_mouse_layer_init(void) {
    LOG_INF("Auto mouse layer initialized (layer=%d, timeout=%dms)",
            AUTO_MOUSE_LAYER, AUTO_MOUSE_TIMEOUT_MS);
    k_work_init_delayable(&auto_mouse_deactivate_work, deactivate_auto_mouse_layer);
    return 0;
}

SYS_INIT(auto_mouse_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
