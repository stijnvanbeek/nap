/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Std includes
#include <atomic>

// Audio includes
#include <audio/core/audionode.h>
#include <audio/utility/delay.h>
#include <audio/utility/dirtyflag.h>
#include <audio/utility/linearsmoothedvalue.h>

namespace nap
{
	namespace audio
	{

		/**
		 * Multi-purpose DSP filter using the Butterworth filter algorithm to calculate biquad coefficients.
		 * Is able to apply lowpass and highpass with or without variable resonance peak and bandpass filtering.
		 *
		 * Changes of the cutoff frequency and resonance are smoothed over time and the biquad coefficients are
		 * recalculated from the smoothed parameter values every mCoefficientsUpdateInterval samples while either
		 * parameter is ramping. This allows fast cutoff sweeps without artifacts, because every intermediate
		 * coefficient set is a valid filter, unlike interpolation of the coefficients themselves.
		 */
		class NAPAPI FilterNode : public Node
		{
		public:
			enum class EMode
			{
				LowPass, HighPass, BandPass, LowRes, HighRes
			};

		public:
			FilterNode(NodeManager& nodeManager);

			/**
			 * The input to be filtered
			 */
			InputPin audioInput = {this};

			/**
			 * Outputs the filtered signal
			 */
			OutputPin audioOutput = {this};

			/**
			 * Immediately changes the settings of the filter.
			 * Only call this before the filter starts being processed.
			* @param mode lowpass, highpass, bandpass, lowpass with resonance peak or highpass with resonance peak.
			 * @param frequency The frequency parameter of the filter in Hz.
			 * @param resonance Sets the resonance peak of the filter. 0 means no resonance, 30 means self-oscillation.
			 * @param band Sets the bandwith for bandpass filtering in Hz.
			 * @param gain Sets the gaining factor of the filter's output.
			 */
			void prepare(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band, ControllerValue gain);

			/**
			 * Sets the mode of the filter.
			 * @param mode lowpass, highpass, bandpass, lowpass with resonance peak or highpass with resonance peak.
			 */
			void setMode(EMode mode);

			/**
			 * Sets the frequency parameter of the filter in Hz. For the different modes this means:
			 * - Cutoff frequency for highpass and lowpass filters
			 * - Center frequency for bandpass filtering
			 * The frequency ramps towards the new value using the smoothing time set by setSmoothing().
			 */
			void setFrequency(ControllerValue cutoffFrequency);

			/**
			 * Sets the resonance peak of the filter. 0 means no resonance, 30 means self-oscillation.
			 * The resonance ramps towards the new value using the smoothing time set by setSmoothing().
			 */
			void setResonance(ControllerValue resonance);

			/**
			 * Sets the bandwith for bandpass filtering in Hz.
			 */
			void setBand(ControllerValue band);

			/**
			 * Sets the gaining factor of the filter's output.
			 */
			void setGain(ControllerValue gain);

			/**
			 * Sets all the filter parameters at once, recalculating the coefficients only once.
			 * @param mode lowpass, highpass, bandpass, lowpass with resonance peak or highpass with resonance peak.
			 * @param frequency The frequency parameter of the filter in Hz.
			 * @param resonance Sets the resonance peak of the filter. 0 means no resonance, 30 means self-oscillation.
			 * @param band Sets the bandwith for bandpass filtering in Hz.
			 * @param gain Sets the gaining factor of the filter's output.
			 */
			void set(EMode mode, ControllerValue frequency, ControllerValue resonance, ControllerValue band, ControllerValue gain);

			/**
			 * Sets the smoothing time for the cutoff frequency and resonance parameters to fade to their next value.
			 * @param smoothing Time in ms.
			 */
			void setSmoothing(TimeValue smoothing);

			/**
			 * @return the mode of the filter.
			 */
			EMode getMode() const { return mMode; }

			/**
			 * @return the cutoff frequency (for highpass and lowpass filters) or the center frequency (for bandpass filters) in Hz.
			 */
			ControllerValue getFrequency() const { return mFrequency; }

			/**
			 * @return the resonance peak for resonating low- and highpass filtering.
			 */
			ControllerValue getResonance() const { return mResonance; }

			/**
			 * @return the bandwidth in Hz for bandpass filters.
			 */
			ControllerValue getBand() const { return mBand; }

			/**
			 * @return the output gain factor of the filter.
			 */
			ControllerValue getGain() const { return mGain; }

		private:
			// Inherited from Node
			void process() override;
			void sampleRateChanged(float sampleRate) override;

			void calcCoeffs();

			std::atomic<EMode> mMode = {EMode::LowPass};
			std::atomic<ControllerValue> mFrequency = {440.f};
			std::atomic<ControllerValue> mResonance = {1.f};
			std::atomic<ControllerValue> mBand = {100.f};
			std::atomic<ControllerValue> mGain = {1.f};
			DirtyFlag mIsDirty;

			// Filter coefficients:
			// b0, b1 and b2 multiply the input signal, a1 and a2 the feedback of the output signal.
			// The denominator is normalized (leading coefficient 1) and the gain sits in the
			// numerator taps only, so the filter stays stable for any gain.
			// While the cutoff frequency or resonance is ramping, the coefficients are recalculated
			// from the smoothed parameter values every mCoefficientsUpdateInterval samples.
			ControllerValue b0 = 0;
			ControllerValue b1 = 0;
			ControllerValue b2 = 0;
			ControllerValue a1 = 0;
			ControllerValue a2 = 0;
			TimeValue mSmoothing = 1.f;

			// The cutoff frequency and resonance ramp towards the values set by setFrequency() and setResonance().
			// When none of them is ramping their current value equals their destination value.
			LinearSmoothedValue<ControllerValue> mSmoothedFrequency = { 440.f, 16 };
			LinearSmoothedValue<ControllerValue> mSmoothedResonance = { 1.f, 16 };

			// Number of samples between coefficient recalculations while ramping a parameter.
			static constexpr int mCoefficientsUpdateInterval = 16;

			Delay mOutput; // Delay line storing the output signal.
			Delay mInput; // Delay line storing the input signal.
		};

	}
}
