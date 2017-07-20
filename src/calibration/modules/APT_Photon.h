#pragma once

#include "Photon.h"

class TH1;

namespace ant {

namespace expconfig {
namespace detector {
struct APT;
}}

namespace calibration {

class APT_Photon :
        public Photon,
        public ReconstructHook::EventData
{


public:

    using detector_ptr_t = std::shared_ptr<const expconfig::detector::APT>;

    APT_Photon(
            const detector_ptr_t& apt,
            const std::shared_ptr<DataManager>& calmgr,
            const Calibration::Converter::ptr_t& converter,
            //defaults_t defaultPedestals,
            //defaults_t defaultGains,
            //defaults_t defaultThresholds_Raw, // roughly width of pedestal peak
            //defaults_t defaultThresholds_MeV,
            //defaults_t defaultRelativeGains
            defaults_t defaultPhotonPeak
    );

    virtual ~APT_Photon();

    virtual void GetGUIs(std::list<std::unique_ptr<calibration::gui::CalibModule_traits> >& guis, ant::OptionsPtr options) override;

    using Photon::ApplyTo; // still this ApplyTo should be used
    virtual void ApplyTo(TEventData& reconstructed) override;

protected:
    const detector_ptr_t apt_detector;

};

}} // namespace ant::calibration
