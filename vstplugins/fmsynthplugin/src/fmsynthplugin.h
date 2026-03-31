#pragma once

#include <audioplugin.h>
#include <parametergroup.h>
#include <parametergui.h>
#include <imgui.h>
#include <renderservice.h>
#include <imguiservice.h>
#include <nap/core.h>

namespace nap
{

	class FMSynthPlugin : public AudioPlugin
	{
		RTTI_ENABLE(AudioPlugin)

	public:
		FMSynthPlugin(Core& core) : AudioPlugin(core) { }

		bool init(utility::ErrorState& errorState) override
		{
			mRenderService = getCore().getService<RenderService>();
			mGuiService = getCore().getService<IMGuiService>();

			auto parameterGroup = getCore().getResourceManager()->findObject<nap::ParameterGroup>("Parameters").get();
			if (parameterGroup == nullptr)
			{
				errorState.fail("Can't find parameter group: Parameters");
				return false;
			}

			for (auto& parameter : parameterGroup->mMembers)
				registerParameterSignal(*parameter);

			mParameterGUI = std::make_unique<nap::ParameterGUI>(getCore());
			mParameterGUI->mParameterGroup = parameterGroup;
			mParameterGUI->mSerializable = false;
			if (!mParameterGUI->init(errorState))
			{
				errorState.fail("Failed to initialize parameter GUI.");
				return false;
			}

			return true;
		}

		void update(double deltaTime) override
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

			ImGui::SetNextWindowPos(ImVec2(150, 0));
			ImGui::SetNextWindowSize(ImVec2(400, 390));
			ImGui::SetNextWindowBgAlpha(0.0f);

			ImGui::Begin("FM Synth", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
			if (mParameterGUI != nullptr)
				mParameterGUI->show(false);
			ImGui::NewLine();

			std::string formattedText = nap::utility::stringFormat("Framerate: %.02f", getCore().getFramerate());
			ImGui::Text(formattedText.c_str());
			ImGui::End();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}

		void render(nap::RenderWindow* renderWindow) override
		{
			mRenderService->beginFrame();
			if (renderWindow != nullptr)
			{
				if (mRenderService->beginRecording(*renderWindow))
				{
					renderWindow->beginRendering();
					mGuiService->draw();
					renderWindow->endRendering();
					mRenderService->endRecording();
				}
			}
			mRenderService->endFrame();
		}

		void shutdown() override
		{
			mParameterGUI->onDestroy();
			mParameterGUI = nullptr;
		}

		glm::vec2 getRenderWindowSize() override { return glm::vec2(600.f, 390.f); }

	private:
		std::unique_ptr<nap::ParameterGUI> mParameterGUI = nullptr;
		RenderService* mRenderService = nullptr;
		IMGuiService* mGuiService = nullptr;
	};

}