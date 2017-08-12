void init_monitor(int, char *[]);
void ui_mainloop();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  /* Receive commands from user. */
  ui_mainloop();

  return 0;
}
