#include <unity.h>

#include "multi-state-utils/ImageTransfers/openMVReceiver.h"
#include "multi-state-utils/ImageTransfers/openMVReceiver.cpp"

static bool feedReceiverChunk(openMVReceiver& receiver, const String& input)
{
    int inputLength = 0;
    receiver.testInput(input, inputLength);

    TEST_ASSERT_EQUAL(input.length(), inputLength);

    return receiver.runReceiver();
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_open_mv_receiver_queues_and_returns_images_in_order(void)
{
    openMVReceiver receiver;

    const String imageOne = "image_01: nose cone visible";
    const String imageTwo = "image_02: payload bay centered";
    const String imageThree = "image_03: parachute bundle seen";

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "IMG_BEGIN"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, imageOne));
    TEST_ASSERT_TRUE(feedReceiverChunk(receiver, "IMG_END"));

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "IMG_BEGIN"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, imageTwo));
    TEST_ASSERT_TRUE(feedReceiverChunk(receiver, "IMG_END"));

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "IMG_BEGIN"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, imageThree));
    TEST_ASSERT_TRUE(feedReceiverChunk(receiver, "IMG_END"));

    TEST_ASSERT_EQUAL_UINT8(3, receiver.queueSize());

    String receivedImage = "";
    int receivedByteCount = 0;

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageOne.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageOne.length(), receivedByteCount);

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageTwo.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageTwo.length(), receivedByteCount);

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageThree.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageThree.length(), receivedByteCount);

    TEST_ASSERT_EQUAL_UINT8(0, receiver.queueSize());
    TEST_ASSERT_FALSE(receiver.getImage(receivedImage, receivedByteCount));
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_open_mv_receiver_queues_and_returns_images_in_order);
    return UNITY_END();
}
