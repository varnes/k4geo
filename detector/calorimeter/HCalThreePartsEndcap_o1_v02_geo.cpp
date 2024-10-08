// DD4hep
#include "DD4hep/DetFactoryHelper.h"
#include <DDRec/DetectorData.h>

// todo: remove gaudi logging and properly capture output
#define endmsg std::endl
#define lLog std::cout
namespace MSG {
const std::string ERROR = "createHCalThrePartsEndcap   ERROR  ";
const std::string DEBUG = "createHCalThrePartsEndcap   DEBUG  ";
const std::string INFO  = "createHCalThrePartsEndcap   INFO   ";
}

using dd4hep::Volume;
using dd4hep::DetElement;
using dd4hep::xml::Dimension;
using dd4hep::PlacedVolume;
using xml_comp_t = dd4hep::xml::Component;
using xml_det_t = dd4hep::xml::DetElement;
using xml_h = dd4hep::xml::Handle_t;

namespace det {
static dd4hep::Ref_t createHCalEC(dd4hep::Detector& lcdd, xml_h xmlElement, dd4hep::SensitiveDetector sensDet){
    xml_det_t xmlDet = xmlElement;
    std::string detName = xmlDet.nameStr();

    // Make DetElement
    dd4hep::DetElement caloDetElem(detName, xmlDet.id());

    // Make volume that envelopes the whole endcap; set material to air
    Dimension dimensions(xmlDet.dimensions());

    dd4hep::Tube envelope(dimensions.rmin(), dimensions.rmax1(), (dimensions.v_offset() + dimensions.z_length()));
    dd4hep::Tube negative1(dimensions.rmin(), dimensions.rmax1(), (dimensions.offset() - dimensions.width()));
    dd4hep::Tube negative2(dimensions.rmin(), dimensions.rmin1(), (dimensions.z_offset() - dimensions.dz()));
    dd4hep::Tube negative3(dimensions.rmin(), dimensions.rmin2(), (dimensions.v_offset() - dimensions.z_length()));

    dd4hep::SubtractionSolid envelopeShapeTmp1(envelope, negative1);
    dd4hep::SubtractionSolid envelopeShapeTmp2(envelopeShapeTmp1, negative2);
    dd4hep::SubtractionSolid envelopeShape(envelopeShapeTmp2, negative3);

    Volume envelopeVolume(detName + "_volume", envelopeShape, lcdd.air());
    envelopeVolume.setVisAttributes(lcdd, dimensions.visStr());

    // Set sensitive detector type
    Dimension sensDetType = xmlElement.child(_Unicode(sensitive));
    sensDet.setType(sensDetType.typeStr());

    xml_comp_t xEndPlate = xmlElement.child(_Unicode(end_plate));
    double dZEndPlate = xEndPlate.thickness() / 2.;
    xml_comp_t xFacePlate = xmlElement.child(_Unicode(face_plate));
    double dRhoFacePlate = xFacePlate.thickness() / 2.;
    xml_comp_t xSpace = xmlElement.child(_Unicode(plate_space));  // to avoid overlaps
    double space = xSpace.thickness();
    xml_comp_t xSteelSupport = xmlElement.child(_Unicode(steel_support));
    double dSteelSupport = xSteelSupport.thickness();
    lLog << MSG::DEBUG << "steel support thickness " << dSteelSupport << endmsg;
    lLog << MSG::DEBUG << "steel support material  " << xSteelSupport.materialStr() << endmsg;

    // Calculate sensitive barrel dimensions
    double sensitiveBarrel1Rmin = dimensions.rmin1() + 2 * dRhoFacePlate + space;
    double sensitiveBarrel2Rmin = dimensions.rmin2() + 2 * dRhoFacePlate + space;
    double sensitiveBarrel3Rmin = dimensions.rmin() + 2 * dRhoFacePlate + space;

    // Offset in z is given as distance from 0 to the middle of the Calorimeter volume
    double extBarrelOffset1 = dimensions.offset();
    double extBarrelOffset2 = dimensions.z_offset();
    double extBarrelOffset3 = dimensions.v_offset();

    // Hard-coded assumption that we have two different sequences for the modules
    std::vector<xml_comp_t> sequences = {xmlElement.child(_Unicode(sequence_a)), xmlElement.child(_Unicode(sequence_b))};
    // Check if both sequences are present
    if (!sequences[0] || !sequences[1]) {
      lLog << MSG::ERROR << "The two sequences sequence_a and sequence_b must be present in the xml file." << endmsg;
      throw std::runtime_error("Missing sequence_a or sequence_b in the xml file.");
    }
    // Check if both sequences have the same dimensions
    Dimension dimensionsA(sequences[0].dimensions());
    Dimension dimensionsB(sequences[1].dimensions());
    if (dimensionsA.dz() != dimensionsB.dz()) {
        lLog << MSG::ERROR << "The dimensions of sequence_a and sequence_b do not match." << endmsg;
        throw std::runtime_error("The dimensions of the sequence_a and sequence_b do not match.");
    }
    double dzSequence = dimensionsB.dz();
    lLog << MSG::DEBUG << "sequence thickness " << dzSequence << endmsg;

    // calculate the number of modules fitting in  Z
    unsigned int numSequencesZ1 = static_cast<unsigned>((2 * dimensions.width() - 2 * dZEndPlate - space) / dzSequence);
    unsigned int numSequencesZ2 = static_cast<unsigned>((2 * dimensions.dz() - 2 * dZEndPlate - space) / dzSequence);
    unsigned int numSequencesZ3 = static_cast<unsigned>((2 * dimensions.z_length() - 2 * dZEndPlate - space) / dzSequence);

    unsigned int numLayersR1 = 0;
    unsigned int numLayersR2 = 0;
    unsigned int numLayersR3 = 0;
    double moduleDepth1 = 0.;
    double moduleDepth2 = 0.;
    double moduleDepth3 = 0.;

    std::vector<double> layerDepths1 = std::vector<double>();
    std::vector<double> layerDepths2 = std::vector<double>();
    std::vector<double> layerDepths3 = std::vector<double>();

    std::vector<double> layerInnerRadii1 = std::vector<double>();
    std::vector<double> layerInnerRadii2 = std::vector<double>();
    std::vector<double> layerInnerRadii3 = std::vector<double>();

    // iterating over XML elements to retrieve all child elements of 'layers'
    for (xml_coll_t xCompColl(xmlElement.child(_Unicode(layers)), _Unicode(layer)); xCompColl; ++xCompColl){
        xml_comp_t currentLayer = xCompColl;
        Dimension layerDimension(currentLayer.dimensions());
        numLayersR1 += layerDimension.nmodules();
        numLayersR2 += layerDimension.nsegments();
        numLayersR3 += layerDimension.nPads();

        for (int nLayer = 0; nLayer < layerDimension.nmodules(); nLayer++){
            moduleDepth1 += layerDimension.dr();
            layerDepths1.push_back(layerDimension.dr());
        }
        for (int nLayer = 0; nLayer < layerDimension.nsegments(); nLayer++){
            moduleDepth2 += layerDimension.dr();
            layerDepths2.push_back(layerDimension.dr());
        }
        for (int nLayer = 0; nLayer < layerDimension.nPads(); nLayer++){
            moduleDepth3 += layerDimension.dr();
            layerDepths3.push_back(layerDimension.dr());
        }
    }

    lLog << MSG::DEBUG << "retrieved number of layers in first Endcap part:  " << numLayersR1 << " , which end up to a full module depth in rho of " << moduleDepth1 << " cm" << endmsg;
    lLog << MSG::DEBUG << "retrieved number of layers in second Endcap part:  " << numLayersR2 << " , which end up to a full module depth in rho of " << moduleDepth2 << " cm" << endmsg;
    lLog << MSG::DEBUG << "retrieved number of layers in third Endcap part:  " << numLayersR3 << " , which end up to a full module depth in rho of " << moduleDepth3 << " cm" << endmsg;

    lLog << MSG::INFO << "constructing first part EC: with z offset " << extBarrelOffset1 << " cm: "<< numSequencesZ1 << " sequences in Z, " << numLayersR1 << " layers in Rho, " << numLayersR1 * numSequencesZ1 << " tiles" << endmsg;
    lLog << MSG::INFO << "constructing second part EC: with offset " << extBarrelOffset2 << " cm: " << numSequencesZ2 << " sequences in Z, " << numLayersR2 << " layers in Rho, " << layerDepths2.size() * numSequencesZ2 << " tiles" << endmsg;

    lLog << MSG::INFO << "constructing third part EC: with offset " << extBarrelOffset3 << " cm: " << numSequencesZ3 << " sequences in Z, " << numLayersR3 << " layers in Rho, " << layerDepths3.size() * numSequencesZ3 << " tiles" << endmsg;

    lLog << MSG::INFO << "number of channels: " << (numLayersR1 * numSequencesZ1) + (numLayersR2 * numSequencesZ2) + (numLayersR3 * numSequencesZ3) << endmsg;

    // Calculate correction along z based on the module size (can only have natural number of modules)
    double dzDetector1 = (numSequencesZ1 * dzSequence) / 2 + 2 * dZEndPlate + space;
    double dzDetector2 = (numSequencesZ2 * dzSequence) / 2;
    double dzDetector3 = (numSequencesZ3 * dzSequence) / 2 + 2 * dZEndPlate + space;

    lLog << MSG::INFO << "correction of dz (negative = size reduced) first part EC :" << dzDetector1*2 - dimensions.width()*2 << endmsg;
    lLog << MSG::INFO << "dz second part EC:" << dzDetector2 * 2 << endmsg;
    lLog << MSG::INFO << "width second part EC:" << dimensions.dz() * 2 << endmsg;
    lLog << MSG::INFO << "correction of dz (negative = size reduced) second part EB:" << dzDetector2*2 - dimensions.dz()*2 << endmsg;

    lLog << MSG::INFO << "dz third part EC:" << dzDetector3 * 2 << endmsg;


    for (int iSign = -1; iSign < 2; iSign+=2){
        int sign; 
        if(iSign < 0){
            sign = -1;
            lLog << MSG::DEBUG << "Placing detector on the negative side: (cm) " << -(dimensions.offset() + dimensions.dz()) << endmsg;
        }
        else{
            sign = +1;
            lLog << MSG::DEBUG << "Placing detector on the positive side: (cm) " << (dimensions.offset() + dimensions.dz()) << endmsg;
        }
        // Add structural support made of steel inside of HCal
        DetElement facePlate1(caloDetElem, "FacePlate_" + std::to_string(1 * sign), 0);
        dd4hep::Tube facePlateShape1(dimensions.rmin1(), (sensitiveBarrel1Rmin - space), (dzDetector1 - 2 * dZEndPlate - space));
        Volume facePlateVol1("facePlateVol1", facePlateShape1, lcdd.material(xFacePlate.materialStr()));
        facePlateVol1.setVisAttributes(lcdd, xFacePlate.visStr());
        dd4hep::Position offsetFace1(0, 0, sign * extBarrelOffset1);

        // Faceplate for 2nd part of extended Barrel
        DetElement facePlate2(caloDetElem, "FacePlate_" + std::to_string(2 * sign), 0);
        dd4hep::Tube facePlateShape2(dimensions.rmin2(), (sensitiveBarrel2Rmin - space),
                                    dzDetector2);
        Volume facePlateVol2("facePlateVol2", facePlateShape2, lcdd.material(xFacePlate.materialStr()));
        facePlateVol2.setVisAttributes(lcdd, xFacePlate.visStr());
        dd4hep::Position offsetFace2(0, 0, sign * extBarrelOffset2);

        // Faceplate for 3rd part of extended Barrel
        DetElement facePlate3(caloDetElem, "FacePlate_" + std::to_string(3 * sign), 0);
        dd4hep::Tube facePlateShape3(dimensions.rmin(), (sensitiveBarrel3Rmin - space),
                                    (dzDetector3 - 2 * dZEndPlate - space));
        Volume facePlateVol3("facePlateVol3", facePlateShape3, lcdd.material(xFacePlate.materialStr()));
        facePlateVol3.setVisAttributes(lcdd, xFacePlate.visStr());
        dd4hep::Position offsetFace3(0, 0, sign * extBarrelOffset3);

        PlacedVolume placedFacePlate1 = envelopeVolume.placeVolume(facePlateVol1, offsetFace1);
        facePlate1.setPlacement(placedFacePlate1);
        PlacedVolume placedFacePlate2 = envelopeVolume.placeVolume(facePlateVol2, offsetFace2);
        facePlate2.setPlacement(placedFacePlate2);
        PlacedVolume placedFacePlate3 = envelopeVolume.placeVolume(facePlateVol3, offsetFace3);
        facePlate3.setPlacement(placedFacePlate3);

        // Add structural support made of steel at both ends of extHCal
        dd4hep::Tube endPlateShape1(dimensions.rmin1(), (dimensions.rmax1() - dSteelSupport), dZEndPlate);
        Volume endPlateVol1("endPlateVol1", endPlateShape1, lcdd.material(xEndPlate.materialStr()));
        endPlateVol1.setVisAttributes(lcdd, xEndPlate.visStr());
        dd4hep::Tube endPlateShape3(dimensions.rmin(), (dimensions.rmax() - dSteelSupport), dZEndPlate);
        Volume endPlateVol3("endPlateVol3", endPlateShape3, lcdd.material(xEndPlate.materialStr()));
        endPlateVol3.setVisAttributes(lcdd, xEndPlate.visStr());

        // Endplates placed for the extended Barrels in front and in the back to the central Barrel
        DetElement endPlatePos(caloDetElem, "endPlate_" + std::to_string(1 * sign), 0);
        dd4hep::Position posOffset(0, 0, sign * (extBarrelOffset3 + dzDetector3 - dZEndPlate));
        PlacedVolume placedEndPlatePos = envelopeVolume.placeVolume(endPlateVol3, posOffset);
        endPlatePos.setPlacement(placedEndPlatePos);

        DetElement endPlateNeg(caloDetElem, "endPlate_" + std::to_string(2 * sign), 0);
        dd4hep::Position negOffset(0, 0, sign * (extBarrelOffset1 - dzDetector1 + dZEndPlate));
        PlacedVolume placedEndPlateNeg = envelopeVolume.placeVolume(endPlateVol1, negOffset);
        endPlateNeg.setPlacement(placedEndPlateNeg);

        std::vector<dd4hep::PlacedVolume> layers;
        layers.reserve(layerDepths1.size()+layerDepths2.size()+layerDepths3.size());
        std::vector<std::vector<dd4hep::PlacedVolume> > seqInLayers;
        seqInLayers.reserve(layerDepths1.size()+layerDepths2.size()+layerDepths3.size());
        std::vector<dd4hep::PlacedVolume> tilesPerLayer;
        tilesPerLayer.reserve(layerDepths1.size()+layerDepths2.size()+layerDepths3.size());

        // loop over R ("layers")
        double layerR = 0.;
        for (unsigned int idxLayer = 0; idxLayer < layerDepths1.size(); ++idxLayer){
            // in Module rmin = 0  for first wedge, changed radius to the full radius starting at (0,0,0)
            double rminLayer = sensitiveBarrel1Rmin + layerR;
            double rmaxLayer = sensitiveBarrel1Rmin + layerR + layerDepths1.at(idxLayer);
            layerR += layerDepths1.at(idxLayer);
            layerInnerRadii1.push_back(rminLayer);

            //alternate: even layers consist of tile sequence b, odd layer of tile sequence a
            unsigned int sequenceIdx = (idxLayer+1) % 2;
         
            dd4hep::Tube tileSequenceShape(rminLayer, rmaxLayer, 0.5*dzSequence);
            Volume tileSequenceVolume("HCalECTileSequenceVol1", tileSequenceShape, lcdd.air());

            lLog << MSG::DEBUG << "layer radii:  " << rminLayer << " - " << rmaxLayer << " [cm]" << endmsg;

            dd4hep::Tube layerShape(rminLayer, rmaxLayer, dzDetector1 );
            Volume layerVolume("HCalECLayerVol1", layerShape, lcdd.air());

            layerVolume.setVisAttributes(lcdd.invisible());
            unsigned int idxSubMod = 0;

            dd4hep::Position moduleOffset1 (0,0,sign * extBarrelOffset1);

            dd4hep::PlacedVolume placedLayerVolume = envelopeVolume.placeVolume(layerVolume, moduleOffset1);
            unsigned int type1 = 0;
            if (sign<0){
                type1 = 3;
            }
            placedLayerVolume.addPhysVolID("type", type1);  // First module type=0,3 in front of second +/-
            placedLayerVolume.addPhysVolID("layer", idxLayer);

            layers.push_back(placedLayerVolume);
   
            double tileZOffset = - 0.5* dzSequence;

            // first Z loop (tiles that make up a sequence)
            for (xml_coll_t xCompColl(sequences[sequenceIdx], _Unicode(module_component)); xCompColl; ++xCompColl, ++idxSubMod){
                xml_comp_t xComp = xCompColl;
                dd4hep::Tube tileShape(rminLayer, rmaxLayer, 0.5 * xComp.thickness());
           
                Volume tileVol("HCalECTileVol_"+ xComp.materialStr(), tileShape, lcdd.material(xComp.materialStr()));
                tileVol.setVisAttributes(lcdd, xComp.visStr());
           
                dd4hep::Position tileOffset(0, 0, tileZOffset + 0.5 * xComp.thickness() );
                dd4hep::PlacedVolume placedTileVol = tileSequenceVolume.placeVolume(tileVol, tileOffset);
           
                if (xComp.isSensitive()){
                    tileVol.setSensitiveDetector(sensDet);
                    tilesPerLayer.push_back(placedTileVol);
                }
                tileZOffset += xComp.thickness();
            }

            // second z loop (place sequences in layer)
            std::vector<dd4hep::PlacedVolume> seqs; 
            double zOffset = - dzDetector1 + 0.5 * dzSequence; //2*dZEndPlate + space + 0.5 * dzSequence;
         
            for (uint numSeq=0; numSeq < numSequencesZ1; numSeq++){
                dd4hep::Position tileSequencePosition(0, 0, zOffset);
                dd4hep::PlacedVolume placedTileSequenceVolume = layerVolume.placeVolume(tileSequenceVolume, tileSequencePosition);
                placedTileSequenceVolume.addPhysVolID("row", numSeq);
                seqs.push_back(placedTileSequenceVolume);
                zOffset += dzSequence;
            }
            seqInLayers.push_back(seqs);
        } // layers loop


        layerR = 0.;
        // Placement of subWedges in Wedge, 2nd part
        for (unsigned int idxLayer = 0; idxLayer < layerDepths2.size(); ++idxLayer){
            // in Module rmin = 0  for first wedge, changed radius to the full radius starting at (0,0,0)                                                                      
            double rminLayer = sensitiveBarrel2Rmin + layerR;
            double rmaxLayer = sensitiveBarrel2Rmin + layerR + layerDepths2.at(idxLayer);
            layerR += layerDepths2.at(idxLayer);
            layerInnerRadii2.push_back(rminLayer);

            //alternate: even layers consist of tile sequence b, odd layer of tile sequence a                                                                                  
            unsigned int sequenceIdx = (idxLayer+1) % 2;

            dd4hep::Tube tileSequenceShape(rminLayer, rmaxLayer, 0.5*dzSequence);
            Volume tileSequenceVolume("HCalECTileSequenceVol2", tileSequenceShape, lcdd.air());

            lLog << MSG::DEBUG << "layer radii:  " << rminLayer << " - " << rmaxLayer << " [cm]" << endmsg;

            dd4hep::Tube layerShape(rminLayer, rmaxLayer, dzDetector2);
            Volume layerVolume("HCalECLayerVol2", layerShape, lcdd.air());

            layerVolume.setVisAttributes(lcdd.invisible());
            unsigned int idxSubMod = 0;

            double tileZOffset = - 0.5* dzSequence;

            // first Z loop (tiles that make up a sequence)                                                                                                                    
            for (xml_coll_t xCompColl(sequences[sequenceIdx], _Unicode(module_component)); xCompColl; ++xCompColl, ++idxSubMod){
                xml_comp_t xComp = xCompColl;
                dd4hep::Tube tileShape(rminLayer, rmaxLayer, 0.5 * xComp.thickness());

                Volume tileVol("HCalECTileVol_", tileShape, lcdd.material(xComp.materialStr()));
                tileVol.setVisAttributes(lcdd, xComp.visStr());

                dd4hep::Position tileOffset(0, 0, tileZOffset + 0.5 * xComp.thickness() );
                dd4hep::PlacedVolume placedTileVol = tileSequenceVolume.placeVolume(tileVol, tileOffset);

                if (xComp.isSensitive()){
                    tileVol.setSensitiveDetector(sensDet);
                    tilesPerLayer.push_back(placedTileVol);
                }
                tileZOffset += xComp.thickness();
            } // close first Z loop 

            // second z loop (place sequences in layer)                                                                                                                        
            std::vector<dd4hep::PlacedVolume> seqs;
            double zOffset = - dzDetector2 + 0.5 * dzSequence; //(dzSequence * 0.5);

            for (uint numSeq=0; numSeq < numSequencesZ2; numSeq++){
                dd4hep::Position tileSequencePosition(0, 0, zOffset);
                dd4hep::PlacedVolume placedTileSequenceVolume = layerVolume.placeVolume(tileSequenceVolume, tileSequencePosition);
                placedTileSequenceVolume.addPhysVolID("row", numSeq);
                seqs.push_back(placedTileSequenceVolume);
                zOffset += dzSequence;
            }
            seqInLayers.push_back(seqs);

            dd4hep::Position moduleOffset2 (0, 0, sign * extBarrelOffset2);
            dd4hep::PlacedVolume placedLayerVolume = envelopeVolume.placeVolume(layerVolume, moduleOffset2);
            unsigned int type2 = 1;
            if (sign<0){
                type2 = 4;
            }
            placedLayerVolume.addPhysVolID("type", type2);  // Second module type=1,4 behind the first +/-
            placedLayerVolume.addPhysVolID("layer", layerDepths1.size() + idxLayer);
            layers.push_back(placedLayerVolume);
        }

        layerR = 0.;
        // Placement of subWedges in Wedge, 3th part
        for (unsigned int idxLayer = 0; idxLayer < layerDepths3.size(); ++idxLayer){
            // in Module rmin = 0  for first wedge, changed radius to the full radius starting at (0,0,0)                                                                      
            double rminLayer = sensitiveBarrel3Rmin + layerR;
            double rmaxLayer = sensitiveBarrel3Rmin + layerR + layerDepths3.at(idxLayer);
            layerR += layerDepths3.at(idxLayer);
            layerInnerRadii3.push_back(rminLayer);

         
            //alternate: even layers consist of tile sequence b, odd layer of tile sequence a                                                                                  
            unsigned int sequenceIdx = (idxLayer+1) % 2;

            dd4hep::Tube tileSequenceShape(rminLayer, rmaxLayer, 0.5*dzSequence);
            Volume tileSequenceVolume("HCalECTileSequenceVol3", tileSequenceShape, lcdd.air());

            lLog << MSG::DEBUG << "layer radii:  " << rminLayer << " - " << rmaxLayer << " [cm]" << endmsg;

            dd4hep::Tube layerShape(rminLayer, rmaxLayer, dzDetector3);
            Volume layerVolume("HCalECLayerVol3", layerShape, lcdd.air());

            layerVolume.setVisAttributes(lcdd.invisible());
            unsigned int idxSubMod = 0;

            double tileZOffset = - 0.5* dzSequence;

            // first Z loop (tiles that make up a sequence)                                                                                                                    
            for (xml_coll_t xCompColl(sequences[sequenceIdx], _Unicode(module_component)); xCompColl; ++xCompColl, ++idxSubMod){
                xml_comp_t xComp = xCompColl;
                dd4hep::Tube tileShape(rminLayer, rmaxLayer, 0.5 * xComp.thickness());
           
                Volume tileVol("HCalECTileVol_" , tileShape, lcdd.material(xComp.materialStr()));
                tileVol.setVisAttributes(lcdd, xComp.visStr());

                dd4hep::Position tileOffset(0, 0, tileZOffset + 0.5 * xComp.thickness() );
                dd4hep::PlacedVolume placedTileVol = tileSequenceVolume.placeVolume(tileVol, tileOffset);

                if (xComp.isSensitive()){
                    tileVol.setSensitiveDetector(sensDet);
                    tilesPerLayer.push_back(placedTileVol);
                }
                tileZOffset += xComp.thickness();
            }
       
            // second z loop (place sequences in layer)
            std::vector<dd4hep::PlacedVolume> seqs;
            double zOffset = - dzDetector3 + 0.5 * dzSequence; //2*dZEndPlate + space + (dzSequence * 0.5);

            for (uint numSeq=0; numSeq < numSequencesZ3; numSeq++){
                dd4hep::Position tileSequencePosition(0, 0, zOffset);
                dd4hep::PlacedVolume placedTileSequenceVolume = layerVolume.placeVolume(tileSequenceVolume, tileSequencePosition);
                placedTileSequenceVolume.addPhysVolID("row", numSeq);
                seqs.push_back(placedTileSequenceVolume);
                zOffset += dzSequence;
            }
            seqInLayers.push_back(seqs);

            dd4hep::Position moduleOffset3 (0, 0, sign * extBarrelOffset3);
            dd4hep::PlacedVolume placedLayerVolume = envelopeVolume.placeVolume(layerVolume, moduleOffset3);
            unsigned int type3 = 2;
            if (sign<0){
                type3 = 5;
            }
            placedLayerVolume.addPhysVolID("type", type3);  // Second module type=2,5 behind the first +/-
            placedLayerVolume.addPhysVolID("layer", layerDepths1.size() + layerDepths2.size() + idxLayer);
            layers.push_back(placedLayerVolume);
        } // end loop placement of subwedges

        // Placement of DetElements
        lLog << MSG::DEBUG << "Layers in r :    " << layers.size() << std::endl;
        lLog << MSG::DEBUG << "Tiles in layers :" << tilesPerLayer.size() << std::endl;

        // Place det elements wihtin each other to recover volume positions later via cellID  
        for (uint iLayer = 0; iLayer < (layerDepths1.size()+layerDepths2.size()+layerDepths3.size()); iLayer++){
            DetElement layerDet(caloDetElem, dd4hep::xml::_toString(sign*(iLayer+1), "layer%d"), sign*(iLayer+1));
            layerDet.setPlacement(layers[iLayer]);
         
            for (uint iSeq = 0; iSeq < seqInLayers[iLayer].size(); iSeq++){
                DetElement seqDet(layerDet, dd4hep::xml::_toString(iSeq, "seq%d"), sign*(iSeq+1));
                seqDet.setPlacement(seqInLayers[iLayer][iSeq]);

                DetElement tileDet(seqDet, dd4hep::xml::_toString(iSeq, "tile%d"), sign*(iSeq+1));
                tileDet.setPlacement(tilesPerLayer[iLayer]);
            }
        }
    } // for signs loop 

    // Place envelope volume
    Volume motherVol = lcdd.pickMotherVolume(caloDetElem);

    PlacedVolume placedHCal = motherVol.placeVolume(envelopeVolume);
    placedHCal.addPhysVolID("system", caloDetElem.id());
    caloDetElem.setPlacement(placedHCal);


    // Create caloData object
    auto caloData = new dd4hep::rec::LayeredCalorimeterData;
    caloData->layoutType = dd4hep::rec::LayeredCalorimeterData::EndcapLayout;
    caloDetElem.addExtension<dd4hep::rec::LayeredCalorimeterData>(caloData);

    caloData->extent[0] = sensitiveBarrel3Rmin;
    caloData->extent[1] = sensitiveBarrel3Rmin + moduleDepth3; // 
    caloData->extent[2] = 0.;      // NN: for barrel detectors this is 0
    caloData->extent[3] = dzDetector1 + dzDetector2 + dzDetector3; 

    dd4hep::rec::LayeredCalorimeterData::Layer caloLayer;

    // IMPORTANT: the information below is used to calculate the cell position in CellPositionsHCalPhiThetaSegTool in k4RecCalorimeter
    // if the definition distance or sensitive_thickness is modified, one also needs to modify CellPositionsHCalPhiThetaSegTool
    for (unsigned int idxLayer = 0; idxLayer < layerDepths1.size(); ++idxLayer) {
        const double difference_bet_r1r2 = layerDepths1.at(idxLayer); 
        caloLayer.distance                  = layerInnerRadii1.at(idxLayer); // radius of the current layer 
        caloLayer.sensitive_thickness       = difference_bet_r1r2;           // radial dimension of the current layer
        caloLayer.inner_thickness           = difference_bet_r1r2 / 2.0;
        caloLayer.outer_thickness           = difference_bet_r1r2 / 2.0;

        caloData->layers.push_back(caloLayer);
    }

    for (unsigned int idxLayer = 0; idxLayer < layerDepths2.size(); ++idxLayer) {
        const double difference_bet_r1r2 = layerDepths2.at(idxLayer); 
        caloLayer.distance                  = layerInnerRadii2.at(idxLayer);
        caloLayer.sensitive_thickness       = difference_bet_r1r2; 
        caloLayer.inner_thickness           = difference_bet_r1r2 / 2.0;
        caloLayer.outer_thickness           = difference_bet_r1r2 / 2.0;

        caloData->layers.push_back(caloLayer);
    }

    for (unsigned int idxLayer = 0; idxLayer < layerDepths3.size(); ++idxLayer) {
        const double difference_bet_r1r2 = layerDepths3.at(idxLayer); 
        caloLayer.distance                  = layerInnerRadii3.at(idxLayer); 
        caloLayer.sensitive_thickness       = difference_bet_r1r2; 
        caloLayer.inner_thickness           = difference_bet_r1r2 / 2.0;
        caloLayer.outer_thickness           = difference_bet_r1r2 / 2.0;

        caloData->layers.push_back(caloLayer);
    }
    return caloDetElem; 
}  
}// namespace det
DECLARE_DETELEMENT(CaloThreePartsEndcap_o1_v02, det::createHCalEC)
