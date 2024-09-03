#include <unity.h>
#include "Dummy.h"

DummyClass dummy;

void setUp(void)
{
  // set stuff up here
  if (!dummy.begin())
  {
    TEST_FAIL_MESSAGE("Can't begin");
  }
}

void tearDown(void)
{
  // clean stuff up here
  if (!dummy.end())
  {
    TEST_FAIL_MESSAGE("Can't end");
  }
}

void test_true(void)
{
  TEST_ASSERT_TRUE(true);
}

void test_connect(void)
{
  dummy.connect();
  TEST_ASSERT_TRUE(dummy.connected());
  dummy.disconnect();
  TEST_ASSERT_FALSE(dummy.connected());
}

void test_reset(void)
{
  dummy.connect();
  TEST_ASSERT_TRUE(dummy.connected());
  dummy.reset();
  TEST_ASSERT_TRUE(dummy.connected());
}

void test_loop(void)
{
  TEST_ASSERT_FALSE(dummy.emitted);
  dummy.loop();
  TEST_ASSERT_TRUE(dummy.emitted);
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_true);
  RUN_TEST(test_connect);
  RUN_TEST(test_reset);
  RUN_TEST(test_loop);
  return UNITY_END();
}

int main(void)
{
  return runUnityTests();
}