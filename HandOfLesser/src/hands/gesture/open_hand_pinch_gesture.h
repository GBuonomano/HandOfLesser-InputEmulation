#pragma once

#include "base_gesture.h"
#include <HandOfLesserCommon.h>
#include <vector>
#include "above_below_curl_plane_gesture.h"
#include "proximity_gesture.h"


namespace HOL::Gesture::OpenHandPinchGesture
{
	struct Parameters
	{
		HOL::FingerType pinchFinger;
		HOL::HandSide side;
	};

	class Gesture : public BaseGesture::Gesture
	{

	public:
		Gesture() : BaseGesture::Gesture(){};
		static std::shared_ptr<Gesture> Create()
		{
			return std::make_shared<Gesture>();
		}

		void setup();

		OpenHandPinchGesture::Parameters parameters;

	private:

		std::vector<std::shared_ptr<AboveBelowCurlPlaneGesture::Gesture>> mCurlPlaneGestures;
		std::shared_ptr<ProximityGesture> mProxGesture;

	protected:
		float evaluateInternal(GestureData data) override;
	};



}

