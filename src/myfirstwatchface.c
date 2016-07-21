#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_bluepoop;
static int s_battery_level;
static Layer *s_battery_layer;

static void bluetooth_callback(bool connected) {
  // show icon if disconnected
  layer_set_hidden(text_layer_get_layer(s_bluepoop), connected);

  if(!connected) {
    // vibrating alert
    vibes_double_pulse();
  }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // find width of bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 100.0F);

  // draw the background
  graphics_context_set_fill_color(ctx, GColorClear);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // draw the bar
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void battery_callback(BatteryChargeState state) {
  // record the new battery level
  s_battery_level = state.charge_percent;
  
  // update meter
  layer_mark_dirty(s_battery_layer);
}

static void update_time() {
  // get a tm struct
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                            "%H:%M" : "%I:%M", tick_time);
  
  // display time on TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // copy date into buffer from tm struct
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%d / %m", tick_time);
  
  // display date
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // get info about Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // create TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  // formatting for time
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // add it as child layer to Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 120, 144, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorCadetBlue);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // create battery meter Layer
  s_battery_layer = layer_create(GRect(20, 28, 115, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  // create bluepoop TextLayer
  s_bluepoop = text_layer_create(GRect(0, 120, 144, 30));
  text_layer_set_background_color(s_bluepoop, GColorClear);
  text_layer_set_text_color(s_bluepoop, GColorBlack);
  text_layer_set_font(s_bluepoop, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_bluepoop, GTextAlignmentCenter);
  text_layer_set_text(s_bluepoop, "\U0001F4A9");

  // add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bluepoop));
  
  // show the correct state of BT connection from start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  // destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_bluepoop);
  
  // destroy battery layer
  layer_destroy(s_battery_layer);
}

static void init() {
  // create main Window element and assign to pointer
  s_main_window = window_create();
  
  // set handlers to manage elements inside Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show Window on watch, animated=true
  window_stack_push(s_main_window, true);
  
  // register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // make sure time is displayed from start
  update_time();
  
  // register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // make sure battery level is displayed from start
  battery_callback(battery_state_service_peek());

  // register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
}

static void deinit() {
  // destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}