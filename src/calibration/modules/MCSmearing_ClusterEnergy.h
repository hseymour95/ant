#pragma once

#include "calibration/Calibration.h"
#include "base/Detector_t.h"
#include "base/OptionsList.h"

#include "tree/TID.h" // for TKeyValue, TID

#include <memory>

class TH2;
namespace ant {

namespace calibration {

class DataManager;

class MCSmearing_ClusterEnergy :
        public Calibration::Module, // this makes this module abstract
        public ReconstructHook::Clusters
{

public:
    // ReconstructHook
    virtual void ApplyTo(clusters_t& clusters) override;

    // Updateable_traits interface
    virtual std::list<Loader_t> GetLoaders() override;

protected:
    MCSmearing_ClusterEnergy(Detector_t::Type_t detectorType,
           std::shared_ptr<DataManager> calmgr,
                              std::vector<double> defaultPedestals
           );
    virtual ~MCSmearing_ClusterEnergy();

    /**
     * @brief The CalibType struct stores the data
     * for the Updateable interface and for the GUI
     */
    struct CalibType
    {
        // see also implementation of Get method
        std::vector<double> DefaultValues; // if empty, channel-independent DefaultValue is used
        std::vector<double> Values;        // if empty, channel-dependent DefaultValues[ch] is used
        const std::string   Name;

        double Get(unsigned channel, const double E) const;

        CalibType(const std::vector<double>& defaultValues, const std::string& name) :
            DefaultValues(defaultValues),
            Values(),
            Name(name)
        {}
    }; // CalibType


    const Detector_t::Type_t DetectorType;

    std::shared_ptr<DataManager> calibrationManager;

    CalibType EnergySigma;

    std::vector<CalibType*> AllCalibrations = {
        std::addressof(EnergySigma)
    };

};

}}  // namespace ant::calibration