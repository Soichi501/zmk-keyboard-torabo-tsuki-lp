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

// スクロールレイヤー番号
#define SCROLL_LAYER 5

// トラックボール操作後、何も入力がない場合に元レイヤーへ戻るまでの時間 (ms)
#define AUTO_MOUSE_TIMEOUT_MS 1000

// スクロール感度の分母（大きいほど遅くなる）
#define SCROLL_DIVISOR 4

// オートマウスレイヤー中に押してもレイヤーを解除しないキーのposition番号
static const uint32_t mouse_layer_positions[] = {28, 33, 34, 35};

static struct k_work_delayable auto_mouse_deactivate_work;
static bool auto_mouse_active = false;

// スクロール用の端数累積バッファ
static int scroll_x_remainder = 0;
static int scroll_y_remainder = 0;

static bool is_mouse_layer_position(uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(mouse_layer_positions); i++) {
        if (mouse_layer_positions[i] == position) {
            return true;
        }
    }
    return false;
}

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
    k_work_reschedule(&auto_mouse_deactivate_work, K_MSEC(AUTO_MOUSE_TIMEOUT_MS));
}

static void trackball_input_callback(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);

    if (zmk_keymap_layer_active(SCROLL_LAYER)) {
        if (evt->type == INPUT_EV_REL) {
            if (evt->code == INPUT_REL_X) {
                scroll_x_remainder += evt->value;
                int scroll_val = scroll_x_remainder / SCROLL_DIVISOR;
                scroll_x_remainder %= SCROLL_DIVISOR;
                if (scroll_val != 0) {
                    input_report_rel(evt->dev, INPUT_REL_HWHEEL, -scroll_val, false, K_NO_WAIT);
                }
                // Zephyrのiterable sectionは名前順(SORT_BY_NAME)で実行される
                // コールバック名を "_aaa_" で始めることで input_listener.c より先に実行され
                // ここでの evt->value = 0 がポインタ移動を確実に抑制する
                evt->value = 0;
            } else if (evt->code == INPUT_REL_Y) {
                scroll_y_remainder += evt->value;
                int scroll_val = scroll_y_remainder / SCROLL_DIVISOR;
                scroll_y_remainder %= SCROLL_DIVISOR;
                if (scroll_val != 0) {
                    input_report_rel(evt->dev, INPUT_REL_WHEEL, -scroll_val, true, K_NO_WAIT);
                }
                evt->value = 0;
            }
        }
        return;
    }

    // スクロールレイヤーを抜けたら端数をリセット
    scroll_x_remainder = 0;
    scroll_y_remainder = 0;

    activate_auto_mouse_layer();
}

// "_aaa_" で始まる名前を使うことでアルファベット順で先頭に来るようにする
// Zephyrのiterable sectionはSORTED_BY_NAMEのため、名前が早いほど先に実行される
INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET_OR_NULL(DT_NODELABEL(trackball)),
                            trackball_input_callback, NULL,
                            _aaa_auto_mouse_trackball);

static int position_state_changed_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (!ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (auto_mouse_active) {
        if (is_mouse_layer_position(ev->position)) {
            LOG_DBG("Mouse layer key at position %d - keeping auto mouse layer", ev->position);
            return ZMK_EV_EVENT_BUBBLE;
        }

        LOG_DBG("Key pressed at position %d - deactivating auto mouse layer", ev->position);
        k_work_cancel_delayable(&auto_mouse_deactivate_work);
        zmk_keymap_layer_deactivate(AUTO_MOUSE_LAYER);
        auto_mouse_active = false;
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_position, position_state_changed_listener);
ZMK_SUBSCRIPTION(auto_mouse_position, zmk_position_state_changed);

static int auto_mouse_layer_init(void) {
    LOG_INF("Auto mouse layer initialized (layer=%d, scroll_layer=%d, timeout=%dms)",
            AUTO_MOUSE_LAYER, SCROLL_LAYER, AUTO_MOUSE_TIMEOUT_MS);
    k_work_init_delayable(&auto_mouse_deactivate_work, deactivate_auto_mouse_layer);
    return 0;
}

SYS_INIT(auto_mouse_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
