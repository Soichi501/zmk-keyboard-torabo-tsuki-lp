#include <zmk/event_manager.h>
#include <zmk/events/pointing_event.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>

#define AUTO_MOUSE_LAYER 4

static bool auto_mouse_active = false;

// トラックボール動いたらレイヤーON
static int handle_pointing_event(const zmk_event_t *eh) {
    const struct zmk_pointing_event *ev = as_zmk_pointing_event(eh);

    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // 移動 or スクロールがあれば発火
    if (ev->x != 0 || ev->y != 0 || ev->scroll_x != 0 || ev->scroll_y != 0) {
        if (!auto_mouse_active) {
            zmk_keymap_layer_on(AUTO_MOUSE_LAYER);
            auto_mouse_active = true;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

// キー押したらレイヤーOFF
static int handle_key_event(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->state) { // キー押下時
        if (auto_mouse_active) {
            zmk_keymap_layer_off(AUTO_MOUSE_LAYER);
            auto_mouse_active = false;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_layer, NULL);

ZMK_SUBSCRIPTION(auto_mouse_layer, zmk_pointing_event);
ZMK_SUBSCRIPTION(auto_mouse_layer, zmk_keycode_state_changed);
