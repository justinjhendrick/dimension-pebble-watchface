#include <pebble.h>

#define DEBUG_BBOX (false)
#define DEBUG_TIME (false)
#define BUFFER_LEN (40)
#define ROUND (PBL_IF_ROUND_ELSE(true, false))

static Window* s_window;
static Layer* s_layer;

static void debug_bbox(GContext* ctx, GRect bbox) {
  if (!DEBUG_BBOX) {
    return;
  }
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_draw_rect(ctx, bbox);
}

static GRect rect_from_midpoint(GPoint midpoint, GSize size) {
  GRect ret;
  ret.origin.x = midpoint.x - size.w / 2;
  ret.origin.y = midpoint.y - size.h / 2;
  ret.size = size;
  return ret;
}

static void draw_text(
    GContext* ctx,
    const char* buffer,
    GFont font,
    GRect bbox,
    GTextAlignment align,
    int shift_up
  ) {
  GRect fixed_bbox = GRect(bbox.origin.x, bbox.origin.y - shift_up, bbox.size.w, bbox.size.h);
  graphics_draw_text(ctx, buffer, font, fixed_bbox, GTextOverflowModeWordWrap, align, NULL);
}

static void draw_digit(GContext* ctx, GRect bbox, int digit) {
}

static void draw_time(GContext* ctx, struct tm* now, GPoint center, int radius) {
  GRect full = rect_from_midpoint(center, GSize(radius * 2, radius * 2));
  int x = full.origin.x;
  int y = full.origin.y;
  int w = full.size.w / 2;
  int h = full.size.h / 2;
  int g = 1;
  GRect tl = GRect(x - g    , y,     w, h);
  GRect tr = GRect(x + g + w, y,     w, h);
  GRect bl = GRect(x - g    , y + h, w, h);
  GRect br = GRect(x + g + w, y + h, w, h);
  debug_bbox(ctx, tl);
  debug_bbox(ctx, tr);
  debug_bbox(ctx, bl);
  debug_bbox(ctx, br);

  char t[BUFFER_LEN];

  // hours on top
  int hours_shift_up = 10;
  int hour = now->tm_hour;
  if (clock_is_24h_style()) {
    draw_digit(ctx, tl, hour / 10);
    draw_digit(ctx, tr, hour % 10);
  } else {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
    if (hour / 10 != 0) {
      draw_digit(ctx, tl, hour / 10);
    }
    draw_digit(ctx, tr, hour % 10);
  }

  // minutes on bottom
  int minute = now->tm_min;
  draw_digit(ctx, bl, minute / 10);
  draw_digit(ctx, br, minute % 10);
}

static void draw_date(GContext* ctx, GRect bounds, int date_height, struct tm* now) {
  GRect date_bbox = GRect(
    bounds.origin.x,
    bounds.origin.y + bounds.size.h - date_height,
    bounds.size.w,
    date_height
  );
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, date_bbox, 0, GCornerNone);
  char t[BUFFER_LEN];
  strftime(t, BUFFER_LEN, "%a %d %b", now);
  graphics_context_set_text_color(ctx, GColorWhite);
  draw_text(ctx, t, fonts_get_system_font(FONT_KEY_GOTHIC_28), date_bbox, GTextAlignmentCenter, 6);
}

static void update_layer(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  time_t t = time(NULL);
  if (DEBUG_TIME) {
    static time_t extra = 0;
    extra += 1;
    t += extra;
  }

  struct tm* now = localtime(&t);
  draw_time(ctx, now, grect_center_point(&main), 100);
  if (!ROUND) {
    draw_date(ctx, bounds, 28, now);
  }
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_layer);
  layer_add_child(window_layer, s_layer);
}

static void window_unload(Window* window) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

static void tick_handler(struct tm* now, TimeUnits units_changed) {
  if (s_layer) layer_mark_dirty(s_layer);
}

static void init(void) {
  s_font_lg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_71));
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(DEBUG_TIME ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
