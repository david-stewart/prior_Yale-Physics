// Process:
//      * Measure the ranking
//      * cut on ranking
//      * cut on Vz
//      * measure the Vz
//      * cut on Vz-vpd
//      * measure Vpd
//      * fill numberPrimaryTracks
//      * fill numberGLobalTracks
//      * fill hit ratio
//
//
// Includes code from (1) Joel Mazer from https://github.com/joelmazer/star-jetframework in July 2018
//                    (2) Tong Liu from ARCAS, also July 2018
//
#include "basicRunQA.h"
#include <iostream>
#define zmin -30
#define zmax  30


//-----------------------------------------------------------------------------
ClassImp(basicRunQA)

basicRunQA::basicRunQA(
        const char*     name, 
        StPicoDstMaker *picoMaker,
        /* const char* prof,*/ 
        const char*     outName, 
        bool            debug
        )
  : StMaker(name), 
    mPicoDstMaker(picoMaker),
    mOutName(outName),
    fdebug{debug},
    fEventsProcessed{0}
{ };

//----------------------------------------------------------------------------- 
basicRunQA::~basicRunQA()
{ /*  */ }

//----------------------------------------------------------------------------- 
Int_t basicRunQA::Init() {
    fout = new TFile(TString::Format("%s.root",mOutName.Data()).Data(),"RECREATE") ;
    flog = fopen(TString::Format("%s.log",mOutName.Data()).Data(),"w");
    fprintf(flog,"Initializing log\n");

    ftree = new TTree("tree","Tree of General Event Characteristics");
    ftree->Branch("event",  &fevent);

    return kStOK;
}

//----------------------------------------------------------------------------- 
Int_t basicRunQA::Finish() {
    fout->Write();
    fout->Close();
    fprintf(flog,"Closing log at end of file.\n");
    fclose(flog);
    return kStOK;
}
    
//----------------------------------------------------------------------------- 
void basicRunQA::Clear(Option_t *opt) { }

//----------------------------------------------------------------------------- 
Int_t basicRunQA::Make() {
    if(!mPicoDstMaker) {
        LOG_WARN << " No PicoDstMaker! Skip! " << endm;
        return kStWarn;
    }

    mPicoDst = mPicoDstMaker->picoDst();
    if(!mPicoDst) {
        LOG_WARN << " No PicoDst! Skip! " << endm;
        return kStWarn;
    }

    /* fHgram_ev_cuts->Fill(0); */
    StPicoEvent* mevent = mPicoDst->event();
    fevent.runId = mevent->runId();
    fevent.eventId = mevent->eventId();

    // check for the triggers
    bool has_trig{false};

    if (mevent->isTrigger(500206)) { fevent.trig_500206 = true; has_trig = true; } else { fevent.trig_500206 = false; }
    if (mevent->isTrigger(470202)) { fevent.trig_470202 = true; has_trig = true; } else { fevent.trig_470202 = false; }
    if (mevent->isTrigger(480202)) { fevent.trig_480202 = true; has_trig = true; } else { fevent.trig_480202 = false; }
    if (mevent->isTrigger(490202)) { fevent.trig_490202 = true; has_trig = true; } else { fevent.trig_490202 = false; }
    if (mevent->isTrigger(500202)) { fevent.trig_500202 = true; has_trig = true; } else { fevent.trig_500202 = false; }
    if (mevent->isTrigger(510202)) { fevent.trig_510202 = true; has_trig = true; } else { fevent.trig_510202 = false; }
    if (mevent->isTrigger(470205)) { fevent.trig_470205 = true; has_trig = true; } else { fevent.trig_470205 = false; }
    if (mevent->isTrigger(480205)) { fevent.trig_480205 = true; has_trig = true; } else { fevent.trig_480205 = false; }
    if (mevent->isTrigger(490205)) { fevent.trig_490205 = true; has_trig = true; } else { fevent.trig_490205 = false; }
    if (mevent->isTrigger(500215)) { fevent.trig_500215 = true; has_trig = true; } else { fevent.trig_500215 = false; }
    if (mevent->isTrigger(500001)) { fevent.trig_500001 = true; has_trig = true; } else { fevent.trig_500001 = false; }
    if (mevent->isTrigger(500904)) { fevent.trig_500904 = true; has_trig = true; } else { fevent.trig_500904 = false; }
    if (mevent->isTrigger(510009)) { fevent.trig_510009 = true; has_trig = true; } else { fevent.trig_510009 = false; }

    if (!has_trig) return kStOK;

    fevent.ranking = mevent->ranking();
    fevent.zPV     = mevent->primaryVertex().z();
    fevent.xPV     = mevent->primaryVertex().x();
    fevent.yPV     = mevent->primaryVertex().y();
    
    fevent.zdcX     = mevent->ZDCx();

    // Note: the "large signal" is for the large area exlussive of the small area
    double sum{0};
    for (int i = 1; i < 16; ++i) sum += mevent->bbcAdcEast(i);
    fevent.bbcAdcES = sum;

    sum = 0;
    for (int i = 16; i < 24; ++i) sum += mevent->bbcAdcEast(i);
    fevent.bbcAdcEL = sum;

    sum = 0;
    for (int i = 1; i < 16; ++i) sum += mevent->bbcAdcWest(i);
    fevent.bbcAdcWS = sum;

    sum = 0;
    for (int i = 16; i < 24; ++i) sum += mevent->bbcAdcWest(i);
    fevent.bbcAdcWL = sum;

    fevent.zdcSumAdcEast = mevent->ZdcSumAdcEast();
    fevent.zdcSumAdcWest = mevent->ZdcSumAdcWest();

    fevent.nGlobalTracks = mevent->numberOfGlobalTracks();
    fevent.refmult       = mevent->refMult(); 
    /* cout << "nGlobal " << mevent->numberOfGlobalTracks() << endl; */
    fevent.nTracks = mPicoDst->numberOfTracks();
    /* cout << "nPrimaryTracks " << mPicoDst->numberOfTracks(); */

    // now have to loop through the tracks to find the number of good primary tracks
    // (and therefore the ratio as well)
    double phi_sum{0};
    double eta_sum{0};
    
    double phi_lead{0};
    double eta_lead{0};

    double sumpt{0};
    double maxpt{0};

    int nPrimary{0};
    int nTof{0};

    fevent.nGoodPrimaryTracks = 0; // update in the loop;
    for (int i = 0; i < fevent.nTracks; ++i){
        StPicoTrack* track = static_cast<StPicoTrack*>(mPicoDst->track(i));
        if (!track->isPrimary()) continue;
        ++nPrimary;

        // thanks to joel mazer
        StThreeVectorF Ptrack = track->pMom();
        float pt  = Ptrack.perp();
        float eta { Ptrack.pseudoRapidity() };
        if (pt > 30) {
            fprintf(flog, "Run(%i) Event(%i) cut b/c track > 30 GeV at (%f GeV)\n",
                    fevent.runId, mevent->eventId(), pt);
            return kStOK; // cut out this event
        }
        float nhit_ratio = ((float)track->nHitsFit()) / (float)track->nHitsMax();
        if (   pt < 0.2 
            || (track->dcaPoint() - mevent->primaryVertex()).mag() > 1.0
            || TMath::Abs(eta)  >= 1.0
            || nhit_ratio < 0.52
        )  continue;

        if (track->bTofPidTraitsIndex() != -1) ++nTof;

        ++ fevent.nGoodPrimaryTracks;
        float phi { Ptrack.phi() };

        phi_sum += phi;
        eta_sum += eta;
        sumpt += pt;

        if (pt > maxpt) { maxpt = pt; phi_lead = phi; eta_lead = eta; }
    }
    /* cout << "nPrimary " << nPrimary << endl; */
    fevent.nTofMatch = nTof;
    fevent.nPrimaryTracks = nPrimary;
    fevent.maxpt = maxpt;
    fevent.sumpt = sumpt;
    fevent.phiTrkLead = phi_lead;
    fevent.etaTrkLead = eta_lead;
    fevent.phiTrkMean = phi_sum / fevent.nGoodPrimaryTracks;
    fevent.etaTrkMean = eta_sum / fevent.nGoodPrimaryTracks;
    fevent.goodTrkRatio = (double) fevent.nGoodPrimaryTracks / fevent.nPrimaryTracks;

    // look through the towers
    fevent.ntowTriggers = mPicoDst->numberOfEmcTriggers();
    fevent.nHT1trigs = 0;
    fevent.nHT2trigs = 0;
    fevent.maxEt = 0;
    fevent.phiEt = 0;
    fevent.etaEt = 0;
    fevent.maxEt_is_maxAdc = true;
    fevent.maxTowAdc = 0;

    phi_sum = 0;
    eta_sum = 0;

    double sumEt {0};
    double sumTowAdc {0};

    for (int i=0 ; i < fevent.ntowTriggers; ++i){
        StPicoEmcTrigger* emcTrig = mPicoDst->emcTrigger(i);
        bool isHT1 { emcTrig->isHT1() };
        bool isHT2 { emcTrig->isHT2() };

        if (isHT1) ++fevent.nHT1trigs;
        if (isHT2) ++fevent.nHT2trigs;

        if (!isHT1 && !isHT2) continue;
        /* fevent.trigId = emcTrig->id(); */
        int   trigId  = emcTrig->id();
        if (trigId < 1) {
            fprintf(flog,"Warning! in ev(%i) emcTriggerId(%i) (should be > 1)\n", 
                    trigId, mevent->eventId());
            continue;
        }
        StPicoBTowHit* bTowHit = mPicoDst->btowHit(trigId-1); 
        StEmcPosition *mPosition = new StEmcPosition();
        // thanks to Joel Mazer
        StThreeVectorF towLoc = mPosition->getPosFromVertex(mevent->primaryVertex(), trigId);
        double eta { towLoc.pseudoRapidity() };
        double phi { towLoc.phi() };
        float towEt = bTowHit->energy() / ((double)TMath::CosH(towLoc.pseudoRapidity()));
        if (towEt > 30) {
            fprintf(flog, "Run(%i) Event(%i) cut b/c tower > 30 GeV at (%f GeV)\n",
                    fevent.runId, mevent->eventId(), towEt);
            return kStOK; // cut out this event
        }
        sumEt += towEt;
        sumTowAdc += bTowHit->adc();


        bool isEtMax{false};
        bool isAdcMax{false};

        phi_sum += phi;
        eta_sum += eta;
        if (towEt > fevent.maxEt) { 
            fevent.maxEt = towEt; 
            fevent.phiEt = phi;
            fevent.etaEt = eta; 
            isEtMax = true;
            fevent.trigId = trigId;
        }
        if (bTowHit->adc() > fevent.maxTowAdc) {
            fevent.maxTowAdc = bTowHit->adc();
            isAdcMax = true;
        }
        if (isAdcMax || isEtMax) {
            if (isAdcMax ^ isEtMax) fevent.maxEt_is_maxAdc = true;
            else                    fevent.maxEt_is_maxAdc = false;
        }
    }
    fevent.phiEtMean = phi_sum / (fevent.nHT1trigs + fevent.nHT2trigs);
    fevent.etaEtMean = eta_sum / (fevent.nHT1trigs + fevent.nHT2trigs);
    fevent.sumEt = sumEt;
    fevent.sumTowAdc = sumTowAdc;

    ftree->Fill();
    fEventsProcessed++ ;
	return kStOK;
}

