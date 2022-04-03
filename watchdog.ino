#ifdef WATCHDOG_ENABLED
void setup_watchdog() {
  ESP.wdtEnable(WDTO_8S);
}
void loop_watchdog() {
  ESP.wdtFeed();
}
#endif