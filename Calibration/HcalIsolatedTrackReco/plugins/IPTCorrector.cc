/* \class IsolatedPixelTrackCorrector
 *
 *  
 */

#include <vector>
#include <memory>
#include <algorithm>

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/HcalIsolatedTrack/interface/IsolatedPixelTrackCandidate.h"
#include "DataFormats/HcalIsolatedTrack/interface/IsolatedPixelTrackCandidateFwd.h"
#include "DataFormats/HLTReco/interface/TriggerFilterObjectWithRefs.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/TrackReco/interface/Track.h"

// Framework
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"
//
///
// Math
#include "Math/GenVector/VectorUtil.h"
#include "Math/GenVector/PxPyPzE4D.h"

class IPTCorrector : public edm::global::EDProducer<> {
public:
  IPTCorrector(const edm::ParameterSet& ps);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  void produce(edm::StreamID, edm::Event&, edm::EventSetup const&) const override;

private:
  const edm::EDGetTokenT<reco::TrackCollection> tok_cor_;
  const edm::EDGetTokenT<trigger::TriggerFilterObjectWithRefs> tok_uncor_;
  const double assocCone_;
};

IPTCorrector::IPTCorrector(const edm::ParameterSet& config)
    : tok_cor_(consumes<reco::TrackCollection>(config.getParameter<edm::InputTag>("corTracksLabel"))),
      tok_uncor_(consumes<trigger::TriggerFilterObjectWithRefs>(config.getParameter<edm::InputTag>("filterLabel"))),
      assocCone_(config.getParameter<double>("associationCone")) {
  // register the product
  produces<reco::IsolatedPixelTrackCandidateCollection>();
}

void IPTCorrector::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("corTracksLabel", edm::InputTag("hltIter0PFlowCtfWithMaterialTracks"));
  desc.add<edm::InputTag>("filterLabel", edm::InputTag("hltIsolPixelTrackL2Filter"));
  desc.add<double>("associationCone", 0.2);
  descriptions.add("iptCorrector", desc);
}

void IPTCorrector::produce(edm::StreamID, edm::Event& theEvent, edm::EventSetup const&) const {
  auto trackCollection = std::make_unique<reco::IsolatedPixelTrackCandidateCollection>();

  edm::Handle<reco::TrackCollection> corTracks;
  theEvent.getByToken(tok_cor_, corTracks);

  edm::Handle<trigger::TriggerFilterObjectWithRefs> fiCand;
  theEvent.getByToken(tok_uncor_, fiCand);

  std::vector<edm::Ref<reco::IsolatedPixelTrackCandidateCollection> > isoPixTrackRefs;

  fiCand->getObjects(trigger::TriggerTrack, isoPixTrackRefs);

  int nCand = isoPixTrackRefs.size();

  //loop over input ipt

  for (int p = 0; p < nCand; p++) {
    double iptEta = isoPixTrackRefs[p]->track()->eta();
    double iptPhi = isoPixTrackRefs[p]->track()->phi();

    int ntrk = 0;
    double minDR = 100;
    reco::TrackCollection::const_iterator citSel;

    for (reco::TrackCollection::const_iterator cit = corTracks->begin(); cit != corTracks->end(); cit++) {
      double dR = deltaR(cit->eta(), cit->phi(), iptEta, iptPhi);
      if (dR < minDR && dR < assocCone_) {
        minDR = dR;
        ntrk++;
        citSel = cit;
      }
    }

    if (ntrk > 0) {
      reco::IsolatedPixelTrackCandidate newCandidate(reco::TrackRef(corTracks, citSel - corTracks->begin()),
                                                     isoPixTrackRefs[p]->l1tau(),
                                                     isoPixTrackRefs[p]->maxPtPxl(),
                                                     isoPixTrackRefs[p]->sumPtPxl());
      newCandidate.setEnergyIn(isoPixTrackRefs[p]->energyIn());
      newCandidate.setEnergyOut(isoPixTrackRefs[p]->energyOut());
      newCandidate.setNHitIn(isoPixTrackRefs[p]->nHitIn());
      newCandidate.setNHitOut(isoPixTrackRefs[p]->nHitOut());
      if (isoPixTrackRefs[p]->etaPhiEcalValid()) {
        std::pair<double, double> etaphi = (isoPixTrackRefs[p]->etaPhiEcal());
        newCandidate.setEtaPhiEcal(etaphi.first, etaphi.second);
      }
      trackCollection->push_back(newCandidate);
    }
  }

  // put the product in the event
  theEvent.put(std::move(trackCollection));
}

#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "FWCore/Framework/interface/MakerMacros.h"

DEFINE_FWK_MODULE(IPTCorrector);
