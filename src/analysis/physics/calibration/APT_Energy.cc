#include "APT_Energy.h"

#include "expconfig/ExpConfig.h"

#include "base/std_ext/container.h"

#include "base/Logger.h"

#include <numeric>  // std::accumulate

using namespace std;
using namespace ant;
using namespace ant::analysis::physics;

template<typename T>
bool APT_Energy::shift_right(std::vector<T>& v)
{
    std::rotate(v.begin(), v.end() -1, v.end());
    return true;
}

APLCON::Fit_Settings_t APT_Energy::MakeFitSettings(unsigned max_iterations)
{
    APLCON::Fit_Settings_t settings;
    settings.MaxIterations = max_iterations;
    return settings;
}

APT_Energy::APT_Energy(const string& name, OptionsPtr opts) :
    Physics(name, opts),
    useHEP(opts->Get<bool>("UseHEP", false)),
    MAX_GAMMA(opts->Get<unsigned>("MaxGamma", 4)),
    model(make_shared<utils::UncertaintyModels::FitterSergey>()),
    kinfit(model, true, MakeFitSettings(20))
{
    promptrandom.AddPromptRange({-3, 2});

    const auto detector = ExpConfig::Setup::GetDetector(Detector_t::Type_t::APT);
    const auto nChannels = detector->GetNChannels();

    const BinSettings apt_channels(nChannels);
    const BinSettings apt_rawvalues(300);
    const BinSettings energybins(500, 0, 10);
    const BinSettings cb_energy(600, 0, 1200);
    const BinSettings apt_energy(100, 0, 10);


    h_pedestals = HistFac.makeTH2D(
                      "APT Pedestals",
                      "Raw ADC value",
                      "#",
                      apt_rawvalues,
                      apt_channels,
                      "Pedestals");

    h_bananas = HistFac.makeTH3D(
                "APT Bananas",
                "CB Energy / MeV",
                "APT Energy / MeV",
                "Channel",
                cb_energy,
                apt_energy,
                apt_channels,
                "Bananas"
                );

    for(unsigned ch=0;ch<nChannels;ch++) {
        stringstream ss;
        ss << "Ch" << ch;
        h_perChannel.push_back(
                    PerChannel_t(HistogramFactory(ss.str(), HistFac, ss.str())));
    }


    constexpr double Z_VERTEX = 3.;

    kinfit.SetZVertexSigma(Z_VERTEX);

    // initialize the different kinematic fits for each multiplicity
    unsigned n = MinNGamma()-1;
    while (++n <= MaxNGamma())
        kinfits.emplace_back(utils::KinFitter(model, true, MakeFitSettings(20)));

    for (auto& fit : kinfits)
        fit.SetZVertexSigma(Z_VERTEX);


    dEvE_all_combined = HistFac.makeTH2D("M+ combined all channels dEvE proton fitted",
                                         "E_{p} [MeV]", "E_{APT} [MeV]",
                                         cb_energy, apt_energy, "dEvE_all_combined");

    for (size_t i = 0; i < nChannels; i++)
        dEvE_combined.emplace_back(HistFac.makeTH2D("M+ combined channel "+to_string(i)+" dEvE proton fitted",
                                                    "E_{p} [MeV]", "E_{APT} [MeV]",
                                                    cb_energy, apt_energy,
                                                    "dEvE_fit_p_combined_chan"+to_string(i)));

    projections = HistFac.makeTH2D("Projections of High Energy Tail of Protons", "E_{APT} [MeV]", "APT Channel",
                                   apt_energy, BinSettings(nChannels), "projections_hep");


    if (useHEP) {
        LOG(INFO) << "Create APT Calibration histograms for High Energy Protons method";
        LOG(INFO) << "Search protons in events with up to " << MaxNGamma() << " Photons";
    }
}

APT_Energy::PerChannel_t::PerChannel_t(HistogramFactory HistFac)
{
    const BinSettings cb_energy(400,0,800);
    const BinSettings apt_timing(300,-300,700);
    const BinSettings apt_rawvalues(300);
    const BinSettings apt_energy(150,0,30);

    PedestalTiming = HistFac.makeTH2D(
                         "APT Pedestals Timing",
                         "Timing / ns",
                         "Raw ADC value",
                         apt_timing,
                         apt_rawvalues,
                         "PedestalTiming");

    PedestalNoTiming = HistFac.makeTH1D(
                           "APT Pedestals No Timing",
                           "Raw ADC value",
                           "#",
                           apt_rawvalues,
                           "PedestalNoTiming");

    Banana = HistFac.makeTH2D(
                 "APT Banana",
                 "CB Energy / MeV",
                 "APT Energy / MeV",
                 cb_energy,
                 apt_energy,
                 "Banana"
                 );

    BananaRaw = HistFac.makeTH2D(
                    "APT Banana Raw",
                    "CB Energy / MeV",
                    "APT ADC Value",
                    cb_energy,
                    BinSettings(300,0,2000),
                    "BananaRaw"
                    );

    BananaUnmatched = HistFac.makeTH2D(
                 "APT Banana",
                 "CB Energy / MeV",
                 "APT Energy / MeV",
                 cb_energy,
                 apt_energy,
                 "BananaUnmatched"
                 );


    TDCMultiplicity = HistFac.makeTH1D("APT TDC Multiplicity", "nHits", "#", BinSettings(10), "TDCMultiplicity");
    QDCMultiplicity = HistFac.makeTH1D("APT QDC Multiplicity", "nHits", "#", BinSettings(10), "QDCMultiplicity");
}

void APT_Energy::ProcessEvent(const TEvent& event, manager_t&)
{
    triggersimu.ProcessEvent(event);

    // pedestals, best determined from clusters with energy information only

    struct hitmapping_t {
        vector<TDetectorReadHit::Value_t> Integrals;
        vector<TDetectorReadHit::Value_t> Timings;
    };

    std::map<unsigned, hitmapping_t> hits;

    for(const TDetectorReadHit& readhit : event.Reconstructed().DetectorReadHits) {
        if(readhit.DetectorType != Detector_t::Type_t::APT)
            continue;

        auto& item = hits[readhit.Channel];

        if(readhit.ChannelType == Channel_t::Type_t::Integral) {
            std_ext::concatenate(item.Integrals, readhit.Values);
        }
        else if(readhit.ChannelType == Channel_t::Type_t::Timing) {
            std_ext::concatenate(item.Timings, readhit.Values); // passed the timing window!
        }
    }

    for(const auto& it_hit : hits) {

        const auto channel = it_hit.first;
        const hitmapping_t& item = it_hit.second;

        PerChannel_t& h = h_perChannel[channel];

        h.QDCMultiplicity->Fill(item.Integrals.size());
        h.TDCMultiplicity->Fill(item.Timings.size());


        if(item.Integrals.size() != 1)
            continue;
        if(item.Timings.size()>1)
            continue;

        const auto& pedestal = item.Integrals.front().Uncalibrated;

        h_pedestals->Fill(pedestal, channel);

        if(item.Timings.size()==1)
            h.PedestalTiming->Fill(item.Timings.front().Calibrated, pedestal);
        else
            h.PedestalNoTiming->Fill(pedestal);

    }

    // get some CandidateMatcher independent bananas
    {
        TClusterPtrList cbClusters;
        TClusterPtrList aptClusters;
        for(auto cl : event.Reconstructed().Clusters.get_iter()) {
            if(cl->DetectorType == Detector_t::Type_t::CB)
                cbClusters.emplace_back(cl);
            else if(cl->DetectorType == Detector_t::Type_t::APT)
                aptClusters.emplace_back(cl);

        }

        if(aptClusters.size() == 1) {
            const auto& APT_cluster = *aptClusters.front();
            // per channel histograms
            PerChannel_t& h = h_perChannel[APT_cluster.CentralElement];

            for(auto& cb_cluster : cbClusters) {
                h.BananaUnmatched->Fill(cb_cluster->Energy, APT_cluster.Energy);
            }
        }
    }

    // bananas per channel histograms
    for(const auto& candidate : event.Reconstructed().Candidates) {
        // only candidates with one cluster in CB and one cluster in APT
        if(candidate.Clusters.size() != 2)
            continue;
        const bool cb_and_apt = candidate.Detector & Detector_t::Type_t::CB &&
                                candidate.Detector & Detector_t::Type_t::APT;
        if(!cb_and_apt)
            continue;

        // search for APT cluster
        const auto& APT_cluster = candidate.FindFirstCluster(Detector_t::Type_t::APT);

        if (!useHEP)
            h_bananas->Fill(candidate.CaloEnergy,
                            candidate.VetoEnergy,
                            APT_cluster->CentralElement);

        // per channel histograms
        PerChannel_t& h = h_perChannel[APT_cluster->CentralElement];

        // fill the banana
        h.Banana->Fill(candidate.CaloEnergy,
                       candidate.VetoEnergy);

        // is there an pedestal available?
        const auto it_hit = hits.find(APT_cluster->CentralElement);
        if(it_hit == hits.end()) {
            continue;
        }
        const auto& integrals = it_hit->second.Integrals;
        if(integrals.size() != 1)
            continue;

        const auto& pedestal = integrals.front().Uncalibrated;

        h.BananaRaw->Fill(candidate.CaloEnergy, pedestal);
        //h.BananaTiming->Fill(candidate.ClusterEnergy(), candidate.VetoEnergy(), timing);

    }


    if (useHEP)
        ProcessHEP(event);
}

void APT_Energy::ProcessHEP(const TEvent &event)
{
    const auto& cands = event.Reconstructed().Candidates;

    if (cands.size() > MaxNGamma()+1 || cands.size() < MinNGamma()+1)
        return;

    TCandidatePtrList comb;
    TParticlePtr fitted_proton;

    for (const auto& taggerhit : event.Reconstructed().TaggerHits) {
        promptrandom.SetTaggerTime(triggersimu.GetCorrectedTaggerTime(taggerhit));
        comb.clear();
        for (auto p : cands.get_iter())
            comb.emplace_back(p);
        if (!find_best_comb(taggerhit, comb, fitted_proton))
            continue;

        // proton candidate found, fill histograms
        comb.pop_back();
        if (fitted_proton->Candidate->VetoEnergy && fitted_proton->Candidate->Detector & Detector_t::Type_t::CB) {
            dEvE_combined.at(fitted_proton->Candidate->FindVetoCluster()->CentralElement)
                    ->Fill(fitted_proton->E - ParticleTypeDatabase::Proton.Mass(), fitted_proton->Candidate->VetoEnergy);
            h_bananas->Fill(fitted_proton->E - ParticleTypeDatabase::Proton.Mass(),
                            fitted_proton->Candidate->VetoEnergy,
                            fitted_proton->Candidate->FindVetoCluster()->CentralElement);
            dEvE_all_combined->Fill(fitted_proton->E - ParticleTypeDatabase::Proton.Mass(), fitted_proton->Candidate->VetoEnergy);
        }
    }
}

void APT_Energy::Finish()
{
    const auto detector = ExpConfig::Setup::GetDetector(Detector_t::Type_t::APT);

    h_BananaEntries = HistFac.makeTH1D("Banana Entries","Channel","",
                                       BinSettings(detector->GetNChannels()), "h_BananaEntries");
    h_BananaEntriesUnmatched = HistFac.makeTH1D("Banana Entries Unmatched","Channel","",
                                                BinSettings(detector->GetNChannels()), "h_BananaEntriesUnmatched");
    for(int ch=0;ch<int(h_perChannel.size());ch++) {
        auto& perCh = h_perChannel.at(ch);
        h_BananaEntries->SetBinContent(ch+1, perCh.Banana->GetEntries());
        h_BananaEntriesUnmatched->SetBinContent(ch+1, perCh.BananaUnmatched->GetEntries());
    }

    h_BananaEntries->SetMinimum(0);
    h_BananaEntriesUnmatched->SetMinimum(0);

    if (!useHEP)
        return;

    int channel = 0;
    for (auto& hist : dEvE_combined) {
        TH1* h = hist->ProjectionY("_py", hist->GetXaxis()->FindBin(FIRST), hist->GetXaxis()->FindBin(LAST));
        int bins = h->GetXaxis()->GetNbins();
        for (int i = 1; i <= bins; i++)
            projections->Fill(h->GetBinCenter(i), channel, h->GetBinContent(i));
        channel++;
        delete h;
    }
}

void APT_Energy::ShowResult()
{
    canvas(GetName())
            << drawoption("colz") << h_pedestals
            << endc;
    canvas c_bananas(GetName()+": Bananas");
    canvas c_bananas_unmatched(GetName()+": Bananas Unmatched");

    c_bananas << drawoption("colz") << h_BananaEntries;
    c_bananas_unmatched << drawoption("colz") << h_BananaEntriesUnmatched;
    for(auto& h : h_perChannel) {
        c_bananas << h.Banana;
        c_bananas_unmatched << h.BananaUnmatched;
    }
    c_bananas << endc;
    c_bananas_unmatched << endc;

    if (useHEP) {
        canvas(GetName()+": HEP") << drawoption("colz") << projections << endc;
        canvas hep_bananas(GetName()+": HEP Bananas");
        hep_bananas << drawoption("colz");
        for (auto& hist : dEvE_combined)
            hep_bananas << hist;
        hep_bananas << endc;
    }
}

bool APT_Energy::find_best_comb(const TTaggerHit& taggerhit,
                                TCandidatePtrList& comb,
                                TParticlePtr& fitted_proton)
{
    double best_prob_fit = -std_ext::inf;
    size_t best_comb_fit = comb.size();
    TLorentzVector eta;
    TParticlePtr proton;
    TParticleList photons;

    /* kinematical checks to reduce computing time */
    const interval<double> coplanarity({-25, 25});
    const interval<double> mm = ParticleTypeDatabase::Proton.GetWindow(300);

    /* test all different combinations to find the best proton candidate */
    size_t i = 0;
    do {
        // ensure the possible proton candidate is kinematically allowed
        if (std_ext::radian_to_degree(comb.back()->Theta) > 90.)
            continue;

        photons.clear();
        proton = make_shared<TParticle>(ParticleTypeDatabase::Proton, comb.back());  // always assume last particle is the proton
        eta.SetXYZT(0,0,0,0);
        for (size_t j = 0; j < comb.size()-1; j++) {
            photons.emplace_back(make_shared<TParticle>(ParticleTypeDatabase::Photon, comb.at(j)));
            eta += TParticle(ParticleTypeDatabase::Photon, comb.at(j));
        }

        const double copl = std_ext::radian_to_degree(abs(eta.Phi() - proton->Phi())) - 180.;
        if (!coplanarity.Contains(copl))
            continue;

        LorentzVec missing = taggerhit.GetPhotonBeam() + LorentzVec({0, 0, 0}, ParticleTypeDatabase::Proton.Mass());
        missing -= eta;
        if (!mm.Contains(missing.M()))
            continue;

        /* now start with the kinematic fitting */
        auto& fit = kinfits.at(photons.size()-MinNGamma());  // choose the fitter for the right amount of photons

        auto kinfit_result = fit.DoFit(taggerhit.PhotonEnergy, proton, photons);

        if (kinfit_result.Status != APLCON::Result_Status_t::Success)
            continue;

        if (PROBABILITY_CUT)
            if (kinfit_result.Probability < PROBABILITY)
                continue;

        if (!std_ext::copy_if_greater(best_prob_fit, kinfit_result.Probability))
            continue;

        best_comb_fit = i;
        fitted_proton = fit.GetFittedProton();
    } while (shift_right(comb) && ++i < comb.size());

    // check if a valid combination was found
    if (best_comb_fit >= comb.size() || !isfinite(best_prob_fit))
        return false;

    // restore combinations with best probability
    while (best_comb_fit-- > 0)
        shift_right(comb);

    return true;
}

AUTO_REGISTER_PHYSICS(APT_Energy)
