#ifndef DETSEGMENTATION_ENDCAPTURBINE_H
#define DETSEGMENTATION_ENDCAPTURBINE_H

// FCCSW
//#include "detectorSegmentations/GridTheta_k4geo.h"
#include "DDSegmentation/Segmentation.h"
#include "TVector3.h"
#include <cmath>

/** FCCSWEndcapTurbine_k4geo 
 *
 *  Segmentation for turbine-style endcap calorimeter
 *
 */

namespace dd4hep {
namespace DDSegmentation {
class FCCSWEndcapTurbine_k4geo : public Segmentation {
public:
  /// default constructor using an arbitrary type
  FCCSWEndcapTurbine_k4geo(const std::string& aCellEncoding);
  /// Default constructor used by derived classes passing an existing decoder
  FCCSWEndcapTurbine_k4geo(const BitFieldCoder* decoder);

  /// destructor
  virtual ~FCCSWEndcapTurbine_k4geo() = default;

  /**  Determine the global position based on the cell ID. **/
  virtual Vector3D position(const CellID& aCellID) const;
  /**  Determine the cell ID based on the position.
   *   @param[in] aLocalPosition (not used).
   *   @param[in] aGlobalPosition position in the global coordinates.
   *   @param[in] aVolumeId ID of a volume.
   *   return Cell ID.
   */
  virtual CellID cellID(const Vector3D& aLocalPosition, const Vector3D& aGlobalPosition,
                        const VolumeID& aVolumeID) const;
  /**  Determine the azimuthal angle based on the cell ID.
   *   @param[in] aCellId ID of a cell.
   *   return Phi.
   */
  double phi(const CellID& aCellID) const;

  /**  Get the coordinate offset in azimuthal angle.
   *   return The offset in phi.
   */
  inline double offsetPhi() const { return m_offsetPhi; }
  /**  Get the field name for azimuthal angle.
   *   return The field name for phi.
   */
  inline const std::string& fieldNamePhi() const { return m_phiID; }
  /**  Set the number of bins in azimuthal angle.
   *   @param[in] aNumberBins Number of bins in phi.
   */
  inline void setPhiBins(int bins) { m_phiBins = bins; }
  /**  Set the coordinate offset in azimuthal angle.
   *   @param[in] aOffset Offset in phi.
   */
  inline void setOffsetPhi(double offset) { m_offsetPhi = offset; }
  /**  Set the field name used for phi.
   *   @param[in] aFieldName Field name for phi.
   */
  inline void setFieldNamePhi(const std::string& fieldName) { m_phiID = fieldName; }
  /**  Determine the transverse distance from the beamline rho based on the cell ID.
   *   @param[in] aCellId ID of a cell.
   *   return rho.
   */
  double rho(const CellID& aCellID) const;
   /**  Get the coordinate offset in rho.
   *   return The offset in rho.
   */
  inline double offsetRho() const { return m_offsetRho; }
  /**  Get the field name for rho.
   *   return The field name for rho.
   */
  inline const std::string& fieldNameRho() const { return m_rhoID; }
  /**  Set the number of bins in rho.
   *   @param[in] aNumberBins Number of bins in rho.
   */
  inline void setRhoBins(int bins) { m_rhoBins = bins; }
  /**  Set the coordinate offset in rho.
   *   @param[in] aOffset Offset in rho.
   */
  inline void setOffsetRho(double offset) { m_offsetRho = offset; }
 /**  Set the field name used for transverse distance from IP rho.
   *   @param[in] aFieldName Field name for rho.
   */
  inline void setFieldNameRho(const std::string& fieldName) { m_rhoID = fieldName; }

  /**  Determine the x coordinate based on the cell ID.
   *   @param[in] aCellId ID of a cell.
   *   return x.
   */
  double x(const CellID& aCellID) const;
  /**  Determine the z coordinate based on the cell ID.
   *   @param[in] aCellId ID of a cell.
   *   return z.
   */
  double z(const CellID& aCellID) const;
  /**  Get the coordinate offset in z.
   *   return The offset in z.
   */
  inline double offsetZ() const { return m_offsetZ; }
  /**  Get the field name for z.
   *   return The field name for z.
   */
  inline const std::string& fieldNameZ() const { return m_zID; }
  /**  Set the number of bins in z.
   *   @param[in] aNumberBins Number of bins in z.
   */
  inline void setZBins(int bins) { m_zBins = bins; }
  /**  Set the coordinate offset in z.
   *   @param[in] aOffset Offset in z.
   */
  inline void setOffsetZ(double offset) { m_offsetZ = offset; }
  /**  Set the field name used for z.
   *   @param[in] aFieldName Field name for z.
   */
  inline void setFieldNameZ(const std::string& fieldName) { m_zID = fieldName; }
  inline double rhoFromXYZ(const Vector3D& aposition) const {
    TVector3 vec(aposition.X, aposition.Y, aposition.Z);
    return vec.Perp();
  }

protected:
  /// turbine blade angle
  double m_bladeAngle;
  /// the number of bins in phi
  int m_phiBins;
  /// the coordinate offset in phi
  double m_offsetPhi;
  /// the field name used for phi
  std::string m_phiID; 
  /// the number of bins in rho
  int m_rhoBins;
  ////grid size in rho
  double m_gridSizeRho;
  /// the coordinate offset in rho
  double m_offsetRho;
  /// the field name used for rho
  std::string m_rhoID;
  /// the number of bins in z
  int m_zBins;
  ///grid size in z
  double m_gridSizeZ; 
  /// the coordinate offset in z
  double m_offsetZ;
  /// the field name used for z
  std::string m_zID; 
  std::string m_sideID;
};
}
}
#endif /* DETSEGMENTATION_ENDCAPTURBINE_H */
