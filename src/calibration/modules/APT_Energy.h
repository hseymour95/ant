#pragma once

#include "Energy.h"

class TH1;

namespace ant {

namespace expconfig {
namespace detector {
struct APT;
}}

namespace calibration {

class APT_Energy :
        public Energy,
        public ReconstructHook::EventData
{


public:

    using detector_ptr_t = std::shared_ptr<const expconfig::detector::APT>;

    APT_Energy(
            const detector_ptr_t& apt,
            const std::shared_ptr<DataManager>& calmgr,
            const Calibration::Converter::ptr_t& converter,
            defaults_t defaultPedestals,
            defaults_t defaultPhotons,
            defaults_t defaultGains,
            defaults_t defaultThresholds_Raw, // roughly width of pedestal peak
            defaults_t defaultThresholds_MeV,
            defaults_t defaultRelativeGains
    );

    virtual ~APT_Energy();

    virtual void GetGUIs(std::list<std::unique_ptr<calibration::gui::CalibModule_traits> >& guis, ant::OptionsPtr options) override;

    using Energy::ApplyTo; // still this ApplyTo should be used
    virtual void ApplyTo(TEventData& reconstructed) override;

protected:
    const detector_ptr_t apt_detector;

};

}} // namespace ant::calibration
