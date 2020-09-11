#include "syncvideocomponent.h"

// External Includes
#include <entity.h>
#include <mathutils.h>
#include <renderablemeshcomponent.h>
#include <rendertexture2d.h>

// nap::selectvideocomponent run time class definition 
RTTI_BEGIN_CLASS(nap::SyncVideoComponent)
	RTTI_PROPERTY("VideoPlayer", &nap::SyncVideoComponent::mVideoPlayer,	nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

// nap::selectvideocomponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::SyncVideoComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	void SyncVideoComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.emplace_back(RTTI_OF(nap::RenderableMeshComponent));
	}


	bool SyncVideoComponentInstance::init(utility::ErrorState& errorState)
	{
		// Copy over video
		nap::SyncVideoComponent* resource = getComponent<SyncVideoComponent>();
		mVideoPlayer = resource->mVideoPlayer.get();

		// Get the render-able mesh that has the video material
		mVideoMesh = getEntityInstance()->findComponent<RenderableMeshComponentInstance>();
		if (!errorState.check(mVideoMesh != nullptr, "%s: missing RenderableMeshComponent", mID.c_str()))
			return false;

		// Listen to video selection changes
		mVideoPlayer->VideoChanged.connect(mVideoChangedSlot);

		// Update textures on initialization
		videoChanged(*mVideoPlayer);
		return true;
	}


	void SyncVideoComponentInstance::videoChanged(VideoPlayer& player)
	{
		// Sync textures for video material
		assert(mVideoMesh != nullptr);
		MaterialInstance& video_material = mVideoMesh->getMaterialInstance();
		video_material.getOrCreateSampler<Sampler2DInstance>("yTexture")->setTexture(player.getYTexture());
		video_material.getOrCreateSampler<Sampler2DInstance>("uTexture")->setTexture(player.getUTexture());
		video_material.getOrCreateSampler<Sampler2DInstance>("vTexture")->setTexture(player.getVTexture());
	}
}