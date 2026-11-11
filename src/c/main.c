#include <pebble.h>

#define DEBUG_BBOX (false)
#define DEBUG_TIME (false)
#define DEBUG_CHEATSHEET (false)
#define BUFFER_LEN (40)

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

static int min(int a, int b) {
  if (a < b) {
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
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bbox, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  GPoint center = grect_center_point(&bbox);
  int sz = min(bbox.size.h, bbox.size.w);
  int R = sz / 4;
  int D = 2 * R;
  int sw = sz * 7 / 100;
  int dot_radius = sw - 1;
  graphics_context_set_stroke_width(ctx, sw);
  switch (digit) {
    case 0: {
      graphics_draw_circle(ctx, center, R);
      break;
    } case 1: {
      graphics_fill_circle(ctx, center, dot_radius);
      break;
    } case 2: {
      GPoint left = GPoint(center.x - R, center.y);
      GPoint right = GPoint(center.x + R, center.y);
      graphics_draw_line(ctx, left, right);
      break;
    } case 3: {
      int g = R * 3 / 5;
      GPoint u = GPoint(center.x, center.y - g);
      GPoint l = GPoint(center.x - R, center.y + g);
      GPoint r = GPoint(center.x + R, center.y + g);
      graphics_fill_circle(ctx, u, dot_radius);
      graphics_draw_line(ctx, l, r);
      break;
    } case 4: {
      GRect four = rect_from_midpoint(center, GSize(D, D));
      graphics_draw_rect(ctx, four);
      break;
    } case 5: {
      GRect four = rect_from_midpoint(center, GSize(D, D));
      graphics_draw_rect(ctx, four);
      graphics_fill_circle(ctx, center, dot_radius);
      break;
    } case 6: {
      GRect four = rect_from_midpoint(center, GSize(D, D));
      graphics_draw_rect(ctx, four);
      GPoint start = GPoint(four.origin.x               + 1, four.origin.y + four.size.h - 1);
      GPoint end   = GPoint(four.origin.x + four.size.w - 1, four.origin.y               + 1);
      graphics_draw_line(ctx, start, end);
      break;
    } case 7: {
      GRect four = rect_from_midpoint(center, GSize(D, D));
      graphics_draw_rect(ctx, four);
      GPoint start = GPoint(four.origin.x               + 1, four.origin.y + four.size.h - 1);
      GPoint end   = GPoint(four.origin.x + four.size.w - 1, four.origin.y               + 1);
      graphics_draw_line(ctx, start, end);
      GPoint dot = GPoint(four.origin.x + four.size.w * 11 / 16, four.origin.y + four.size.h * 11 / 16);
      graphics_fill_circle(ctx, dot, dot_radius);
      break;
    } case 8: {
      GRect tr = rect_from_midpoint(GPoint(center.x + R / 2, center.y - R / 2), GSize(R, R));
      GRect bl = rect_from_midpoint(GPoint(center.x - R / 2, center.y + R / 2), GSize(R, R));
      graphics_draw_rect(ctx, tr);
      graphics_draw_rect(ctx, bl);
      break;
    } case 9: {
      int half_w = R;
      int half_h = half_w * 173 / 200;  // approx sqrt(3) / 2
      GPoint u = GPoint(center.x, center.y - half_h);
      GPoint l = GPoint(center.x - half_w, center.y + half_h);
      GPoint r = GPoint(center.x + half_w, center.y + half_h);
      graphics_draw_line(ctx, u, l);
      graphics_draw_line(ctx, l, r);
      graphics_draw_line(ctx, r, u);
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
  if (clock_is_24h_style()) {
    draw_digit(ctx, tl, hour / 10);
    draw_digit(ctx, tr, hour % 10);
  } else {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
    if (hour / 10 == 0) {
      draw_digit(ctx, tl, -1); // still want to draw background
    } else {
      draw_digit(ctx, tl, hour / 10);
    }
    draw_digit(ctx, tr, hour % 10);
  }
  int minute = now->tm_min;
  draw_digit(ctx, bl, minute / 10);
  draw_digit(ctx, br, minute % 10);
}

static void draw_cheatsheet(GContext* ctx, GRect main) {
  char t[BUFFER_LEN];
  graphics_context_set_text_color(ctx, GColorBlack);
  GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  int R = min(main.size.h / 4, main.size.w / 3);
  int g = (main.size.w - R * 3) / 4;
  GRect bbox = GRect(main.origin.x + g, main.origin.y, R, R - 2); draw_digit(ctx, bbox, 0); snprintf(t, BUFFER_LEN, "%d", 0); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 1); snprintf(t, BUFFER_LEN, "%d", 1); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 2); snprintf(t, BUFFER_LEN, "%d", 2); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 3); snprintf(t, BUFFER_LEN, "%d", 3); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.x += R + g; bbox.origin.y = main.origin.y         ; draw_digit(ctx, bbox, 4); snprintf(t, BUFFER_LEN, "%d", 4); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 5); snprintf(t, BUFFER_LEN, "%d", 5); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 6); snprintf(t, BUFFER_LEN, "%d", 6); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 7); snprintf(t, BUFFER_LEN, "%d", 7); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.x += R + g; bbox.origin.y = main.origin.y         ; draw_digit(ctx, bbox, 8); snprintf(t, BUFFER_LEN, "%d", 8); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
  bbox.origin.y += R                                            ; draw_digit(ctx, bbox, 9); snprintf(t, BUFFER_LEN, "%d", 9); draw_text(ctx, t, f, bbox, GTextAlignmentLeft, 0);
}

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
  graphics_context_set_text_color(ctx, GColorWhite);
  draw_text(ctx, t, fonts_get_system_font(FONT_KEY_GOTHIC_28), date_bbox, GTextAlignmentCenter, 6);
}

static void update_layer(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  int date_height = PBL_IF_ROUND_ELSE(0, 28);
  int inset = PBL_IF_ROUND_ELSE(30, 0);
  GRect main = GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h - date_height);
  main = grect_inset(main, GEdgeInsets(inset));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  time_t t = time(NULL);
  if (DEBUG_TIME) {
    static time_t extra = 0;
    extra += 59;
    t += extra;
  }

#if DEBUG_CHEATSHEET
  draw_cheatsheet(ctx, bounds);
#else
  struct tm* now = localtime(&t);
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
  char* sys_locale = setlocale(LC_ALL, ""); // changes strftime behavior
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
