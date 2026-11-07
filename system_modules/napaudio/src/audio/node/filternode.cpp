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
			setSmoothing(mSmoothing);
			calcCoeffs();
		}


		void FilterNode::process()
		{
			auto inputBuffer = audioInput.pull();
			auto& outputBuffer = getOutputBuffer(audioOutput);

			// Recalculate the coefficients once when a discrete parameter changed (mode, band or gain) or
			// when the cutoff frequency or resonance started ramping towards a new value.
			if (mIsDirty.check())
				calcCoeffs();

			bool ramping = false;
			for (auto i = 0; i < outputBuffer.size(); ++i)
			{
				// While ramping, recalculate the coefficients from the smoothed parameter values every
				// mCoefficientsUpdateInterval samples, so that the filter follows the sweep without artifacts.
				ramping = mSmoothedFrequency.isRamping() || mSmoothedResonance.isRamping();
				if (ramping)
					calcCoeffs();

				mSmoothedFrequency.getNextValue();
				mSmoothedResonance.getNextValue();

				auto inputSample = inputBuffer != nullptr ? (*inputBuffer)[i] : 0;
				mInput.write(inputSample);
				auto temp = b0 * mInput.read(0) + b1 * mInput.read(1) + b2 * mInput.read(2) - a1 * mOutput.read(0) - a2 * mOutput.read(1);

				mOutput.write(temp);
				outputBuffer[i] = temp;
			}

			// When a ramp finished within this buffer, land the coefficients exactly on the destination values.
			if (ramping && !mSmoothedFrequency.isRamping() && !mSmoothedResonance.isRamping())
				calcCoeffs();
		}


		void FilterNode::sampleRateChanged(float sampleRate)
		{
			setSmoothing(mSmoothing);
			calcCoeffs();
		}


		void FilterNode::prepare(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band, ControllerValue gain)
		{
			mMode = mode;
			mFrequency = frequency;
			mResonance = pow(10., -(resonance * 0.1));
			mBand = band;
			mGain = gain;
			mSmoothedFrequency.reset(mFrequency);
			mSmoothedResonance.reset(mResonance);
			mOutput.clear();
			mInput.clear();
			calcCoeffs();
		}



		void FilterNode::setMode(EMode mode)
		{
			mMode = mode;
			mIsDirty.set();
		}


		void FilterNode::setFrequency(ControllerValue frequency)
		{
			mFrequency = math::max<float>(frequency, 1.f);
			mSmoothedFrequency.setValue(mFrequency);
		}


		void FilterNode::setResonance(ControllerValue resonance)
		{
			mResonance = pow(10., -(resonance * 0.1));
			mSmoothedResonance.setValue(mResonance);
		}


		void FilterNode::setBand(ControllerValue band)
		{
			mBand = band;
			if (mBand <= 0)
				mBand = 1;
			mIsDirty.set();
		}


		void FilterNode::setGain(ControllerValue gain)
		{
			mGain = gain;
			mIsDirty.set();
		}

		void FilterNode::set(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band,
			ControllerValue gain)
		{
			mMode = mode;
			mFrequency = math::max<float>(frequency, 1.f);
			mResonance = pow(10., -(resonance * 0.1));
			mBand = band;
			mGain = gain;
			mSmoothedFrequency.setValue(mFrequency);
			mSmoothedResonance.setValue(mResonance);
			mIsDirty.set();
		}


		void FilterNode::setSmoothing(TimeValue smoothing)
		{
			mSmoothing = smoothing;
			int stepCount = smoothing * getNodeManager().getSamplesPerMillisecond();
			mSmoothedFrequency.setStepCount(stepCount);
			mSmoothedResonance.setStepCount(stepCount);
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
		//
		// The coefficients are calculated from the current values of the smoothed frequency and
		// resonance, which equal their destinations whenever the node is not ramping.
		void FilterNode::calcCoeffs()
		{
			ControllerValue frequency = mSmoothedFrequency.getValue();
			ControllerValue resonance = mSmoothedResonance.getValue();

			switch (mMode) {
				case EMode::LowPass:
				{
					ControllerValue c, cSqr, den;
					c	 = 1 / tan(math::PI * frequency / getSampleRate());
					cSqr = c * c;
					den = 1 + math::SQRT2 * c + cSqr;
					b0 = mGain / den;
					b1 = 2 * b0;
					b2 = b0;
					a1 = 2 * (1 - cSqr) / den;
					a2 = (1 - math::SQRT2 * c + cSqr) / den;
					break;
				}
				case EMode::HighPass:
				{
					ControllerValue c, cSqr, den;
					c = tan(math::PI * frequency / getSampleRate());
					cSqr = c * c;
					den = 1 + math::SQRT2 * c + cSqr;
					b0 = mGain / den;
					b1 = -2 * b0;
					b2 = b0;
					a1 = 2 * (cSqr - 1) / den;
					a2 = (1 - math::SQRT2 * c + cSqr) / den;
					break;
				}
				case EMode::BandPass:
				{
					ControllerValue c, d, den;
					c = 1 / tan(math::PI * mBand / getSampleRate());
					d = 2 * cos(2 * math::PI * frequency / getSampleRate());
					den = 1 + c;
					b0 = mGain / den;
					b1 = 0;
					b2 = -b0;
					a1 = -c * d / den;
					a2 = (c - 1) / den;
					break;
				}
				case EMode::LowRes:
				{
					ControllerValue c, cSqr, q, den;
					c = 1 / tan(math::PI * frequency / getSampleRate());
					cSqr = c * c;
					q = math::SQRT2 * resonance;
					den = 1 + q * c + cSqr;
					b0 = mGain / den;
					b1 = 2 * b0;
					b2 = b0;
					a1 = 2 * (1 - cSqr) / den;
					a2 = (1 - q * c + cSqr) / den;
					break;
				}
				case EMode::HighRes:
				{
					ControllerValue c, cSqr, q, den;
					c = tan(math::PI * frequency / getSampleRate());
					cSqr = c * c;
					q = math::SQRT2 * resonance;
					den = 1 + q * c + cSqr;
					b0 = mGain / den;
					b1 = -2 * b0;
					b2 = b0;
					a1 = 2 * (cSqr - 1) / den;
					a2 = (1 - q * c + cSqr) / den;
					break;
				}
			}
		}


	}
}
