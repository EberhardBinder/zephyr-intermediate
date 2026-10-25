/*
 * temp_alarm - Error-path unit tests (homework skeleton)
 *
 * Task 2: fill in the stubs according to TEST_SPEC.md. Each stub calls
 * ztest_test_skip(), so the binary builds and runs cleanly until the test is
 * implemented.
 *
 * Run:
 *   west twister -T tests/temp_alarm -p native_sim
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "sensor_fake.h"
#include "temp_alarm.h"

#define DEFAULT_THRESHOLD 30

/* Same before() hook as in test_basic.c: a clean fake and an initialised module. */
static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	sensor_fake_reset();
	sensor_fake_init();

	zassume_ok(temp_alarm_init(&fake_sensor_dev, DEFAULT_THRESHOLD),
		   "precondition: temp_alarm_init must succeed");
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_sensor_errors
 *
 * Failures from the sensor driver, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_sensor_errors, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates)
{
	/* TODO(l8-task2): a failed fetch is reported,
	 * and the module does not go on to read the channel.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #1.
	 */

        struct temp_alarm_status status;

	int ret;

        fake_sensor_sample_fetch_fake.return_val = -EIO;

        ret = sensor_sample_fetch(&fake_sensor_dev);

  	zassert_equal(ret, -EIO, "return value sample_fetch must be %d, got %d",
                      -EIO, ret);

        ret = temp_alarm_check();

  	zassert_equal(ret, -EIO, "return value alarm_check must be %d, got %d",
                      -EIO, ret);

 	zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

  	zassert_equal(status.last_error, -EIO, "last_error must be %d, got %d",
                      -EIO, status.last_error);

  	zassert_equal(fake_sensor_channel_get_fake.call_count, 0, "call_count must be %d, got %d",
                      0, fake_sensor_channel_get_fake.call_count);

}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates)
{
	/* TODO(l8-task2): a failed channel read is reported.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #2.
	 */

        struct temp_alarm_status status;

	int ret;

 	struct sensor_value val;

        fake_sensor_channel_get_fake.return_val = -EIO;

        fake_sensor_channel_get_fake.custom_fake = NULL;

        ret = sensor_channel_get(&fake_sensor_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);

  	zassert_equal(ret, -EIO, "return value channel_get must be %d, got %d",
                      -EIO, ret);

	ret = temp_alarm_check();

  	zassert_equal(ret, -EIO, "return value alarm_check must be %d, got %d",
                      -EIO, ret);

	zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

  	zassert_equal(status.last_error, -EIO, "last_error must be %d, got %d",
                      -EIO, status.last_error);

  	zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "call_count must be %d, got %d",
                      1, fake_sensor_sample_fetch_fake.call_count);

}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover)
{
	/* TODO(l8-task2): the first fetch fails and the second one succeeds.
	 * The module must recover on the second check.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #3.
	 */

	int ret;

        struct temp_alarm_status status;

        fake_sensor_sample_fetch_fake.return_val = -EIO;

        ret = sensor_sample_fetch(&fake_sensor_dev);

  	zassert_equal(ret, -EIO, "return value sample_fetch must be %d, got %d",
                      -EIO, ret);

	ret = temp_alarm_check();

  	zassert_equal(ret, -EIO, "return value alarm_check must be %d, got %d",
                      -EIO, ret);

        fake_sensor_sample_fetch_fake.return_val = 0;

        ret = sensor_sample_fetch(&fake_sensor_dev);

  	zassert_equal(ret, 0, "return value sample_fetch must be %d, got %d",
                      0, ret);

	sensor_fake_set_temperature(20);

	ret = temp_alarm_check();

  	zassert_equal(ret, 0, "return value alarm_check must be %d, got %d",
                      0, ret);

	zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

  	zassert_equal(status.last_temp, 20, "last_temp must be %d, got %d",
                      20, status.last_temp);

  	zassert_equal(status.is_alarming, false, "is_alarming must be %d, got %d",
                      false, status.is_alarming);

  	zassert_equal(status.last_error, -EIO, "last_error must be %d, got %d",
                      -EIO, status.last_error);

}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null)
{
	/* TODO(l8-task2): a NULL status pointer is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #1.
	 */

	int ret;

	ret = temp_alarm_check();

	ret = temp_alarm_get_status(NULL);

  	zassert_equal(ret, -EINVAL, "return value alarm_get_status must be %d, got %d",
                      -EINVAL, ret);

}

ZTEST(temp_alarm_invalid_input, test_init_without_device)
{
	/* TODO(l8-task2): init without a sensor device is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #2.
	 */

	int ret;

	ret = temp_alarm_init(NULL, 30);

  	zassert_equal(ret, -ENODEV, "return value alarm_init must be %d, got %d",
                      -ENODEV, ret);

}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range)
{
	/* TODO(l8-task2): init rejects thresholds outside the allowed range.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #3.
	 */

	int ret;

	ret = temp_alarm_init(&fake_sensor_dev, -1);

  	zassert_equal(ret, -EINVAL, "return value alarm_init must be %d, got %d",
                      -EINVAL, ret);

	ret = temp_alarm_init(&fake_sensor_dev, 101);

  	zassert_equal(ret, -EINVAL, "return value alarm_init must be %d, got %d",
                      -EINVAL, ret);

}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range)
{
	/* TODO(l8-task2): set_threshold rejects values outside the allowed range,
	 * and keeps the old threshold.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #4.
	 */

	int ret;

	ret = temp_alarm_set_threshold(-1);

  	zassert_equal(ret, -EINVAL, "return value alarm_set_threshold must be %d, got %d",
                      -EINVAL, ret);

	ret = temp_alarm_set_threshold(101);

  	zassert_equal(ret, -EINVAL, "return value alarm_set_threshold must be %d, got %d",
                      -EINVAL, ret);

}
