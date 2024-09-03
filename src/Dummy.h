#ifdef PLATFORMIO_PLATFORM_NATIVE
#ifndef Dummy_H
#define Dummy_H
#endif

class DummyClass {
  public:
    bool suspended = false;
    bool dummyConnected = false;
    bool emitted = false;
    bool begin();
    bool loop();
    bool end();
    bool connect();
    bool disconnect();
    bool connected();
    void reset();
    void emitData();
};
#endif