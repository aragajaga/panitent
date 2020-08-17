#ifndef PANITENT_PENCIL_TOOL_H_
#define PANITENT_PENCIL_TOOL_H_

typedef enum {
  UNKNOWN,
  MOUSE_MOVE,
  LEFT_BUTTON_DOWN,
  LEFT_BUTTON_UP,
  RIGHT_BUTTON_DOWN,
  RIGHT_BUTTON_UP
} callback_id_e;

#define CANVAS_REQUIRED 1

typedef struct _mouse_event {
  int x;
  int y;
} mouse_event_t;

typedef struct _callback_node {
  void* p_function;
} callback_node_t;

typedef struct _pencil {
  HCURSOR cursor;
  BOOL draws; 
  int prev_x;
  int prev_y;
} pencil_t;
extern pencil_t g_pencil;

typedef struct _tool_descriptor 
{
  wchar_t* name;
  uint32_t requirements;
  callback_node_t* callback_table;
  size_t callback_table_size
} tool_descriptor_t;


void pencil_register()
{
  tool_descriptor_t pencil_desc;

  callback_node_t callback_table[] = {
    {TOOL_ACTIVATE, pencil_activate},
    {TOOL_DEACTIVATE, pencil_deactivate},
    {MOUSE_MOVE, pencil_onmousemove},
    {LEFT_BUTTON_DOWN, pencil_onlbuttondown},
    {LEFT_BUTTON_UP, pencil_onlbuttonup},
    {RIGHT_BUTTON_DOWN, pencil_onrbuttondown},
    {RIGHT_BUTTON_UP, pencil_onrbuttonup}
  };

  pencil_desc.name = L"Pencil";
  pencil_desc.requirements = REQUIRE_CANVAS;
  pencil_desc.callback_table = callback_table;
  pencil_desc.callback_table_size = sizeof(callback_table)
      / sizeof(callback_node_t);
}

void pencil_initialize()
{
  g_pencil.cursor = LoadCursorFromFile(L"cursors\\pencil.cur"); 
}

void pencil_activate()
{
  viewport_set_cursor(g_pencil.cursor);
}

void pencil_deactivate()
{
  viewport_reset_cursor();
}

void pencil_onmousemove(mouse_event_t evt)
{
  if (g_pencil.draws)
  {
    canvas_t canvas = g_viewport.document->canvas;

    RECT path_rect = {};
    path_rect.left = g_pencil.prev_x;
    path_rect.top  = g_pencil.prev_y;
    path_rect.right  = evt.x;
    path_rect.bottom = evt.y;

    draw_line(canvas, path_rect);

    g_pencil.prev_x = evt.x;
    g_pencil.prev_y = evt.y;
  }
}

void pencil_onlbuttondown(mouse_event_t evt)
{
  g_pencil.draw = TRUE;
  g_pencil.prev_x = evt.x;
  g_pencil.prev_y = evt.y;
}

void pencil_onlbuttonup(mouse_event_t evt)
{
  if (g_pencil.draws)
  {
    canvas_t canvas = g_viewport.document->canvas;

    RECT path_rect = {};
    path_rect.left = g_pencil.prev_x;
    path_rect.top  = g_pencil.prev_y;
    path_rect.right  = evt.x;
    path_rect.bottom = evt.y;

    draw_line(canvas, path_rect);
  }

  g_pencil.draws = FALSE;
}

void pencil_onrbuttondown(mouse_event_t evt)
{

}

void pencil_onrbuttonup(mouse_event_t evt)
{

}

#endif  /* PANITENT_PENCIL_TOOL_H_ */
