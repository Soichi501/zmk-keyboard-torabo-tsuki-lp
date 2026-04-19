#include <zmk/event_manager.h>
#include <zmk/events/mouse_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>

#define AUTO_MOUSE_LAYER 4

static bool auto_mouse_active = false;

static int listener(const zmk_event_t *eh) {

    // マウスイベント（旧ZMK）
    const struct zmk_mouse_state_changed *me = as_zmk_mouse_state_changed(eh);
    if (me) {
        if (me->state.x != 0 || me->state.y != 0 ||
            me->state.wheel_x != 0 || me->state.wheel_y != 0) {

            if (!auto_mouse_active) {
                zmk_keymap_layer_on(AUTO_MOUSE_LAYER);
                auto_mouse_active = true;
            }
        }
        return ZMK_EV_EVENT_BUBBLE;
    }

    // キー入力で戻す
    const struct zmk_keycode_state_changed *ke = as_zmk_keycode_state_changed(eh);
    if (ke && ke->state) {
        if (auto_mouse_active) {
            zmk_keymap_layer_off(AUTO_MOUSE_LAYER);
            auto_mouse_active = false;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(auto_mouse_layer, listener);
ZMK_SUBSCRIPTION(auto_mouse_layer, zmk_mouse_state_changed);
ZMK_SUBSCRIPTION(auto_mouse_layer, zmk_keycode_state_changed);
