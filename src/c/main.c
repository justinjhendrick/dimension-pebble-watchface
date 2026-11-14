#include <pebble.h>

#define BUFFER_LEN (40)

static Window* s_window;
static Layer* s_layer;
static GColor s_col1 = GColorMediumSpringGreen;
static GColor s_col2 = GColorVividCerulean;

static void debug_bbox(GContext* ctx, GRect bbox) {
#ifdef DEBUG_BBOX
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_draw_rect(ctx, bbox);
#endif
}

static GRect rect_from_midpoint(GPoint midpoint, GSize size) {
  GRect ret;
  ret.origin.x = midpoint.x - size.w / 2;
  ret.origin.y = midpoint.y - size.h / 2;
  ret.size = size;
  return ret;
}

static int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

static int max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
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
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, GColorPastelYellow);
  graphics_fill_rect(ctx, bbox, 0, GCornerNone);
  graphics_draw_rect(ctx, bbox);

  int sz = min(bbox.size.h, bbox.size.w);
  int R = sz / 4;
  int D = 2 * R;
  int sw = max(1, sz * 7 / 100);
  int dot_radius = max(1, sw - 1);

  graphics_context_set_stroke_width(ctx, sw);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  GPoint center = grect_center_point(&bbox);
  GRect  four      = rect_from_midpoint(center, GSize(D, D));
  GPoint two_start = GPoint(four.origin.x               + 1, four.origin.y + four.size.h - 1);
  GPoint two_end   = GPoint(four.origin.x + four.size.w - 1, four.origin.y               + 1);
  GRect  eight_tr  = rect_from_midpoint(GPoint(center.x + R / 2, center.y - R / 2), GSize(R, R));
  GRect  eight_bl  = rect_from_midpoint(GPoint(center.x - R / 2, center.y + R / 2), GSize(R, R));
  switch (digit) {
    case 1: {
      graphics_fill_circle(ctx, center, dot_radius);
      break;
    } case 2: {
      graphics_draw_line(ctx, two_start, two_end);
      break;
    } case 3: {
      graphics_draw_line(ctx, two_start, two_end);
      GPoint dot = GPoint(four.origin.x + four.size.w * 14 / 16, four.origin.y + four.size.h * 14 / 16);
      graphics_fill_circle(ctx, dot, dot_radius);
      break;
    } case 4: {
      graphics_draw_rect(ctx, four);
      break;
    } case 5: {
      graphics_draw_rect(ctx, four);
      graphics_fill_circle(ctx, center, dot_radius);
      break;
    } case 6: {
      graphics_draw_rect(ctx, four);
      graphics_draw_line(ctx, two_start, two_end);
      break;
    } case 7: {
      graphics_draw_rect(ctx, four);
      graphics_draw_line(ctx, two_start, two_end);
      GPoint dot = GPoint(four.origin.x + four.size.w * 23 / 32, four.origin.y + four.size.h * 23 / 32);
      graphics_fill_circle(ctx, dot, dot_radius);
      break;
    } case 8: {
      graphics_draw_rect(ctx, eight_tr);
      graphics_draw_rect(ctx, eight_bl);
      break;
    } case 9: {
      graphics_draw_rect(ctx, eight_tr);
      graphics_draw_rect(ctx, eight_bl);
      GPoint dot = grect_center_point(&eight_bl);
      graphics_fill_circle(ctx, dot, max(1, dot_radius - 1));
      break;
    }
  }
}

static void draw_time(GContext* ctx, GRect bbox, struct tm* now) {
  int x = bbox.origin.x;
  int y = bbox.origin.y;
  int w = bbox.size.w / 2;
  int h = bbox.size.h / 2;
  GRect tl = GRect(x    , y,     w, h);
  GRect tr = GRect(x + w, y,     w, h);
  GRect bl = GRect(x    , y + h, w, h);
  GRect br = GRect(x + w, y + h, w, h);
  debug_bbox(ctx, tl);
  debug_bbox(ctx, tr);
  debug_bbox(ctx, bl);
  debug_bbox(ctx, br);

  int hour = now->tm_hour;
  int minute = now->tm_min;
  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  graphics_context_set_fill_color(ctx, s_col1);
  draw_digit(ctx, tl, hour / 10);
  graphics_context_set_fill_color(ctx, s_col2);
  draw_digit(ctx, tr, hour % 10);
  graphics_context_set_fill_color(ctx, s_col2);
  draw_digit(ctx, bl, minute / 10);
  graphics_context_set_fill_color(ctx, s_col1);
  draw_digit(ctx, br, minute % 10);
}

#ifdef DEBUG_CHEATSHEET

static void draw_cheatsheet(GContext* ctx, GRect main) {
  char t[BUFFER_LEN];
  graphics_context_set_text_color(ctx, GColorBlack);
  GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  int R = min(main.size.h / 4, main.size.w / 3);
  int g = (main.size.w - R * 3) / 4;

  graphics_context_set_fill_color(ctx, s_col1);
  GRect bbox = GRect(main.origin.x + g, main.origin.y, R, R - 2); draw_digit(ctx, bbox, 0); snprintf(t, BUFFER_LEN, " %d", 0); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col2);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 1); snprintf(t, BUFFER_LEN, " %d", 1); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col1);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 2); snprintf(t, BUFFER_LEN, " %d", 2); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col2);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 3); snprintf(t, BUFFER_LEN, " %d", 3); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col1);
  bbox.origin.x += R + g; bbox.origin.y = main.origin.y         ; draw_digit(ctx, bbox, 4); snprintf(t, BUFFER_LEN, " %d", 4); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col2);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 5); snprintf(t, BUFFER_LEN, " %d", 5); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col1);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 6); snprintf(t, BUFFER_LEN, " %d", 6); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col2);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 7); snprintf(t, BUFFER_LEN, " %d", 7); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col1);
  bbox.origin.x += R + g; bbox.origin.y = main.origin.y         ; draw_digit(ctx, bbox, 8); snprintf(t, BUFFER_LEN, " %d", 8); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  graphics_context_set_fill_color(ctx, s_col2);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 9); snprintf(t, BUFFER_LEN, " %d", 9); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
}

#else

static void draw_date(GContext* ctx, GRect bounds, int date_height, struct tm* now) {
  if (date_height < 10) {
    return;
  }
  GRect date_bbox = GRect(
    bounds.origin.x,
    bounds.origin.y + bounds.size.h - date_height,
    bounds.size.w,
    date_height
  );
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, date_bbox, 0, GCornerNone);
  char t[BUFFER_LEN];
  if (clock_is_24h_style()) {
    strftime(t, BUFFER_LEN, "%H:%M, %a, %d %b", now);
  } else {
    strftime(t, BUFFER_LEN, "%l:%M, %a, %d %b", now);
  }
  graphics_context_set_text_color(ctx, GColorPastelYellow);
  draw_text(ctx, t, fonts_get_system_font(FONT_KEY_GOTHIC_28), date_bbox, GTextAlignmentCenter, 6);
}

#endif

static void update_layer(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int inset = PBL_IF_ROUND_ELSE(22, 0);
  GRect main = grect_inset(bounds, GEdgeInsets(inset));

#ifdef DEBUG_CHEATSHEET
  draw_cheatsheet(ctx, main);
#else
  int date_height = PBL_IF_ROUND_ELSE(0, 28);
  main = GRect(main.origin.x, main.origin.y, main.size.w, main.size.h - date_height);
  time_t t = time(NULL);

#ifdef DEBUG_TIME
  static int extra = 0;
  extra += 59;
  t += extra;
#endif

  struct tm* now = localtime(&t);
#ifdef HOUR_OVERRIDE
  now->tm_hour = HOUR_OVERRIDE;
#endif
#ifdef MINUTE_OVERRIDE
  now->tm_min = MINUTE_OVERRIDE;
#endif

  draw_time(ctx, main, now);
  draw_date(ctx, bounds, date_height, now);
#endif
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
  setlocale(LC_ALL, ""); // changes strftime behavior
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
#ifdef DEBUG_TIME
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
#else
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
#endif
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
