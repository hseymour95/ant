#include "etaprime_sergey.h"

#include "plot/root_draw.h"
#include "base/std_ext/misc.h"
#include "base/Logger.h"
#include "utils/particle_tools.h"

using namespace ant;
using namespace ant::analysis;
using namespace ant::analysis::physics;
using namespace std;

EtapSergey::EtapSergey(const string& name, OptionsPtr opts) :
    Physics(name, opts),
    fit_model(utils::UncertaintyModels::Interpolated::makeAndLoad()
          ),
    fitter("KinFit", 2, fit_model)
{
    promptrandom.AddPromptRange({ -7,   7});
    promptrandom.AddRandomRange({-65, -15});
    promptrandom.AddRandomRange({ 15,  65});

    steps = HistFac.makeTH1D("Steps","","#",BinSettings(10),"steps");

    t.CreateBranches(HistFac.makeTTree("tree"));
}

void EtapSergey::ProcessEvent(const TEvent& event, manager_t&)
{
    const TEventData& data = event.Reconstructed();
    const bool is_MC = data.ID.isSet(TID::Flags_t::MC);

    steps->Fill("Seen",1);

    if(is_MC) {
        if(data.Trigger.CBEnergySum <= 550)
            return;
        steps->Fill("MC CBESum>550MeV",1);
    }

    const auto& cands = data.Candidates;
    if(cands.size() != 3)
        return;
    steps->Fill("nCands==3",1);


    for(const TTaggerHit& taggerhit : data.TaggerHits) {

        steps->Fill("Seen taggerhits",1.0);

        promptrandom.SetTaggerHit(taggerhit.Time);
        if(promptrandom.State() == PromptRandom::Case::Outside)
            continue;

        t.TaggW = promptrandom.FillWeight();
        t.TaggE = taggerhit.PhotonEnergy;
        t.TaggT = taggerhit.Time;
        t.TaggCh = taggerhit.Channel;

        double best_prob = std_ext::NaN;

        // use any candidate as proton, and do the analysis (ignore ParticleID stuff)
        for(auto i_proton : cands.get_iter()) {

            steps->Fill("Seen protons",1.0);

            TParticlePtr proton = std::make_shared<TParticle>(ParticleTypeDatabase::Proton, i_proton);
            std::vector<TParticlePtr> photons;
            for(auto i_photon : cands.get_iter()) {
                if(i_photon == i_proton)
                    continue;
                photons.emplace_back(make_shared<TParticle>(ParticleTypeDatabase::Photon, i_photon));
            }

            LorentzVec photon_sum({0,0,0},0);
            for(const auto& p : photons) {
                photon_sum += *p;
            }

            // missing mass
            const LorentzVec& beam_target = taggerhit.GetPhotonBeam() + LorentzVec({0, 0, 0}, ParticleTypeDatabase::Proton.Mass());
            const LorentzVec& missing = beam_target - photon_sum;
            const double missing_mass = missing.M();

            if(missing_mass<550 || missing_mass>1300)
                continue;
            steps->Fill("MM in [550;1300]",1);

            // do the fitting

            fitter.SetEgammaBeam(taggerhit.PhotonEnergy);
            fitter.SetProton(proton);
            fitter.SetPhotons(photons);
            const auto& fit_result = fitter.DoFit();

            if(fit_result.Status != APLCON::Result_Status_t::Success)
                continue;

            steps->Fill("KinFit OK",1);

            // only use the proton permutation with the best fit
            if(!std_ext::copy_if_greater(best_prob, fit_result.Probability))
                continue;

            t.ProtonTime = proton->Candidate->Time;
            t.ProtonE = proton->Ek();
            t.ProtonTheta = std_ext::radian_to_degree(proton->Theta());
            t.ProtonVetoE = proton->Candidate->VetoEnergy;
            t.ProtonShortE = proton->Candidate->FindCaloCluster()->ShortEnergy;

            t.PhotonsTheta().resize(0);
            t.PhotonsE().resize(0);
            t.nPhotonsCB = 0;
            t.nPhotonsTAPS = 0;
            for(const auto& photon : photons) {
                t.PhotonsTheta().push_back(std_ext::radian_to_degree(photon->Theta()));\
                t.PhotonsE().push_back(photon->Ek());\
                if(photon->Candidate->Detector & Detector_t::Type_t::CB)
                    t.nPhotonsCB()++;
                if(photon->Candidate->Detector & Detector_t::Type_t::TAPS)
                    t.nPhotonsTAPS()++;
            }

            t.PhotonSum   = photon_sum.M();
            t.MissingMass = missing_mass;

            t.KinFitProb = fit_result.Probability;
            t.KinFitIterations = fit_result.NIterations;


            auto fitted_proton = fitter.GetFittedProton();
            t.FittedProtonE =  fitted_proton->Ek();
            t.FittedProtonTheta = std_ext::radian_to_degree(fitted_proton->Theta());


            auto fitted_photons = fitter.GetFittedPhotons();
            t.FittedPhotonsTheta().resize(0);
            t.FittedPhotonsE().resize(0);
            LorentzVec fitted_photon_sum({0,0,0},0);
            for(const auto& photon : fitted_photons) {
                t.FittedPhotonsTheta().push_back(std_ext::radian_to_degree(photon->Theta()));\
                t.FittedPhotonsE().push_back(photon->Ek());\
                fitted_photon_sum += *photon;
            }
            t.FittedPhotonSum = fitted_photon_sum.M();
        }

        if(!isfinite(best_prob))
            continue;

        steps->Fill("Fill",1);

        t.Tree->Fill();

    }

}

void EtapSergey::ShowResult()
{
    canvas(GetName())
            << TTree_drawable(t.Tree, "PhotonSum >> h1(150,400,700)","TaggW*(KinFitProb>0.01)")
            << TTree_drawable(t.Tree, "PhotonSum >> h2(150,400,700)","(TaggW<0)*(KinFitProb>0.01)")
            << TTree_drawable(t.Tree, "PhotonSum >> h3(150,400,700)","(TaggW>0)*(KinFitProb>0.01)")
            << TTree_drawable(t.Tree, "FittedPhotonSum >> h4(150,400,700)","TaggW*(KinFitProb>0.01)")
            << TTree_drawable(t.Tree, "FittedPhotonSum >> h5(150,400,700)","(TaggW<0)*(KinFitProb>0.01)")
            << TTree_drawable(t.Tree, "FittedPhotonSum >> h6(150,400,700)","(TaggW>0)*(KinFitProb>0.01)")
            << endc;
}

AUTO_REGISTER_PHYSICS(EtapSergey)