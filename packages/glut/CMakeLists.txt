project(glut)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${X11_INCLUDE_DIR})

set(SOURCES
  glut_8x13.c
  glut_9x15.c
  glut_bitmap.c
  glut_bwidth.c
  glut_cindex.c
  glut_cmap.c
  glut_cursor.c
  glut_dials.c
  glut_dstr.c
  glut_event.c
  glut_ext.c
  glut_fullscrn.c
  glut_gamemode.c
  glut_get.c
  glut_glxext.c
  glut_hel10.c
  glut_hel12.c
  glut_hel18.c
  glut_init.c
  glut_input.c
  glut_joy.c
  glut_key.c
  glut_keyctrl.c
  glut_keyup.c
  glut_menu2.c
  glut_menu.c
  glut_mesa.c
  glut_modifier.c
  glut_mroman.c
  glut_overlay.c
  glut_roman.c
  glut_shapes.c
  glut_space.c
  glut_stroke.c
  glut_swap.c
  glut_swidth.c
  glut_tablet.c
  glut_teapot.c
  glut_tr10.c
  glut_tr24.c
  glut_util.c
  glut_vidresize.c
  glut_warp.c
  glut_win.c
  glut_winmisc.c
  layerutil.c
)

add_library(glut STATIC ${SOURCES})

