#pragma once

#include <nap/core.h>
#include <parameter.h>

#include <renderwindow.h>

namespace nap
{

	class AudioPlugin
	{
		RTTI_ENABLE()

	public:
		AudioPlugin(Core& core) : mCore(core) { }

		virtual bool init(utility::ErrorState& errorState) { return true; }
		virtual void update(double deltaTime) { }
		virtual void render(RenderWindow* renderWindow) { }
		virtual void shutdown() { }

		Signal<Parameter&> registerParameterSignal;

	protected:
		Core& getCore() { return mCore; }

	private:
		Core& mCore;
	};

}
