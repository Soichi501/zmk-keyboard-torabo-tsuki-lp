// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 sekigon-gonnoc

// NOTE: オートマウスレイヤーとスクロール変換は
// zmk-input-behavior-listener モジュール（tb_mmv_ibl / tb_msl_ibl）が担当する。
// このファイルはキー入力によるオートマウスレイヤー解除のみを管理する。

#include <zephyr/sys/util_macro.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_REGISTER(auto_mouse_layer, CONFIG_ZMK_LOG_LEVEL);

#define AUTO_MOUSE_LAYER 4

// オートマウスレイヤー中に押してもレイヤーを解除しないキーのposition番号
// position 28: &mo 5  (スクロールレイヤー)
// position 33: &mkp LCLK
// position 34: &mkp MCLK
// position 35: &mkp RCLK
static const uint32_t mouse_layer_positions[] = {28, 33, 34, 35};

static bool is_mouse_layer_position(uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(mouse_layer_positions); i++) {
        if (mouse_layer_positions[i] == position) {
            return true;
        }
    }
    return false;
}

// 通常キー入力でオートマウスレイヤーを即座に解除する
static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (zmk_keymap_layer_active(AUTO_MOUSE_LAYER)) {
        if (is_mouse_layer_position(ev->position)) {
            LOG_DBG("Mouse layer key at position %d - keeping auto mouse layer", ev->position);
            return ZMK_EV_EVENT_BUBBLE;
        }

        LOG_DBG("Key pressed at position %d - deactivating auto mouse layer", ev->position);
        zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_position, position_state_changed_listener);
ZMK_SUBSCRIPTION(auto_mouse_position, zmk_position_state_changed);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
