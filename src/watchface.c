#include <pebble.h>
#define KEY_TEMP 0
#define KEY_ICON 1
#define ANTIALIASING true

static Window *s_main_window;
static Layer *s_time_circle, *s_batt_circle, *s_health_circle;
static GFont s_font, s_font_small;
static int s_hour, s_minute, battery_percent, buf=12, radial_width=11, step_goal=10000, current_screen=2;
static double step_count;
static char *char_current_temp, *char_battery_percent, *char_current_steps, *char_current_time, *char_current_icon_id;
static TextLayer *s_current_screen;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;

////////////////////////////////////////////////////////////////////
// writes current user info to screen                             //
// screen order is weather, steps, time, battery                  //
// pushing up or down will navigate the center to the next screen //
////////////////////////////////////////////////////////////////////
static void update_current_screen(bool up) {
  // if up button
  if(up==true) {
    // handles rolling from 3 to 0, else increment 1
    if(current_screen==3) {
      current_screen=0;
    } else {
      current_screen=current_screen+1;
    }
  } else {
    // handles rolling from 0 to 3, else decrement 1
    if(current_screen==0) {
      current_screen=3;
    } else {
      current_screen=current_screen-1;
    }       
  }
  
  // destroy bitmap if it exists
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }
  
  // handle current screen with images and text
  switch(current_screen) {
    case 0:
      // load temp
      // handles 10 different images
      text_layer_set_font(s_current_screen, s_font);
      text_layer_set_text(s_current_screen, char_current_temp);
      if(strcmp(char_current_icon_id, "01d")==0) {
        // clear sky (day)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "01n")==0) {
        // clear sky (night)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "02d")==0) {
        // partly cloudy (day)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "02n")==0) {
        // partly cloudy (night)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "03d")==0 || 
                strcmp(char_current_icon_id, "03n")==0) {
        // cloudy (both)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "04d")==0) {
        // partly cloudy (day)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "04n")==0) {
        // partly cloudy (night)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "09d")==0 || 
                strcmp(char_current_icon_id, "09n")==0 || 
                strcmp(char_current_icon_id, "10d")==0 || 
                strcmp(char_current_icon_id, "10n")==0 || 
                strcmp(char_current_icon_id, "11d")==0|| 
                strcmp(char_current_icon_id, "11n")==0) {
        // rain (both)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "13d")==0 || 
                strcmp(char_current_icon_id, "13n")==0) {
        // snow (both)
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_WHITE_ICON);
      } else if(strcmp(char_current_icon_id, "50d")==0 || 
                strcmp(char_current_icon_id, "50n")==0) {
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_WHITE_ICON);
      }
    break;
    case 1:
      // load steps
      text_layer_set_font(s_current_screen, s_font_small);
      text_layer_set_text(s_current_screen, char_current_steps);
      s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SHOE_WHITE_ICON);
    break;
    case 2:
      // load time
      text_layer_set_font(s_current_screen, s_font);
      text_layer_set_text(s_current_screen, char_current_time);
      s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOCK_WHITE_ICON);
    break;
    case 3:
      // load battery
      text_layer_set_font(s_current_screen, s_font);
      text_layer_set_text(s_current_screen, char_battery_percent);
      s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_WHITE_ICON);
    break;
  }
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap); 
}

//////////////////
// select click //
//////////////////
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // does nothing
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select clicked");
}

//////////////
// up click //
//////////////
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_current_screen(true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up clicked");
}

////////////////
// down click //
////////////////
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_current_screen(false);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down clicked");
}

///////////////////
// assign clicks //
///////////////////
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "click_config_provider completed");
}

//////////////////////////////
// draws battery percentage //
//////////////////////////////
static void batt_update_proc(Layer *layer, GContext *ctx) {
  // battery, first circle
  GRect bounds = layer_get_bounds(layer);
  GRect batt_bounds = GRect(bounds.origin.x-(buf/2), 0, bounds.size.w+buf, 168);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_radial(ctx, batt_bounds, GOvalScaleModeFitCircle, radial_width, DEG_TO_TRIGANGLE(360-(battery_percent*3.6)), DEG_TO_TRIGANGLE(360));  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "batt_update_proc completed");
}

////////////////////////
// draws time circles //
////////////////////////
static void time_update_proc(Layer *layer, GContext *ctx) {
  // hours, second circle
  GRect bounds = layer_get_bounds(layer);
  GRect h_bounds = GRect(bounds.origin.x+(buf/2), 0, bounds.size.w-buf, 168);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, h_bounds, GOvalScaleModeFitCircle, radial_width, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(30*s_hour));
  
  // minutes, third circle
  GRect m_bounds = GRect(bounds.origin.x+(buf*1.5), 0, bounds.size.w-(buf*3), 168);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_radial(ctx, m_bounds, GOvalScaleModeFitCircle, radial_width, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(6*s_minute)); 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "time_update_proc completed");
}

/////////////////////////////
// draws health percentage //
/////////////////////////////
static void health_update_proc(Layer *layer, GContext *ctx) {
  // steps, fourth circle
  GRect bounds = layer_get_bounds(layer);
  GRect batt_bounds = GRect(bounds.origin.x+(buf*2.5), 0, bounds.size.w-(buf*5), 168);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, batt_bounds, GOvalScaleModeFitCircle, radial_width, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE((step_count/step_goal)*360));    
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health_update_proc completed");
}

//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack); // default GColorWhite

  // register button clicks
  window_set_click_config_provider(window, click_config_provider);
  
  // create fonts
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEOGREY_18));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEOGREY_12));
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // create time circle
  s_time_circle = layer_create(bounds);
  layer_set_update_proc(s_time_circle, time_update_proc);
  layer_add_child(window_layer, s_time_circle); // add layer to window
  
  // create battery circle
  s_batt_circle = layer_create(bounds);
  layer_set_update_proc(s_batt_circle, batt_update_proc);
  layer_add_child(window_layer, s_batt_circle); // add layer to window
  
  // create health circle
  s_health_circle = layer_create(bounds);
  layer_set_update_proc(s_health_circle, health_update_proc);
  layer_add_child(window_layer, s_health_circle);
  
  // user select screen
  s_current_screen = text_layer_create(GRect(42, 70, 60, 20));
  text_layer_set_background_color(s_current_screen, GColorClear);
  text_layer_set_text_color(s_current_screen, GColorWhite); 
  text_layer_set_text_alignment(s_current_screen, GTextAlignmentCenter);
  text_layer_set_font(s_current_screen, s_font);
  layer_add_child(window_layer, text_layer_get_layer(s_current_screen));  
   
  // create icon layer
  s_bitmap_layer = bitmap_layer_create(GRect(48, 88, 48, 28));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));  
    
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load completed");
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // for digital time
  static char time_buf[16];
  strftime(time_buf, sizeof(time_buf), "%l:%M", tick_time);  
  char_current_time = time_buf;
}

// forces time update on watch
static void time_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  s_hour = tick_time->tm_hour;
  // if 24 hour time and 00:00, change to 12:00
  if(s_hour==0) {
    s_hour=12;
  }
  // if 24 hour time and greater than 12:00, subtract 12 hours
  if(s_hour>12) {
    s_hour = s_hour - 12;
  }
  s_minute = tick_time->tm_min;
  layer_mark_dirty(s_time_circle);
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();  
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "time_handler completed");
}

// registers battery update events
static void battery_handler(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  
  // write to char_battery_percent variable
  static char batt_buf[16];
  snprintf(batt_buf, sizeof(batt_buf), "%d%%", battery_percent);
  char_battery_percent = batt_buf; 
  
  // force update to circle
  layer_mark_dirty(s_batt_circle);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_battery completed");
}

// registers health update events
static void health_handler(HealthEventType event, void *context) {
  if(event==HealthEventMovementUpdate) {
    step_count = (double)health_service_sum_today(HealthMetricStepCount);
    
    // write to char_current_steps variable
    static char health_buf[16];
    snprintf(health_buf, sizeof(health_buf), "%d", (int)step_count);
    char_current_steps = health_buf;
    
    // force update to circle
    layer_mark_dirty(s_health_circle);
    
    APP_LOG(APP_LOG_LEVEL_INFO, "health_handler completed");
  }
}

// unload main window
static void main_window_unload(Window *window) {
  layer_destroy(s_time_circle);
  layer_destroy(s_batt_circle);
  health_service_events_unsubscribe();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload completed");
}

///////////////////////
// for weather calls //
///////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // Store incoming information
  static char temp_buf[8];
  static char temp_layer_buf[32];
  
  static char icon_buf[8];
  static char icon_layer_buf[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMP);
  Tuple *icon_tuple = dict_find(iterator, KEY_ICON);

  // If all data is available, use it
  if(temp_tuple && icon_tuple) {
    
    // temperature
    snprintf(temp_buf, sizeof(temp_buf), "%d°", (int)temp_tuple->value->int32);
    snprintf(temp_layer_buf, sizeof(temp_layer_buf), "%s", temp_buf);
    
    // update current_temp variable
    char_current_temp = temp_layer_buf;
    text_layer_set_text(s_current_screen, temp_layer_buf);
    
    // update icon variable
    snprintf(icon_buf, sizeof(icon_buf), "%s", icon_tuple->value->cstring);
    snprintf(icon_layer_buf, sizeof(icon_layer_buf), "%s", icon_buf);
    char_current_icon_id = icon_layer_buf;
    
    // force current icon for weather
    update_current_screen(true);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  }); 
  window_stack_push(s_main_window, true);
  
  update_time();
    
  // register with second ticks
  tick_timer_service_subscribe(MINUTE_UNIT, time_handler);
  
  // register with Battery State Service
  battery_state_service_subscribe(battery_handler);
  // force initial update
  battery_handler(battery_state_service_peek());   
  
  // subscribe to health events 
  health_service_events_subscribe(health_handler, NULL); 
  // force initial update
  health_handler(HealthEventMovementUpdate, NULL); 
  
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  
  // Open AppMessage for weather callbacks
  const int inbox_size = 32;
  const int outbox_size = 32;
  app_message_open(inbox_size, outbox_size); 
  
  update_current_screen(true);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init completed");
}

void deinit(void) {
  window_destroy(s_main_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit completed");
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}