#pragma once

#include "../struct.h"

namespace HOL
{
enum class NativePacketType : __int32
{
	InvalidPacket = 100,
	HandTransform = 500,
	ControllerInput = 600,
};

struct NativePacket
{
	NativePacketType packetType = NativePacketType::InvalidPacket;
};

struct ControllerInputPacket
{
	NativePacketType packetType = NativePacketType::ControllerInput;
	bool valid = 0;
	HOL::HandSide side = HOL::HandSide::LeftHand;
	float trigger = 0.0f;
	bool triggerClick = false;
	bool systemClick = false;
	float fingerCurlIndex = 0.0f;
	float fingerCurlMiddle = 0.0f;
	float fingerCurlRing = 0.0f;
	float fingerCurlPinky = 0.0f;
};

// Mimics vr::DriverPose_t, but only contains the bits we care about.
// Subject to change.
// include packet type in packet itself so we can just memcpy the whole thing
struct HandTransformPacket
{
	NativePacketType packetType = NativePacketType::HandTransform;
	bool valid;
	HOL::HandSide side;
	HOL::PoseLocation location;
	HOL::PoseVelocity velocity;
};

} // namespace HOL
