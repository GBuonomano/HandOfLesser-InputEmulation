//============ Copyright (c) Valve Corporation, All rights reserved. ============
#include "controller_device_driver.h"

#include "driverlog.h"
#include "vrmath.h"

// Let's create some variables for strings used in getting settings.
// This is the section where all of the settings we want are stored. A section name can be anything,
// but if you want to store driver specific settings, it's best to namespace the section with the
// driver identifier ie "<my_driver>_<section>" to avoid collisions
static const char* my_controller_main_settings_section = "driver_handoflesser";

// Individual right/left hand settings sections
static const char* my_controller_right_settings_section = "driver_handoflesser_left_controller";
static const char* my_controller_left_settings_section = "driver_handoflesser_right_controller";

// These are the keys we want to retrieve the values for in the settings
static const char* my_controller_settings_key_model_number = "controller_model_number";
static const char* my_controller_settings_key_serial_number = "controller_serial_number";

ControllerDeviceDriver::ControllerDeviceDriver(vr::ETrackedControllerRole role)
{
	// Set a member to keep track of whether we've activated yet or not
	is_active_ = false;

	// The constructor takes a role argument, that gives us information about if our controller is a
	// left or right hand. Let's store it for later use. We'll need it.
	my_controller_role_ = role;

	// We have our model number and serial number stored in SteamVR settings. We need to get them
	// and do so here. Other IVRSettings methods (to get int32, floats, bools) return the data,
	// instead of modifying, but strings are different.
	char model_number[1024];
	vr::VRSettings()->GetString(
		my_controller_main_settings_section,
		my_controller_settings_key_model_number,
		model_number,
		sizeof(model_number)
	);
	my_controller_model_number_ = model_number;

	// Get our serial number depending on our "handedness"
	char serial_number[1024];
	vr::VRSettings()->GetString(
		my_controller_role_ == vr::TrackedControllerRole_LeftHand
			? my_controller_left_settings_section
			: my_controller_right_settings_section,
		my_controller_settings_key_serial_number,
		serial_number,
		sizeof(serial_number)
	);
	my_controller_serial_number_ = serial_number;

	// Here's an example of how to use our logging wrapper around IVRDriverLog
	// In SteamVR logs (SteamVR Hamburger Menu > Developer Settings > Web console) drivers have a
	// prefix of
	// "<driver_name>:". You can search this in the top search bar to find the info that you've
	// logged.
	DriverLog("HandOfLesser controller model number: %s", my_controller_model_number_.c_str());
	DriverLog("HandOfLesser controller serial number: %s", my_controller_serial_number_.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver after our
//  IServerTrackedDeviceProvider calls IVRServerDriverHost::TrackedDeviceAdded.
//-----------------------------------------------------------------------------
vr::EVRInitError ControllerDeviceDriver::Activate(uint32_t unObjectId)
{
	// Let's keep track of our device index. It'll be useful later.
	my_controller_index_ = unObjectId;

	auto props = vr::VRProperties();

	// Properties are stored in containers, usually one container per device index. We need to get
	// this container to set The properties we want, so we call this to retrieve a handle to it.
	vr::PropertyContainerHandle_t container
		= props->TrackedDeviceToPropertyContainer(my_controller_index_);

	props->SetStringProperty(
		container, vr::Prop_RenderModelName_String, my_controller_model_number_.c_str()
	);
	// props->SetStringProperty(container, vr::Prop_RenderModelName_String,
	// "C:\\Users\\Roughy\\workspace\\HandOfLesser\\output\\drivers\\handoflesser\\resources\\rendermodels\\MyControllerModelNumber\\MyControllerModelNumber.obj");

	// Let's begin setting up the properties now we've got our container.
	// A list of properties available is contained in vr::ETrackedDeviceProperty.

	// First, let's set the model number.
	props->SetStringProperty(
		container, vr::Prop_ModelNumber_String, my_controller_model_number_.c_str()
	);

	// Let's tell SteamVR our role which we received from the constructor earlier.
	props->SetInt32Property(
		container, vr::Prop_ControllerRoleHint_Int32, my_controller_role_
	);

	// Now let's set up our inputs

	// This tells the UI what to show the user for bindings for this controller,
	// As well as what default bindings should be for legacy apps.
	// Note, we can use the wildcard {<driver_name>} to match the root folder location
	// of our driver.
    props->SetStringProperty(container, vr::Prop_ModelNumber_String, (my_controller_role_ == vr::TrackedControllerRole_LeftHand) ? "Knuckles Left" : "Knuckles Right");
    props->SetStringProperty(container, vr::Prop_RenderModelName_String, (my_controller_role_ == vr::TrackedControllerRole_LeftHand) ? "{indexcontroller}valve_controller_knu_1_0_left" : "{indexcontroller}valve_controller_knu_1_0_right");
    props->SetStringProperty(container, vr::Prop_ResourceRoot_String, "indexcontroller");
    props->SetStringProperty(container, vr::Prop_RegisteredDeviceType_String, (my_controller_role_ == vr::TrackedControllerRole_LeftHand) ? "valve/index_controllerLHR-E217CD00" : "valve/index_controllerLHR-E217CD01");
    props->SetStringProperty(container, vr::Prop_InputProfilePath_String, "{indexcontroller}/input/index_controller_profile.json");
    props->SetStringProperty(container, vr::Prop_ControllerType_String, "knuckles");
	// Let's set up handles for all of our components.
	// Even though these are also defined in our input profile,
	// We need to get handles to them to update the inputs.

	auto input = vr::VRDriverInput();

	// Let's set up our "A" button. We've defined it to have a touch and a click component.
	input->CreateBooleanComponent(
		container, "/input/a/touch", &mInputHandles[InputHandleType::a_touch]
	);
	input->CreateBooleanComponent(
		container, "/input/a/click", &mInputHandles[InputHandleType::a_click]
	);

	// Let's set up our trigger. We've defined it to have a value and click component.

	// CreateScalarComponent requires:
	// EVRScalarType - whether the device can give an absolute position, or just one relative to
	// where it was last. We can do it absolute. EVRScalarUnits - whether the devices has two
	// "sides", like a joystick. This makes the range of valid inputs -1 to 1. Otherwise, it's 0
	// to 1. We only have one "side", so ours is onesided.
	// input->CreateScalarComponent( container, "/input/trigger/value",
	// &mInputHandles[ DeviceInput::trigger_value ], vr::VRScalarType_Absolute,
	// vr::VRScalarUnits_NormalizedOneSided );
	input->CreateBooleanComponent(
		container, "/input/trigger/click", &mInputHandles[InputHandleType::trigger_click]
	);

	input->CreateBooleanComponent(
		container, "/input/system/click", &mInputHandles[InputHandleType::system_click]
	);

	const auto abs = vr::VRScalarType_Absolute;
	const auto norm = vr::VRScalarUnits_NormalizedOneSided;

	input->CreateScalarComponent(
		container, "/input/finger/index", &mInputHandles[InputHandleType::finger_index], abs, norm
	);
	input->CreateScalarComponent(
		container, "/input/finger/middle", &mInputHandles[InputHandleType::finger_middle], abs, norm
	);
	input->CreateScalarComponent(
		container, "/input/finger/ring", &mInputHandles[InputHandleType::finger_ring], abs, norm
	);
	input->CreateScalarComponent(
		container, "/input/finger/pinky", &mInputHandles[InputHandleType::finger_pinky], abs, norm
	);

	const vr::EVRInputError err = input->CreateSkeletonComponent(
		container, // container
		my_controller_role_ == vr::TrackedControllerRole_LeftHand
			? "/input/skeleton/left"
			: "/input/skeleton/right", // The path to the skeleton for input. This is for binding to
									   // applications.
		my_controller_role_ == vr::TrackedControllerRole_LeftHand
			? "/skeleton/hand/left"
			: "/skeleton/hand/right", // Where on the body this skeleton is.
		"/pose/raw", // Where the origin for the skeleton is going to be. These pose locations are
					 // defined in the render model file.
		vr::VRSkeletalTracking_Full, // Inform the runtime about the capabilities of the device.
		nullptr, // Used for calculating curl and splay values. If this is null then defaults are
				 // used.
		0,		 // How many bones there are in the gripLimitTransforms above.
		&mInputHandles[InputHandleType::skeleton] // Bind the component to a handle.
	);

	// initialise our hand tracking simulation class
	my_hand_simulation_ = std::make_unique<MyHandSimulation>();

	// Let's create our haptic component.
	// These are global across the device, and you can only have one per device.
	input->CreateHapticComponent(
		container, "/output/haptic", &mInputHandles[InputHandleType::haptic]
	);

	// Set an member to keep track of whether we've activated yet or not
	is_active_ = true;

	// We've activated everything successfully!
	// Let's tell SteamVR that by saying we don't have any errors.
	return vr::VRInitError_None;
}

//-----------------------------------------------------------------------------
// Purpose: If you're an HMD, this is where you would return an implementation
// of vr::IVRDisplayComponent, vr::IVRVirtualDisplay or vr::IVRDirectModeComponent.
//
// But this a simple example to demo for a controller, so we'll just return nullptr here.
//-----------------------------------------------------------------------------
void* ControllerDeviceDriver::GetComponent(const char* pchComponentNameAndVersion)
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when a debug request has been made from an application to the
// driver. What is in the response and request is up to the application and driver to figure out
// themselves.
//-----------------------------------------------------------------------------
void ControllerDeviceDriver::DebugRequest(
	const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize
)
{
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: This is never called by vrserver in recent OpenVR versions,
// but is useful for giving data to vr::VRServerDriverHost::TrackedDevicePoseUpdated.
//-----------------------------------------------------------------------------
vr::DriverPose_t ControllerDeviceDriver::GetPose()
{
	return this->mLastPose;
}

void ControllerDeviceDriver::UpdatePose(HOL::HandTransformPacket* packet)
{
	// packet data resides in receive buffer and will be replaced on next receive,
	// so make a copy now.
	this->mLastTransformPacket = *packet;

	// Let's retrieve the Hmd pose to base our controller pose off.

	// First, initialize the struct that we'll be submitting to the runtime to tell it we've updated
	// our pose.
	vr::DriverPose_t pose = {0};

	// However, we can also get a predicted pose directly from openxr.
	pose.poseTimeOffset = -0.032f;
	// pose.poseTimeOffset = -0.008f;
	// pose.poseTimeOffset = 0;
	// pose.poseTimeOffset = 0.016f;	// read 16ms in the future from openxr, submit acordingly

	// These need to be set to be valid quaternions. The device won't appear otherwise.
	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;
	// I guess this would be to align coordinate systems if they were offset.
	// Probably won't need that for quest

	// copy our position to our pose
	pose.vecPosition[0] = packet->location.position.x();
	pose.vecPosition[1] = packet->location.position.y();
	pose.vecPosition[2] = packet->location.position.z();

	pose.qRotation.w = packet->location.orientation.w();
	pose.qRotation.x = packet->location.orientation.x();
	pose.qRotation.y = packet->location.orientation.y();
	pose.qRotation.z = packet->location.orientation.z();

	// rotate palm joint position into grip pose orientation
	// pose.qRotation = pose.qRotation * HmdQuaternion_FromEulerAngles(0.f, DEG_TO_RAD(90.f), 0.0);
	// pose.qRotation = pose.qRotation * HmdQuaternion_FromEulerAngles(DEG_TO_RAD(45.f), 0.f, 0.f);

	// pose.qRotation = pose.qRotation * HmdQuaternion_FromEulerAngles(0.0, DEG_TO_RAD(-45.f), 0.f);

	// The two above, but y and z; remember rotations are applied in order.
	// pose.qRotation = pose.qRotation * HmdQuaternion_FromEulerAngles(0.f, DEG_TO_RAD(45.f),
	// DEG_TO_RAD(90.f));

	// I can't test this properly. We're already using other predictions
	float velocityMultiplier = 0.2f;
	// Controllers will vanish if velocities are invalid? not initialized?
	// Velocity numbers are bogus on the quest1 ( pure noise ), but fine on quest3?
	pose.vecVelocity[0] = packet->velocity.linearVelocity.x() * velocityMultiplier;
	pose.vecVelocity[1] = packet->velocity.linearVelocity.y() * velocityMultiplier;
	pose.vecVelocity[2] = packet->velocity.linearVelocity.z() * velocityMultiplier;
	//
	pose.vecAngularVelocity[0] = packet->velocity.angularVelocity.x() * velocityMultiplier;
	pose.vecAngularVelocity[1] = packet->velocity.angularVelocity.y() * velocityMultiplier;
	pose.vecAngularVelocity[2] = packet->velocity.angularVelocity.z() * velocityMultiplier;

	// Acceleration being wrong can make controllers not appear
	pose.vecAcceleration[0] = 0;
	pose.vecAcceleration[1] = 0;
	pose.vecAcceleration[2] = 0;

	pose.vecAngularAcceleration[0] = 0;
	pose.vecAngularAcceleration[1] = 0;
	pose.vecAngularAcceleration[2] = 0;

	// The pose we provided is valid.
	// This should be set is
	pose.poseIsValid = true;

	// Our device is always connected.
	// In reality with physical devices, when they get disconnected,
	// set this to false and icons in SteamVR will be updated to show the device is disconnected
	pose.deviceIsConnected = true;

	// The state of our tracking. For our virtual device, it's always going to be ok,
	// but this can get set differently to inform the runtime about the state of the device's
	// tracking and update the icons to inform the user accordingly.
	pose.result = vr::TrackingResult_Running_OK;

	// Store the pose somewhere
	this->mLastPose = pose;
}

void ControllerDeviceDriver::UpdateInput(HOL::ControllerInputPacket* packet)
{
	this->mLastInputPacket = *packet;
}

void ControllerDeviceDriver::SubmitPose()
{
	if (this->is_active_)
	{
		// UpdateSkeleton(); // TODO: this sends dummy data, needs actual implementation
		// Inform the vrserver that our tracked device's pose has updated, giving it the pose
		// returned by our GetPose().
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
			my_controller_index_, GetPose(), sizeof(vr::DriverPose_t)
		);
	}
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when the device should enter standby mode.
// The device should be put into whatever low power mode it has.
// We don't really have anything to do here, so let's just log something.
//-----------------------------------------------------------------------------
void ControllerDeviceDriver::EnterStandby()
{
	DriverLog(
		"%s hand has been put on standby", vr::TrackedControllerRole_LeftHand ? "Left" : "Right"
	);
}

//-----------------------------------------------------------------------------
// Purpose: This is called by vrserver when the device should deactivate.
// This is typically at the end of a session
// The device should free any resources it has allocated here.
//-----------------------------------------------------------------------------
void ControllerDeviceDriver::Deactivate()
{
	// unassign our controller index (we don't want to be calling vrserver anymore after
	// Deactivate() has been called
	my_controller_index_ = vr::k_unTrackedDeviceIndexInvalid;
}

//-----------------------------------------------------------------------------
// Purpose: This is called by our IServerTrackedDeviceProvider when its RunFrame() method gets
// called. It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
void ControllerDeviceDriver::MyRunFrame()
{
	auto input = vr::VRDriverInput();
	// DriverLog("run frame");
	/*
	//if (this->mLastData.side == XR_HAND_LEFT_EXT)
	{
		DriverLog("Index value: %.2f, click: %d",
			this->mLastData.inputs.trigger,
			this->mLastData.inputs.triggerClick);
	}

	*/
	// update our inputs here
	// input->UpdateScalarComponent(mInputHandles[DeviceInput::trigger_value],
	// this->mLastInputPacket.trigger, 0.0);
	const auto timeOffset = 0.0f;

	input->UpdateBooleanComponent(
		mInputHandles[InputHandleType::trigger_click], this->mLastInputPacket.triggerClick, timeOffset
	);
	input->UpdateBooleanComponent(
		mInputHandles[InputHandleType::system_click], this->mLastInputPacket.systemClick, timeOffset
	);

	auto updateFinger = [&](InputHandleType handle, float fingerCurl)
	{
		input->UpdateScalarComponent(mInputHandles[handle], fingerCurl, timeOffset);
	};

	updateFinger(InputHandleType::finger_index, mLastInputPacket.fingerCurlIndex);
	updateFinger(InputHandleType::finger_middle, mLastInputPacket.fingerCurlMiddle);
	updateFinger(InputHandleType::finger_ring, mLastInputPacket.fingerCurlRing);
	updateFinger(InputHandleType::finger_pinky, mLastInputPacket.fingerCurlPinky);

	// DriverLog(
	// 	"Index value: %.2f, middle: %.2f, ring: %.2f, pinky: %.2f",
	// 	mLastInputPacket.fingerCurlIndex,
	// 	mLastInputPacket.fingerCurlMiddle,
	// 	mLastInputPacket.fingerCurlRing,
	// 	mLastInputPacket.fingerCurlPinky
	// );
}

//-----------------------------------------------------------------------------
// Purpose: This is called by our IServerTrackedDeviceProvider when it pops an event off the event
// queue. It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
void ControllerDeviceDriver::MyProcessEvent(const vr::VREvent_t& vrevent)
{
	switch (vrevent.eventType)
	{
	// Listen for haptic events
	case vr::VREvent_Input_HapticVibration: {
		// We now need to make sure that the event was intended for this device.
		// So let's compare handles of the event and our haptic component
		const auto& haptic = vrevent.data.hapticVibration;
		if (haptic.componentHandle == mInputHandles[InputHandleType::haptic])
		{
			// The event was intended for us!
			// To convert the data to a pulse, see the docs.
			// For this driver, we'll just print the values.

			const float duration = haptic.fDurationSeconds;
			const float frequency = haptic.fFrequency;
			const float amplitude = haptic.fAmplitude;

			DriverLog(
				"Haptic event triggered for %s hand. Duration: %.2f, Frequency: %.2f, Amplitude: "
				"%.2f",
				my_controller_role_ == vr::TrackedControllerRole_LeftHand ? "left" : "right",
				duration,
				frequency,
				amplitude
			);
		}
		break;
	}
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Our IServerTrackedDeviceProvider needs our serial number to add us to vrserver.
// It's not part of the ITrackedDeviceServerDriver interface, we created it ourselves.
//-----------------------------------------------------------------------------
const std::string& ControllerDeviceDriver::MyGetSerialNumber()
{
	return my_controller_serial_number_;
}

void ControllerDeviceDriver::UpdateSkeleton()
{
	if (frame_ >= 4000)
	{
		frame_ = 0;
	}

	const int op = frame_ % 4000;
	// DriverLog("Input thread frame: %d", op);
	if (op < 1000) // curl 0 -> 1
	{
		last_curl_ = last_curl_ + 0.001f;
	}
	else if (op < 2000) // curl 1 -> 0
	{
		last_curl_ = last_curl_ - 0.001f;
	}
	else if (op < 2500) // splay 0 -> 1
	{
		last_splay_ = last_splay_ + 0.002f;
	}
	else if (op < 3500) // splay 1 -> -1
	{
		last_splay_ = last_splay_ - 0.002f;
	}
	else // splay -1 -> 0
	{
		last_splay_ = last_splay_ + 0.002f;
	}

	vr::VRBoneTransform_t transforms[eBone_Count];
	// Pass our calculated curl and splay values to our skeleton simulation model to compute the
	// bone transforms for.
	my_hand_simulation_->ComputeSkeletonTransforms(
		my_controller_role_,
		{last_curl_, last_curl_, last_curl_, last_curl_, last_curl_},
		{last_splay_, last_splay_, last_splay_, last_splay_, last_splay_},
		transforms
	);
	// Update the skeleton components.
	// Applications can choose between using a skeleton as if it's holding a controller, or an
	// interpretation with having it without one. As ours is just a simulation, let's just set
	// them to the same transforms.
	vr::VRDriverInput()->UpdateSkeletonComponent(
		mInputHandles[InputHandleType::skeleton],
		vr::VRSkeletalMotionRange_WithController,
		transforms,
		eBone_Count
	);
	vr::VRDriverInput()->UpdateSkeletonComponent(
		mInputHandles[InputHandleType::skeleton],
		vr::VRSkeletalMotionRange_WithoutController,
		transforms,
		eBone_Count
	);

	frame_++;
}
