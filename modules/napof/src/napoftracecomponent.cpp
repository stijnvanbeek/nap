// Local Includes
#include <napoftracecomponent.h>

// OF Includes
#include <napofsplinecomponent.h>
#include <Utils/ofUtils.h>
#include <Utils/nofUtils.h>

// Core Includes
#include <nap/entity.h>
#include <nap/logger.h>

// STD Includes
#include <cmath>

namespace nap
{
	OFTraceComponent::OFTraceComponent()
	{
		// Set first sample time
		mPreviousTime = ofGetElapsedTimef();

		// Connect signals / slots
		mCount.connectToValue(mCountChanged);

		// Create buffer
		UpdateTraceBuffer(mCount.getValue());
	}


	void OFTraceComponent::onDraw()
	{
		// Get time difference and store
		float current_time = ofGetElapsedTimef();
		float diff_time = current_time - mPreviousTime;
		mPreviousTime = current_time;

		// Get the spline component
		OFSplineComponent* spline_component = getParent()->getComponent<OFSplineComponent>();
		if (spline_component == nullptr)
		{
			Logger::warn("No spline component found, can't draw tracer");
			return;
		}

		// Get the spline
		NSpline& spline = spline_component->mSpline.getValueRef();
		if (spline.GetPointCount() < 2)
			return;

		// Update time
		mTime += (diff_time * mSpeed.getValue());
		float sample_loc = fmod(mTime + mOffset.getValueRef(), 1.0f);

		// Get update step over spline
		float step = mLength.getValue() / float(mCount.getValue());

		uint32 trace_count = uint32(mCount.getValue());
		for (uint32 i = 0; i < trace_count; i++)
		{
			float lookup = sample_loc + (float(i) * step);
			lookup = fmod(lookup, 1.0f);
			mTraceSpline.GetVertex(i) = spline.GetPointAtPercent(lookup);

			// Get the index for the color buffers to interpolate
			float f_idx = spline.GetIndexAtPercent(lookup);

			// Get the two colors closest to that point
			uint32 min_point_if = gMin<int>((uint32)f_idx, spline.GetPointCount()-1);
			uint32 max_point_if = min_point_if + 1 >= (uint32)spline.GetPointCount() ? 0 : min_point_if + 1;

			ofFloatColor& min_color = spline.GetColor(min_point_if);
			ofFloatColor& max_color = spline.GetColor(max_point_if);

			// Interpolate
			float lerp_value = f_idx - floor(f_idx);
			gMixFloatColor(min_color, max_color, lerp_value, mTraceSpline.GetColor(i));
		}

		// Upload to gpu
		mTraceSpline.UpdateVBO(NSpline::DataType::VERTEX);

		// Draw trace spline
		ofSetColor(255, 255, 255, 255);
		mTraceSpline.Draw();

		// Draw a dot over time
		ofSetColor(255, 255, 0, 255);
		if (mDrawDot.getValue())
		{
			ofCircle(mTraceSpline.GetVertex(0), 4.0f);
			ofCircle(mTraceSpline.GetVertex(mTraceSpline.GetPointCount() - 1), 4.0f);
		}
	}


	/**
	@brief Updates the trace buffer (NSpline) based on the points in the 
	**/
	void OFTraceComponent::UpdateTraceBuffer(const int& inCount)
	{
		// Create poly line
		ofPolyline line;

		// Create vertices
		std::vector<ofPoint> vertices;
		vertices.resize(inCount);
		line.addVertices(vertices);

		// Set line as buffer, created additional buffers on the fly
		mTraceSpline.SetPolyLine(line);
	}
}

RTTI_DEFINE(nap::OFTraceComponent)