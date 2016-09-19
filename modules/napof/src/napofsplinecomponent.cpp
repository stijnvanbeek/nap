#include <nap/entity.h>
#include <nap/logger.h>

#include <assert.h>

#include <napofsplinecomponent.h>
#include <napoftransform.h>
#include <napofattributes.h>

// OF Includes
#include <ofGraphics.h>

// std Includes
#include <fmod.h>

namespace nap
{
	/**
	@brief Constructs the spline from a poly line
	**/
	void OFSplineComponent::SetFromPolyLine(const ofPolyline& inPolyline)
	{
		// Make sure it has enough points
		assert(inPolyline.size() >= 2);

		// Set the new poly line
		mSpline.getValueRef().SetPolyLine(inPolyline);

		// Trigger update
		mSpline.emitValueChanged();
	}



	/**
	@brief Draws the Polyline
	**/
	void OFSplineComponent::onDraw()
	{
		// Set to white
		ofSetColor(gDefaultWhiteColor);

		// Get buffer
		const ofVbo& buffer = mSpline.getValue().GetVertexBuffer();

		// Draw buffer
		mSpline.getValueRef().Draw();
	}


	//////////////////////////////////////////////////////////////////////////


	/**
	@brief Constructor
	**/
	OFSplineSelectionComponent::OFSplineSelectionComponent()
	{
		mSplineType.connectToValue(mTypeChangedSlot);
		mSplineSize.connectToValue(mSizeChangedSlot);
		mSplineCount.connectToValue(mCountChangedSlot);
	}



	/**
	@brief Creates a new spline and sets it on all the spline components
	**/
	void OFSplineSelectionComponent::CreateAndUpdateSpline()
	{
		Entity* parent = getParent();
		assert(parent != nullptr);

		// Get all spline components
		std::vector<OFSplineComponent*> splines;
		parent->getComponentsOfType<OFSplineComponent>(splines);

		// If no splines were found, return
		if (splines.empty())
		{
			ofLogWarning("SplineSelectionComponent") << "No valid spline components found!";
			return;
		}

		// Spline to fill
		NSpline out_spline;

		switch (mSplineType.getValue())
		{
		case SplineType::Circle:
			gCreateCircle(mSplineSize.getValue(), mSplineCount.getValue(),  gOrigin, out_spline);
			break;
		case SplineType::Square:
			gCreateSquare(mSplineSize.getValue(), mSplineCount.getValue(),  gOrigin, out_spline);
			break;
		case SplineType::Triangle:
			gCreateEqualTriangle(mSplineSize.getValue(), mSplineCount.getValue(),  gOrigin, out_spline);
			break;
		case SplineType::Line:
			gCreateLine(mSplineSize.getValue(),   mSplineCount.getValue(),  gOrigin, out_spline);
			break;
		case SplineType::Hexagon:
			gCreateHexagon(mSplineSize.getValue(), mSplineCount.getValue(), gOrigin, out_spline);
			break;
		default:
			assert(false);
			break;
		}

		// Set it
		for (auto& spline : splines)
		{
			spline->mSpline.setValue(out_spline);
		}
	}



	//////////////////////////////////////////////////////////////////////////

	/**
	@brief Color constructor
	**/
	OFSplineColorComponent::OFSplineColorComponent()
	{
		mPreviousTime = ofGetElapsedTimef();
	}


	/**
	@brief Updates the colors for the spline component
	**/
	void OFSplineColorComponent::onUpdate()
	{
		// Calculate time difference
		float current_time = ofGetElapsedTimef();
		float diff_time = current_time - mPreviousTime;
		mPreviousTime = current_time;

		// Update time
		mTime += diff_time * mCycleSpeed.getValue();

		// Get spline component
		Entity* parent = getParent();
		OFSplineComponent* spline_component = parent->getComponent<OFSplineComponent>();
		if (spline_component == nullptr)
		{
			ofLogWarning("SplineColorComponent") << "No valid spline components found!";
			return;
		}

		// Make sure we have some colors
		if (mColors.getValue().size() == 0)
		{
			ofLogWarning("SplineColorComponent") << "No colors specified!";
			return;
		}

		// Get spline
		NSpline& spline = spline_component->mSpline.getValueRef();
		
		// Get color data
		SplineColorData& color_data = spline.GetColorDataRef();
		std::vector<ofFloatColor>& cur_colors = mColors.getValueRef();

		// Get count
		float buffer_count = float(color_data.size());
		float color_count  = float(cur_colors.size());

		// Amount of points to blend color a - b
		float div_length = buffer_count / gMax<int>(color_count-1,1);

		// Buffer offset
		float clamped = gClamp<float>(mOffset.getValueRef(), 0.0f, 1.0f);
		
		// Calculate buffer offset -> first add time, after that absolute offset
		float f_offset = (1.0f - fmod(mTime, 1.0f)) * float(color_data.size());
		uint offset = uint(f_offset + (mOffset.getValue() * float(color_data.size())));

		// Intensity
		float intensity = gClamp<float>(mIntensity.getValue(), 0.0f, 1.0f);

		for (uint i = 0; i < color_data.size(); i++)
		{
			// Get current index
			uint idx = (i + offset) % color_data.size();

			// Get floating point representation
			float i_f = float(idx);

			// Get modulated value
			float mod = fmod(i_f, div_length);

			// Map to range
			float nrange = gFit(mod, 0.0f, div_length, 0.0f, 1.0f);

			// Get min / max in colors
			float lookup = gFit(i_f, 0.0f, buffer_count, 0.0f, float(color_count - 1));

			// Get bounds
			int lookup_min = floor(lookup);
			int lookup_max = ceil(lookup);

			// Set interpolated color data
			color_data[i].r = ofLerp(cur_colors[lookup_min].r, cur_colors[lookup_max].r, nrange) * intensity;
			color_data[i].g = ofLerp(cur_colors[lookup_min].g, cur_colors[lookup_max].g, nrange) * intensity;
			color_data[i].b = ofLerp(cur_colors[lookup_min].b, cur_colors[lookup_max].b, nrange) * intensity;
		}
	}

	
	/**
	@brief Updates the color and vertex buffers
	**/
	void OFSplineUpdateGPUComponent::onUpdate()
	{
		Entity* parent = this->getParent();
		OFSplineComponent* spline_component = parent->getComponent<OFSplineComponent>();
		if (spline_component == nullptr)
		{
			Logger::warn("Can't find spline on entity, unable to update GPU buffers");
			return;
		}

		NSpline& spline = spline_component->mSpline.getValueRef();
		spline.UpdateVBO(NSpline::DataType::COLOR);
		spline.UpdateVBO(NSpline::DataType::VERTEX);
	}
}

// Define components
RTTI_DEFINE(nap::OFSplineComponent)
RTTI_DEFINE(nap::OFSplineSelectionComponent)
RTTI_DEFINE(nap::OFSplineColorComponent)
RTTI_DEFINE(nap::OFSplineUpdateGPUComponent)