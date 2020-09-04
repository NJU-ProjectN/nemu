void ui_mainloop();
void init_device();

void engine_start() {
  /* Initialize devices. */
  init_device();

  /* Receive commands from user. */
  ui_mainloop();
}
