#pragma once

// NAP Includes
#include <nap/component.h>
#include <nap/attribute.h>

#include <napofattributes.h>
#include <napofupdatecomponent.h>

// Of Includes Includes
#include <ofVec3f.h>
#include <ofMatrix4x4.h>
#include <utils/nofUtils.h>

namespace nap
{
	// Forward declares
	class OFService;

	/**
	@brief NAP OpenFrameworks transform
	**/
	class OFTransform : public ServiceableComponent
	{
		RTTI_ENABLE_DERIVED_FROM(Component)
		friend class OFService;

	public:
		// Default constructor
		OFTransform();

		Attribute<ofVec3f>		mTranslate{ this, "Translation", gOrigin };
		Attribute<ofVec3f>		mRotate{ this, "Rotation", gOrigin };
		Attribute<ofVec3f>		mScale{ this, "Scale", {1.0f, 1.0f, 1.0f} };
		Attribute<ofVec3f>		mPivot{ this, "Pivot", gOrigin };

		// Getters
		ofMatrix4x4				getLocalTransform() const;
		const ofMatrix4x4&		getGlobalTransform() const						{ return mGlobalTransform; }

	private:		
		// These are only called by the napofservice
		void					fetchChildTransforms();		//< Populates the list with child xforms
		void					update(const ofMatrix4x4& inParentMatrix);	//< Updates all child transforms

		// List of all child transform nodes
		std::vector<OFTransform*> mChildTransforms;

		// Only accessable by the OFService
		ofMatrix4x4				mGlobalTransform;
	};


	/**
	@brief Rotates a transform based on the speed in axis x y z
	**/
	class OFRotateComponent : public OFUpdatableComponent
	{
		RTTI_ENABLE_DERIVED_FROM(OFUpdatableComponent)
	public:

		// Default constructor
		OFRotateComponent();

		Attribute<float>	mX		{ this, "SpeedX", 0.0f };
		Attribute<float>	mY		{ this, "SpeedY", 0.0f };
		Attribute<float>	mZ		{ this, "SpeedZ", 0.0f };
		Attribute<float>	mSpeed	{ this, "Speed",  1.0f };

		// Overrides
		virtual void onUpdate();

		// Reset
		void resetAxis();

	private:
		// Timer
		float				mPreviousTime;
		float				mTimeX = 0.0f;
		float				mTimeY = 0.0f;
		float				mTimeZ = 0.0f;
	};
}

RTTI_DECLARE(nap::OFTransform)
RTTI_DECLARE(nap::OFRotateComponent)