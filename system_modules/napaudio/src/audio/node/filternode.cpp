/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "filternode.h"

#include <audio/core/audionodemanager.h>
#include <mathutils.h>
#include <nap/logger.h>

RTTI_BEGIN_ENUM(nap::audio::FilterNode::EMode)
	RTTI_ENUM_VALUE(nap::audio::FilterNode::EMode::LowPass, "LowPass"),
	RTTI_ENUM_VALUE(nap::audio::FilterNode::EMode::HighPass, "HighPass"),
	RTTI_ENUM_VALUE(nap::audio::FilterNode::EMode::BandPass, "BandPass"),
	RTTI_ENUM_VALUE(nap::audio::FilterNode::EMode::LowRes, "LowRes"),
	RTTI_ENUM_VALUE(nap::audio::FilterNode::EMode::HighRes, "HighRes")
RTTI_END_ENUM

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::FilterNode)
	RTTI_PROPERTY("audioInput", &nap::audio::FilterNode::audioInput, nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("audioOutput", &nap::audio::FilterNode::audioOutput, nap::rtti::EPropertyMetaData::Embedded)
	RTTI_FUNCTION("setMode", &nap::audio::FilterNode::setMode)
	RTTI_FUNCTION("setFrequency", &nap::audio::FilterNode::setFrequency)
	RTTI_FUNCTION("setResonance", &nap::audio::FilterNode::setResonance)
	RTTI_FUNCTION("setBand", &nap::audio::FilterNode::setBand)
	RTTI_FUNCTION("setGain", &nap::audio::FilterNode::setGain)
	RTTI_FUNCTION("prepare", &nap::audio::FilterNode::prepare)
	RTTI_FUNCTION("getMode", &nap::audio::FilterNode::getMode)
	RTTI_FUNCTION("getFrequency", &nap::audio::FilterNode::getFrequency)
	RTTI_FUNCTION("getResonance", &nap::audio::FilterNode::getResonance)
	RTTI_FUNCTION("getBand", &nap::audio::FilterNode::getBand)
	RTTI_FUNCTION("getGain", &nap::audio::FilterNode::getGain)
RTTI_END_CLASS

namespace nap
{
	namespace audio
	{

		FilterNode::FilterNode(NodeManager &nodeManager): Node(nodeManager), mOutput(8), mInput(8)
		{
			calcCoeffs();
			setSmoothing(mSmoothing);
			b0.reset(b0Dest);
			b1.reset(b1Dest);
			b2.reset(b2Dest);
			a1.reset(a1Dest);
			a2.reset(a2Dest);
		}


		void FilterNode::process()
		{
			if (mIsDirty.check())
				update();

			auto inputBuffer = audioInput.pull();
			auto& outputBuffer = getOutputBuffer(audioOutput);
			
			if (inputBuffer != nullptr) {
				for (auto i = 0; i < outputBuffer.size(); ++i) {
					mInput.write((*inputBuffer)[i]);
					auto temp = b0.getNextValue() * mInput.read(0) + b1.getNextValue() * mInput.read(1) + b2.getNextValue() * mInput.read(2) - a1.getNextValue() * mOutput.read(0) - a2.getNextValue() * mOutput.read(1);
					
					mOutput.write(temp);
					outputBuffer[i] = temp;
				}
			} else {
				// process with 0 input
				for (auto i = 0; i < outputBuffer.size(); ++i) {
					mInput.write(0);
					auto temp = b0.getNextValue() * mInput.read(0) + b1.getNextValue() * mInput.read(1) + b2.getNextValue() * mInput.read(2) - a1.getNextValue() * mOutput.read(0) - a2.getNextValue() * mOutput.read(1);
					
					mOutput.write(temp);
					outputBuffer[i] = temp;
				}
			}
		}


		void FilterNode::sampleRateChanged(float sampleRate)
		{
			setSmoothing(mSmoothing);
		}


		void FilterNode::prepare(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band, ControllerValue gain)
		{
			mMode = mode;
			mFrequency = frequency;
			mResonance = pow(10., -(resonance * 0.1));
			mBand = band;
			mGain = gain;
			mOutput.clear();
			mInput.clear();
			calcCoeffs();
			b0.reset(b0Dest);
			b1.reset(b1Dest);
			b2.reset(b2Dest);
			a1.reset(a1Dest);
			a2.reset(a2Dest);
		}

		
		
		void FilterNode::setMode(EMode mode)
		{
			mMode = mode;
			calcCoeffs();
			mIsDirty.set();
		}
		
		
		void FilterNode::setFrequency(ControllerValue frequency)
		{
			mFrequency = math::max<float>(frequency, 1.f);
			calcCoeffs();
			mIsDirty.set();
		}
		
		
		void FilterNode::setResonance(ControllerValue resonance)
		{
			mResonance = pow(10., -(resonance * 0.1));
			calcCoeffs();
			mIsDirty.set();
		}
		
		
		void FilterNode::setBand(ControllerValue band)
		{
			mBand = band;
			if (mBand <= 0)
				mBand = 1;
			calcCoeffs();
			mIsDirty.set();
		}
		
		
		void FilterNode::setGain(ControllerValue gain)
		{
			mGain = gain;
			calcCoeffs();
			mIsDirty.set();
		}

		void FilterNode::set(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band,
			ControllerValue gain)
		{
			mMode = mode;
			mFrequency = frequency;
			mResonance = pow(10., -(resonance * 0.1));
			mBand = band;
			mGain = gain;
			calcCoeffs();
			mIsDirty.set();
		}


		void FilterNode::setSmoothing(TimeValue smoothing)
		{
			mSmoothing = smoothing;
			int stepCount = smoothing * getNodeManager().getSamplesPerMillisecond();
			b0.setStepCount(stepCount);
			b1.setStepCount(stepCount);
			b2.setStepCount(stepCount);
			a1.setStepCount(stepCount);
			a2.setStepCount(stepCount);
		}


		void FilterNode::update()
		{
			b0.setValue(b0Dest);
			b1.setValue(b1Dest);
			b2.setValue(b2Dest);
			a1.setValue(a1Dest);
			a2.setValue(a2Dest);
		}
		
		
		// The coefficients follow the textbook convention, matching BiquadFilter:
		//
		//          b0 + b1 * z^-1 + b2 * z^-2
		//   H(z) = --------------------------
		//          1  + a1 * z^-1 + a2 * z^-2
		//
		// so b0 / b1 / b2 are the numerator taps and a1 / a2 the denominator taps. The denominator
		// is normalized so that the leading coefficient is 1 (the a0 term the recursion divides by).
		// The input gain mGain sits in the numerator taps only: scaling the feedback taps with the
		// gain as well would move the poles radially and make the filter unstable for gains > 1.
		void FilterNode::calcCoeffs()
		{
			switch (mMode) {
				case EMode::LowPass:
				{
					ControllerValue c, cSqr, den;
					c	 = 1 / tan(math::PI * mFrequency / getSampleRate());
					cSqr = c * c;
					den = 1 + math::SQRT2 * c + cSqr;
					b0Dest = mGain / den;
					b1Dest = 2 * b0Dest;
					b2Dest = b0Dest;
					a1Dest = 2 * (1 - cSqr) / den;
					a2Dest = (1 - math::SQRT2 * c + cSqr) / den;
					break;
				}
				case EMode::HighPass:
				{
					ControllerValue c, cSqr, den;
					c = tan(math::PI * mFrequency / getSampleRate());
					cSqr = c * c;
					den = 1 + math::SQRT2 * c + cSqr;
					b0Dest = mGain / den;
					b1Dest = -2 * b0Dest;
					b2Dest = b0Dest;
					a1Dest = 2 * (cSqr - 1) / den;
					a2Dest = (1 - math::SQRT2 * c + cSqr) / den;
					break;
				}
				case EMode::BandPass:
				{
					ControllerValue c, d, den;
					c = 1 / tan(math::PI * mBand / getSampleRate());
					d = 2 * cos(2 * math::PI * mFrequency / getSampleRate());
					den = 1 + c;
					b0Dest = mGain / den;
					b1Dest = 0;
					b2Dest = -b0Dest;
					a1Dest = -c * d / den;
					a2Dest = (c - 1) / den;
					break;
				}
				case EMode::LowRes:
				{
					ControllerValue c, cSqr, q, den;
					c = 1 / tan(math::PI * mFrequency / getSampleRate());
					cSqr = c * c;
					q = math::SQRT2 * mResonance;
					den = 1 + q * c + cSqr;
					b0Dest = mGain / den;
					b1Dest = 2 * b0Dest;
					b2Dest = b0Dest;
					a1Dest = 2 * (1 - cSqr) / den;
					a2Dest = (1 - q * c + cSqr) / den;
					break;
				}
				case EMode::HighRes:
				{
					ControllerValue c, cSqr, q, den;
					c = tan(math::PI * mFrequency / getSampleRate());
					cSqr = c * c;
					q = math::SQRT2 * mResonance;
					den = 1 + q * c + cSqr;
					b0Dest = mGain / den;
					b1Dest = -2 * b0Dest;
					b2Dest = b0Dest;
					a1Dest = 2 * (cSqr - 1) / den;
					a2Dest = (1 - q * c + cSqr) / den;
					break;
				}
			}
		}
		
		
	}
}
