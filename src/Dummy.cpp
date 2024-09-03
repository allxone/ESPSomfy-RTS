#ifdef PLATFORMIO_PLATFORM_NATIVE
#include "Dummy.h"

bool DummyClass::begin()
{
  this->suspended = false;
  return true;
}
bool DummyClass::end()
{
  this->suspended = true;
  this->disconnect();
  return true;
}
void DummyClass::reset()
{
  this->disconnect();
  this->connect();
}
bool DummyClass::loop()
{
  if (!this->suspended)
  {
    if (!this->connected())
      this->connect();
  }

  if (!this->suspended)
  {
    if (this->connected())
    {
      this->emitData();
    }
  }
  return true;
}

bool DummyClass::connect()
{
  this->dummyConnected = true;
  return true;
}
bool DummyClass::disconnect()
{
  this->dummyConnected = false;
  return true;
}
bool DummyClass::connected()
{
  return this->dummyConnected;
}

void DummyClass::emitData()
{
  this->emitted = true;
}

#endif