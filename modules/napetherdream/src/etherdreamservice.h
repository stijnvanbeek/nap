#pragma once

// External Includes
#include <nap/service.h>
#include <nap/entity.h>

// Local Includes
#include "etherdreamdac.h"

namespace nap
{
	/**
	 * Main interface for rendering to various Etherdream Dacs
	 * The service is responsible for opening / closing the general Etherdream library
	 * and allows for rendering data to the available dacs
	 */
	class NAPAPI EtherDreamService : public Service
	{
		friend class EtherDreamDac;
		RTTI_ENABLE(Service)

	public:
		// Default Constructor
		EtherDreamService();

		// Default Destructor
		virtual ~EtherDreamService();

		/**
		 * Initializes the etherdream library.
		 */
		bool init(nap::utility::ErrorState& errorState);

		/**
		 *	Register specific object creators
		 */
		virtual void registerObjectCreators(rtti::Factory& factory) override;

	protected:
		/**
		 * Adds a dac to the system, every dac is associated with a number.
		 * This number can change but the dac name is unique, this call set the associated laser index 
		 * on the etherdream dac object
		 * @return if the dac has been found and added to the system
		 */
		bool allocateDAC(EtherDreamDac& dac);

		/**
		* @return the etherdream interface that manages all the DACs
		*/
		EtherDreamInterface* getInterface() const;

	private:
		// The etherdream interface;
		std::unique_ptr<EtherDreamInterface>		mInterface = nullptr;
		
		// All DACS that are registered and available to the system
		std::unordered_map<int, EtherDreamDac*>		mAvailableDacs;

		// All dacs found on the network associated with their index
		std::unordered_map<int, std::string>		mDacNames;

		/**
		* @return the dac number associated with a dac name, -1 if the dac is not available
		*/
		int getIndex(const std::string& name);
	};
}