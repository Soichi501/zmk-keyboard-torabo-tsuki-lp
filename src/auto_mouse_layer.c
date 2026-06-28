// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 sekigon-gonnoc

// オートマウスレイヤー管理
// - 通常キー押下時: 即座にlayer 4解除
// - クリックキー離し時: CLICK_RELEASE_DELAY_MS後にlayer 4解除
//   （ダブルクリック対応: 猶予時間内にトラックボール操作または再クリックでキャンセル）

#include <zephyr/sys/util_macro.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_REGISTER(auto_mouse_layer, CONFIG_ZMK_LOG_LEVEL);

#define AUTO_MOUSE_LAYER 4

// クリック離し後、layer 4を解除するまでの猶予時間 (ms)
// ダブルクリックのインターバルより長くする必要はない
// 短すぎるとダブルクリックが困難になる
#define CLICK_RELEASE_DELAY_MS 200

// layer 4中に押してもlayer 4を即座解除しないキーのposition番号
// ※ クリックキー（33, 34, 35）はrelease時に猶予タイマーで解除
// ※ &mo 5（28）はlayer 4を維持したままスクロールレイヤーへ
static const uint32_t mouse_layer_positions[] = {28, 33, 34, 35};

// クリックキー（離し時に猶予タイマーを起動するキー）のposition番号
// &mo 5 (28) は除外する
static const uint32_t click_positions[] = {33, 34, 35};

static struct k_work_delayable click_release_deactivate_work;

static bool is_mouse_layer_position(uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(mouse_layer_positions); i++) {
        if (mouse_layer_positions[i] == position) {
            return true;
        }
    }
    return false;
}

static bool is_click_position(uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(click_positions); i++) {
        if (click_positions[i] == position) {
            return true;
        }
    }
    return false;
}

static void click_release_deactivate(struct k_work *work) {
    if (zmk_keymap_layer_active(AUTO_MOUSE_LAYER)) {
        LOG_DBG("Click release timer fired - deactivating layer %d", AUTO_MOUSE_LAYER);
        zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
    }
}

static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (!zmk_keymap_layer_active(AUTO_MOUSE_LAYER)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->state) {
        // キー押下時
        if (is_mouse_layer_position(ev->position)) {
            // クリックキーまたは &mo 5: layer 4を維持
            // クリックキーの場合は猶予タイマーをキャンセル（ダブルクリック対応）
            if (is_click_position(ev->position)) {
                LOG_DBG("Click pressed at position %d - cancelling deactivate timer", ev->position);
                k_work_cancel_delayable(&click_release_deactivate_work);
            }
        } else {
            // 通常キー: 即座にlayer 4解除
            LOG_DBG("Key pressed at position %d - deactivating layer immediately", ev->position);
            k_work_cancel_delayable(&click_release_deactivate_work);
            zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
        }
    } else {
        // キー離し時
        if (is_click_position(ev->position)) {
            // クリックキー離し: 猶予タイマーを起動
            LOG_DBG("Click released at position %d - scheduling deactivate in %dms",
                    ev->position, CLICK_RELEASE_DELAY_MS);
            k_work_reschedule(&click_release_deactivate_work,
                              K_MSEC(CLICK_RELEASE_DELAY_MS));
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_position, position_state_changed_listener);
ZMK_SUBSCRIPTION(auto_mouse_position, zmk_position_state_changed);

// トラックボール入力時に猶予タイマーをキャンセル（ダブルクリック対応）
static void trackball_input_callback(struct input_event *evt) {
    if (!zmk_keymap_layer_active(AUTO_MOUSE_LAYER)) {
        return;
    }
    if (evt->type == INPUT_EV_REL && evt->sync) {
        // トラックボール操作: 猶予タイマーをキャンセルしてlayer 4を維持
        if (k_work_delayable_is_pending(&click_release_deactivate_work)) {
            LOG_DBG("Trackball moved during click release - cancelling deactivate timer");
            k_work_cancel_delayable(&click_release_deactivate_work);
        }
    }
}

static const struct device *const trackball_dev =
    DEVICE_DT_GET_OR_NULL(DT_NODELABEL(trackball));

INPUT_CALLBACK_DEFINE(trackball_dev, trackball_input_callback);

static int auto_mouse_layer_init(void) {
    k_work_init_delayable(&click_release_deactivate_work, click_release_deactivate);
    LOG_INF("Auto mouse layer initialized (layer=%d, click_release_delay=%dms)",
            AUTO_MOUSE_LAYER, CLICK_RELEASE_DELAY_MS);
    return 0;
}

SYS_INIT(auto_mouse_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
