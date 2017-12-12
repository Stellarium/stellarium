/// \file BasicMathPlugin.cpp
/// \author Roger James
/// \date 13th November 2013

#include "BasicMathPlugin.h"

#include "DriverCommon.h"

#include <libnova/julian_day.h>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_linalg.h>

#include <limits>
#include <iostream>
#include <map>

namespace INDI
{
namespace AlignmentSubsystem
{
BasicMathPlugin::BasicMathPlugin()
{
    pActualToApparentTransform = gsl_matrix_alloc(3, 3);
    pApparentToActualTransform = gsl_matrix_alloc(3, 3);
}

// Destructor

BasicMathPlugin::~BasicMathPlugin()
{
    gsl_matrix_free(pActualToApparentTransform);
    gsl_matrix_free(pApparentToActualTransform);
}

// Public methods

bool BasicMathPlugin::Initialise(InMemoryDatabase *pInMemoryDatabase)
{
    MathPlugin::Initialise(pInMemoryDatabase);
    InMemoryDatabase::AlignmentDatabaseType &SyncPoints = pInMemoryDatabase->GetAlignmentDatabase();

    /// See how many entries there are in the in memory database.
    /// - If just one use a hint to mounts approximate alignment, this can either be ZENITH,
    /// NORTH_CELESTIAL_POLE or SOUTH_CELESTIAL_POLE. The hint is used to make a dummy second
    /// entry. A dummy third entry is computed from the cross product of the first two. A transform
    /// matrix is then computed.
    /// - If two make the dummy third entry and compute a transform matrix.
    /// - If three compute a transform matrix.
    /// - If four or more compute a convex hull, then matrices for each
    /// triangular facet of the hull.
    switch (SyncPoints.size())
    {
        case 0:
            // Not sure whether to return false or true here
            return true;

        case 1:
        {
            AlignmentDatabaseEntry &Entry1 = SyncPoints[0];
            ln_equ_posn RaDec;
            ln_hrz_posn ActualSyncPoint1;
            ln_lnlat_posn Position { 0, 0 };
            if (!pInMemoryDatabase->GetDatabaseReferencePosition(Position))
                return false;
            RaDec.dec = Entry1.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec.ra = Entry1.RightAscension * 360.0 / 24.0;
            ln_get_hrz_from_equ(&RaDec, &Position, Entry1.ObservationJulianDate, &ActualSyncPoint1);
            // Now express this coordinate as a normalised direction vector (a.k.a direction cosines)
            TelescopeDirectionVector ActualDirectionCosine1 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint1);
            TelescopeDirectionVector DummyActualDirectionCosine2;
            TelescopeDirectionVector DummyApparentDirectionCosine2;
            TelescopeDirectionVector DummyActualDirectionCosine3;
            TelescopeDirectionVector DummyApparentDirectionCosine3;

            switch (ApproximateMountAlignment)
            {
                case ZENITH:
                    DummyActualDirectionCosine2.x = 0.0;
                    DummyActualDirectionCosine2.y = 0.0;
                    DummyActualDirectionCosine2.z = 1.0;
                    DummyApparentDirectionCosine2 = DummyActualDirectionCosine2;
                    break;

                case NORTH_CELESTIAL_POLE:
                {
                    ln_equ_posn DummyRaDec;
                    ln_hrz_posn DummyAltAz;
                    DummyRaDec.ra  = 0.0;
                    DummyRaDec.dec = 90.0;
                    ln_get_hrz_from_equ(&DummyRaDec, &Position, ln_get_julian_from_sys(), &DummyAltAz);
                    DummyActualDirectionCosine2   = TelescopeDirectionVectorFromAltitudeAzimuth(DummyAltAz);
                    DummyApparentDirectionCosine2 = DummyActualDirectionCosine2;
                    break;
                }
                case SOUTH_CELESTIAL_POLE:
                {
                    ln_equ_posn DummyRaDec;
                    ln_hrz_posn DummyAltAz;
                    DummyRaDec.ra  = 0.0;
                    DummyRaDec.dec = -90.0;
                    ln_get_hrz_from_equ(&DummyRaDec, &Position, ln_get_julian_from_sys(), &DummyAltAz);
                    DummyActualDirectionCosine2   = TelescopeDirectionVectorFromAltitudeAzimuth(DummyAltAz);
                    DummyApparentDirectionCosine2 = DummyActualDirectionCosine2;
                    break;
                }
            }
            DummyActualDirectionCosine3 = ActualDirectionCosine1 * DummyActualDirectionCosine2;
            DummyActualDirectionCosine3.Normalise();
            DummyApparentDirectionCosine3 = Entry1.TelescopeDirection * DummyApparentDirectionCosine2;
            DummyApparentDirectionCosine3.Normalise();
            CalculateTransformMatrices(ActualDirectionCosine1, DummyActualDirectionCosine2, DummyActualDirectionCosine3,
                                       Entry1.TelescopeDirection, DummyApparentDirectionCosine2,
                                       DummyApparentDirectionCosine3, pActualToApparentTransform,
                                       pApparentToActualTransform);
            return true;
        }
        case 2:
        {
            // First compute local horizontal coordinates for the two sync points
            AlignmentDatabaseEntry &Entry1 = SyncPoints[0];
            AlignmentDatabaseEntry &Entry2 = SyncPoints[1];
            ln_hrz_posn ActualSyncPoint1;
            ln_hrz_posn ActualSyncPoint2;
            ln_equ_posn RaDec1;
            ln_equ_posn RaDec2;
            RaDec1.dec = Entry1.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec1.ra  = Entry1.RightAscension * 360.0 / 24.0;
            RaDec2.dec = Entry2.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec2.ra = Entry2.RightAscension * 360.0 / 24.0;
            ln_lnlat_posn Position { 0, 0 };
            if (!pInMemoryDatabase->GetDatabaseReferencePosition(Position))
                return false;
            ln_get_hrz_from_equ(&RaDec1, &Position, Entry1.ObservationJulianDate, &ActualSyncPoint1);
            ln_get_hrz_from_equ(&RaDec2, &Position, Entry2.ObservationJulianDate, &ActualSyncPoint2);

            // Now express these coordinates as normalised direction vectors (a.k.a direction cosines)
            TelescopeDirectionVector ActualDirectionCosine1 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint1);
            TelescopeDirectionVector ActualDirectionCosine2 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint2);
            TelescopeDirectionVector DummyActualDirectionCosine3;
            TelescopeDirectionVector DummyApparentDirectionCosine3;
            DummyActualDirectionCosine3 = ActualDirectionCosine1 * ActualDirectionCosine2;
            DummyActualDirectionCosine3.Normalise();
            DummyApparentDirectionCosine3 = Entry1.TelescopeDirection * Entry2.TelescopeDirection;
            DummyApparentDirectionCosine3.Normalise();

            // The third direction vectors is generated by taking the cross product of the first two
            CalculateTransformMatrices(ActualDirectionCosine1, ActualDirectionCosine2, DummyActualDirectionCosine3,
                                       Entry1.TelescopeDirection, Entry2.TelescopeDirection,
                                       DummyApparentDirectionCosine3, pActualToApparentTransform,
                                       pApparentToActualTransform);
            return true;
        }

        case 3:
        {
            // First compute local horizontal coordinates for the three sync points
            AlignmentDatabaseEntry &Entry1 = SyncPoints[0];
            AlignmentDatabaseEntry &Entry2 = SyncPoints[1];
            AlignmentDatabaseEntry &Entry3 = SyncPoints[2];
            ln_hrz_posn ActualSyncPoint1;
            ln_hrz_posn ActualSyncPoint2;
            ln_hrz_posn ActualSyncPoint3;
            ln_equ_posn RaDec1;
            ln_equ_posn RaDec2;
            ln_equ_posn RaDec3;
            RaDec1.dec = Entry1.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec1.ra  = Entry1.RightAscension * 360.0 / 24.0;
            RaDec2.dec = Entry2.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec2.ra  = Entry2.RightAscension * 360.0 / 24.0;
            RaDec3.dec = Entry3.Declination;
            // libnova works in decimal degrees so conversion is needed here
            RaDec3.ra = Entry3.RightAscension * 360.0 / 24.0;
            ln_lnlat_posn Position { 0, 0 };
            if (!pInMemoryDatabase->GetDatabaseReferencePosition(Position))
                return false;
            ln_get_hrz_from_equ(&RaDec1, &Position, Entry1.ObservationJulianDate, &ActualSyncPoint1);
            ln_get_hrz_from_equ(&RaDec2, &Position, Entry2.ObservationJulianDate, &ActualSyncPoint2);
            ln_get_hrz_from_equ(&RaDec3, &Position, Entry3.ObservationJulianDate, &ActualSyncPoint3);

            // Now express these coordinates as normalised direction vectors (a.k.a direction cosines)
            TelescopeDirectionVector ActualDirectionCosine1 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint1);
            TelescopeDirectionVector ActualDirectionCosine2 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint2);
            TelescopeDirectionVector ActualDirectionCosine3 =
                TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint3);

            CalculateTransformMatrices(ActualDirectionCosine1, ActualDirectionCosine2, ActualDirectionCosine3,
                                       Entry1.TelescopeDirection, Entry2.TelescopeDirection, Entry3.TelescopeDirection,
                                       pActualToApparentTransform, pApparentToActualTransform);
            return true;
        }

        default:
        {
            ln_lnlat_posn Position { 0, 0 };
            if (!pInMemoryDatabase->GetDatabaseReferencePosition(Position))
                return false;

            // Compute Hulls etc.
            ActualConvexHull.Reset();
            ApparentConvexHull.Reset();
            ActualDirectionCosines.clear();

            // Add a dummy point at the nadir
            ActualConvexHull.MakeNewVertex(0.0, 0.0, -1.0, 0);
            ApparentConvexHull.MakeNewVertex(0.0, 0.0, -1.0, 0);

            int VertexNumber = 1;
            // Add the rest of the vertices
            for (InMemoryDatabase::AlignmentDatabaseType::const_iterator Itr = SyncPoints.begin();
                 Itr != SyncPoints.end(); Itr++)
            {
                ln_equ_posn RaDec;
                ln_hrz_posn ActualSyncPoint;
                RaDec.dec = (*Itr).Declination;
                // libnova works in decimal degrees so conversion is needed here
                RaDec.ra = (*Itr).RightAscension * 360.0 / 24.0;
                ln_get_hrz_from_equ(&RaDec, &Position, (*Itr).ObservationJulianDate, &ActualSyncPoint);
                // Now express this coordinate as normalised direction vectors (a.k.a direction cosines)
                TelescopeDirectionVector ActualDirectionCosine =
                    TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint);
                ActualDirectionCosines.push_back(ActualDirectionCosine);
                ActualConvexHull.MakeNewVertex(ActualDirectionCosine.x, ActualDirectionCosine.y,
                                               ActualDirectionCosine.z, VertexNumber);
                ApparentConvexHull.MakeNewVertex((*Itr).TelescopeDirection.x, (*Itr).TelescopeDirection.y,
                                                 (*Itr).TelescopeDirection.z, VertexNumber);
                VertexNumber++;
            }
            // I should only need to do this once but it is easier to do it twice
            ActualConvexHull.DoubleTriangle();
            ActualConvexHull.ConstructHull();
            ActualConvexHull.EdgeOrderOnFaces();
            ApparentConvexHull.DoubleTriangle();
            ApparentConvexHull.ConstructHull();
            ApparentConvexHull.EdgeOrderOnFaces();

            // Make the matrices
            ConvexHull::tFace CurrentFace = ActualConvexHull.faces;
#ifdef CONVEX_HULL_DEBUGGING
            int ActualFaces = 0;
#endif
            if (nullptr != CurrentFace)
            {
                do
                {
#ifdef CONVEX_HULL_DEBUGGING
                    ActualFaces++;
#endif
                    if ((0 == CurrentFace->vertex[0]->vnum) || (0 == CurrentFace->vertex[1]->vnum) ||
                        (0 == CurrentFace->vertex[2]->vnum))
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Initialise - Ignoring actual face %d", ActualFaces);
#endif
                    }
                    else
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Initialise - Processing actual face %d v1 %d v2 %d v3 %d", ActualFaces,
                                  CurrentFace->vertex[0]->vnum, CurrentFace->vertex[1]->vnum,
                                  CurrentFace->vertex[2]->vnum);
#endif
                        CalculateTransformMatrices(ActualDirectionCosines[CurrentFace->vertex[0]->vnum - 1],
                                                   ActualDirectionCosines[CurrentFace->vertex[1]->vnum - 1],
                                                   ActualDirectionCosines[CurrentFace->vertex[2]->vnum - 1],
                                                   SyncPoints[CurrentFace->vertex[0]->vnum - 1].TelescopeDirection,
                                                   SyncPoints[CurrentFace->vertex[1]->vnum - 1].TelescopeDirection,
                                                   SyncPoints[CurrentFace->vertex[2]->vnum - 1].TelescopeDirection,
                                                   CurrentFace->pMatrix, nullptr);
                    }
                    CurrentFace = CurrentFace->next;
                } while (CurrentFace != ActualConvexHull.faces);
            }

            // One of these days I will optimise this
            CurrentFace = ApparentConvexHull.faces;
#ifdef CONVEX_HULL_DEBUGGING
            int ApparentFaces = 0;
#endif
            if (nullptr != CurrentFace)
            {
                do
                {
#ifdef CONVEX_HULL_DEBUGGING
                    ApparentFaces++;
#endif
                    if ((0 == CurrentFace->vertex[0]->vnum) || (0 == CurrentFace->vertex[1]->vnum) ||
                        (0 == CurrentFace->vertex[2]->vnum))
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Initialise - Ignoring apparent face %d", ApparentFaces);
#endif
                    }
                    else
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Initialise - Processing apparent face %d v1 %d v2 %d v3 %d", ApparentFaces,
                                  CurrentFace->vertex[0]->vnum, CurrentFace->vertex[1]->vnum,
                                  CurrentFace->vertex[2]->vnum);
#endif
                        CalculateTransformMatrices(SyncPoints[CurrentFace->vertex[0]->vnum - 1].TelescopeDirection,
                                                   SyncPoints[CurrentFace->vertex[1]->vnum - 1].TelescopeDirection,
                                                   SyncPoints[CurrentFace->vertex[2]->vnum - 1].TelescopeDirection,
                                                   ActualDirectionCosines[CurrentFace->vertex[0]->vnum - 1],
                                                   ActualDirectionCosines[CurrentFace->vertex[1]->vnum - 1],
                                                   ActualDirectionCosines[CurrentFace->vertex[2]->vnum - 1],
                                                   CurrentFace->pMatrix, nullptr);
                    }
                    CurrentFace = CurrentFace->next;
                } while (CurrentFace != ApparentConvexHull.faces);
            }

#ifdef CONVEX_HULL_DEBUGGING
            ASSDEBUGF("Initialise - ActualFaces %d ApparentFaces %d", ActualFaces, ApparentFaces);
            ActualConvexHull.PrintObj("ActualHull.obj");
            ActualConvexHull.PrintOut("ActualHull.log", ActualConvexHull.vertices);
            ApparentConvexHull.PrintObj("ApparentHull.obj");
            ActualConvexHull.PrintOut("ApparentHull.log", ApparentConvexHull.vertices);
#endif
            return true;
        }
    }
}

bool BasicMathPlugin::TransformCelestialToTelescope(const double RightAscension, const double Declination,
                                                    double JulianOffset,
                                                    TelescopeDirectionVector &ApparentTelescopeDirectionVector)
{
    ln_equ_posn ActualRaDec;
    ln_hrz_posn ActualAltAz;
    // libnova works in decimal degrees so conversion is needed here
    ActualRaDec.ra  = RightAscension * 360.0 / 24.0;
    ActualRaDec.dec = Declination;
    ln_lnlat_posn Position { 0, 0 };

    if ((nullptr == pInMemoryDatabase) ||
        !pInMemoryDatabase->GetDatabaseReferencePosition(
            Position)) // Should check that this the same as the current observing position
        return false;

    ln_get_hrz_from_equ(&ActualRaDec, &Position, ln_get_julian_from_sys() + JulianOffset, &ActualAltAz);
    ASSDEBUGF("Celestial to telescope - Actual Alt %lf Az %lf", ActualAltAz.alt, ActualAltAz.az);

    TelescopeDirectionVector ActualVector = TelescopeDirectionVectorFromAltitudeAzimuth(ActualAltAz);

    InMemoryDatabase::AlignmentDatabaseType &SyncPoints = pInMemoryDatabase->GetAlignmentDatabase();
    switch (SyncPoints.size())
    {
        case 0:
        {
            // 0 sync points
            ApparentTelescopeDirectionVector = ActualVector;

            switch (ApproximateMountAlignment)
            {
                case ZENITH:
                    break;

                case NORTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated anticlockwise
                    ApparentTelescopeDirectionVector.RotateAroundY(Position.lat - 90.0);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated clockwise
                    ApparentTelescopeDirectionVector.RotateAroundY(Position.lat + 90.0);
                    break;
            }
            break;
        }
        case 1:
        case 2:
        case 3:
        {
            gsl_vector *pGSLActualVector = gsl_vector_alloc(3);
            gsl_vector_set(pGSLActualVector, 0, ActualVector.x);
            gsl_vector_set(pGSLActualVector, 1, ActualVector.y);
            gsl_vector_set(pGSLActualVector, 2, ActualVector.z);
            gsl_vector *pGSLApparentVector = gsl_vector_alloc(3);
            MatrixVectorMultiply(pActualToApparentTransform, pGSLActualVector, pGSLApparentVector);
            ApparentTelescopeDirectionVector.x = gsl_vector_get(pGSLApparentVector, 0);
            ApparentTelescopeDirectionVector.y = gsl_vector_get(pGSLApparentVector, 1);
            ApparentTelescopeDirectionVector.z = gsl_vector_get(pGSLApparentVector, 2);
            ApparentTelescopeDirectionVector.Normalise();
            gsl_vector_free(pGSLActualVector);
            gsl_vector_free(pGSLApparentVector);
            break;
        }

        default:
        {
            gsl_matrix *pTransform;
            gsl_matrix *pComputedTransform = nullptr;
            // Scale the actual telescope direction vector to make sure it traverses the unit sphere.
            TelescopeDirectionVector ScaledActualVector = ActualVector * 2.0;
            // Shoot the scaled vector in the into the list of actual facets
            // and use the conversuion matrix from the one it intersects
            ConvexHull::tFace CurrentFace = ActualConvexHull.faces;
#ifdef CONVEX_HULL_DEBUGGING
            int ActualFaces = 0;
#endif
            if (nullptr != CurrentFace)
            {
                do
                {
#ifdef CONVEX_HULL_DEBUGGING
                    ActualFaces++;
#endif
                    // Ignore faces containg vertex 0 (nadir).
                    if ((0 == CurrentFace->vertex[0]->vnum) || (0 == CurrentFace->vertex[1]->vnum) ||
                        (0 == CurrentFace->vertex[2]->vnum))
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Celestial to telescope - Ignoring actual face %d", ActualFaces);
#endif
                    }
                    else
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Celestial to telescope - Processing actual face %d v1 %d v2 %d v3 %d", ActualFaces,
                                  CurrentFace->vertex[0]->vnum, CurrentFace->vertex[1]->vnum,
                                  CurrentFace->vertex[2]->vnum);
#endif
                        if (RayTriangleIntersection(ScaledActualVector,
                                                    ActualDirectionCosines[CurrentFace->vertex[0]->vnum - 1],
                                                    ActualDirectionCosines[CurrentFace->vertex[1]->vnum - 1],
                                                    ActualDirectionCosines[CurrentFace->vertex[2]->vnum - 1]))
                            break;
                    }
                    CurrentFace = CurrentFace->next;
                } while (CurrentFace != ActualConvexHull.faces);
                if (CurrentFace == ActualConvexHull.faces)
                {
                    // Find the three nearest points and build a transform
                    std::map<double, const AlignmentDatabaseEntry *> NearestMap;
                    for (InMemoryDatabase::AlignmentDatabaseType::const_iterator Itr = SyncPoints.begin();
                         Itr != SyncPoints.end(); Itr++)
                    {
                        ln_equ_posn RaDec;
                        ln_hrz_posn ActualPoint;
                        RaDec.ra  = (*Itr).RightAscension * 360.0 / 24.0;
                        RaDec.dec = (*Itr).Declination;
                        ln_get_hrz_from_equ(&RaDec, &Position, (*Itr).ObservationJulianDate, &ActualPoint);
                        TelescopeDirectionVector ActualDirectionCosine =
                            TelescopeDirectionVectorFromAltitudeAzimuth(ActualPoint);
                        NearestMap[(ActualDirectionCosine - ActualVector).Length()] = &(*Itr);
                    }
                    // First compute local horizontal coordinates for the three sync points
                    std::map<double, const AlignmentDatabaseEntry *>::const_iterator Nearest = NearestMap.begin();
                    const AlignmentDatabaseEntry *pEntry1                                    = (*Nearest).second;
                    Nearest++;
                    const AlignmentDatabaseEntry *pEntry2 = (*Nearest).second;
                    Nearest++;
                    const AlignmentDatabaseEntry *pEntry3 = (*Nearest).second;
                    ln_hrz_posn ActualSyncPoint1;
                    ln_hrz_posn ActualSyncPoint2;
                    ln_hrz_posn ActualSyncPoint3;
                    ln_equ_posn RaDec1;
                    ln_equ_posn RaDec2;
                    ln_equ_posn RaDec3;
                    RaDec1.dec = pEntry1->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec1.ra  = pEntry1->RightAscension * 360.0 / 24.0;
                    RaDec2.dec = pEntry2->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec2.ra  = pEntry2->RightAscension * 360.0 / 24.0;
                    RaDec3.dec = pEntry3->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec3.ra = pEntry3->RightAscension * 360.0 / 24.0;
                    ln_get_hrz_from_equ(&RaDec1, &Position, pEntry1->ObservationJulianDate, &ActualSyncPoint1);
                    ln_get_hrz_from_equ(&RaDec2, &Position, pEntry2->ObservationJulianDate, &ActualSyncPoint2);
                    ln_get_hrz_from_equ(&RaDec3, &Position, pEntry3->ObservationJulianDate, &ActualSyncPoint3);

                    // Now express these coordinates as normalised direction vectors (a.k.a direction cosines)
                    TelescopeDirectionVector ActualDirectionCosine1 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint1);
                    TelescopeDirectionVector ActualDirectionCosine2 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint2);
                    TelescopeDirectionVector ActualDirectionCosine3 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint3);
                    pComputedTransform = gsl_matrix_alloc(3, 3);
                    CalculateTransformMatrices(ActualDirectionCosine1, ActualDirectionCosine2, ActualDirectionCosine3,
                                               pEntry1->TelescopeDirection, pEntry2->TelescopeDirection,
                                               pEntry3->TelescopeDirection, pComputedTransform, nullptr);
                    pTransform = pComputedTransform;
                }
                else
                    pTransform = CurrentFace->pMatrix;
            }
            else
                return false;

            // OK - got an intersection - CurrentFace is pointing at the face
            gsl_vector *pGSLActualVector = gsl_vector_alloc(3);
            gsl_vector_set(pGSLActualVector, 0, ActualVector.x);
            gsl_vector_set(pGSLActualVector, 1, ActualVector.y);
            gsl_vector_set(pGSLActualVector, 2, ActualVector.z);
            gsl_vector *pGSLApparentVector = gsl_vector_alloc(3);
            MatrixVectorMultiply(pTransform, pGSLActualVector, pGSLApparentVector);
            ApparentTelescopeDirectionVector.x = gsl_vector_get(pGSLApparentVector, 0);
            ApparentTelescopeDirectionVector.y = gsl_vector_get(pGSLApparentVector, 1);
            ApparentTelescopeDirectionVector.z = gsl_vector_get(pGSLApparentVector, 2);
            ApparentTelescopeDirectionVector.Normalise();
            gsl_vector_free(pGSLActualVector);
            gsl_vector_free(pGSLApparentVector);
            if (nullptr != pComputedTransform)
                gsl_matrix_free(pComputedTransform);
            break;
        }
    }

    ln_hrz_posn ApparentAltAz;
    AltitudeAzimuthFromTelescopeDirectionVector(ApparentTelescopeDirectionVector, ApparentAltAz);
    ASSDEBUGF("Celestial to telescope - Apparent Alt %lf Az %lf", ApparentAltAz.alt, ApparentAltAz.az);

    return true;
}

bool BasicMathPlugin::TransformTelescopeToCelestial(const TelescopeDirectionVector &ApparentTelescopeDirectionVector,
                                                    double &RightAscension, double &Declination)
{
    ln_lnlat_posn Position { 0, 0 };

    ln_hrz_posn ApparentAltAz;
    ln_hrz_posn ActualAltAz;
    ln_equ_posn ActualRaDec;

    AltitudeAzimuthFromTelescopeDirectionVector(ApparentTelescopeDirectionVector, ApparentAltAz);
    ASSDEBUGF("Telescope to celestial - Apparent Alt %lf Az %lf", ApparentAltAz.alt, ApparentAltAz.az);

    if ((nullptr == pInMemoryDatabase) || !pInMemoryDatabase->GetDatabaseReferencePosition(Position))
    {
        // Should check that this the same as the current observing position
        ASSDEBUG("No database or no position in database");
        return false;
    }
    InMemoryDatabase::AlignmentDatabaseType &SyncPoints = pInMemoryDatabase->GetAlignmentDatabase();
    switch (SyncPoints.size())
    {
        case 0:
        {
            // 0 sync points
            TelescopeDirectionVector RotatedTDV(ApparentTelescopeDirectionVector);
            switch (ApproximateMountAlignment)
            {
                case ZENITH:
                    break;

                case NORTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated clockwise
                    RotatedTDV.RotateAroundY(90.0 - Position.lat);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated anticlockwise
                    RotatedTDV.RotateAroundY(-90.0 - Position.lat);
                    break;
            }
            ASSDEBUGF("ApparentVector x %lf y %lf z %lf", ApparentTelescopeDirectionVector.x,
                      ApparentTelescopeDirectionVector.y, ApparentTelescopeDirectionVector.z);
            ASSDEBUGF("ActualVector x %lf y %lf z %lf", RotatedTDV.x, RotatedTDV.y, RotatedTDV.z);
            AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, ActualAltAz);
            ln_get_equ_from_hrz(&ActualAltAz, &Position, ln_get_julian_from_sys(), &ActualRaDec);
            // libnova works in decimal degrees so conversion is needed here
            RightAscension = ActualRaDec.ra * 24.0 / 360.0;
            Declination    = ActualRaDec.dec;
            break;
        }
        case 1:
        case 2:
        case 3:
        {
            gsl_vector *pGSLApparentVector = gsl_vector_alloc(3);
            gsl_vector_set(pGSLApparentVector, 0, ApparentTelescopeDirectionVector.x);
            gsl_vector_set(pGSLApparentVector, 1, ApparentTelescopeDirectionVector.y);
            gsl_vector_set(pGSLApparentVector, 2, ApparentTelescopeDirectionVector.z);
            gsl_vector *pGSLActualVector = gsl_vector_alloc(3);
            MatrixVectorMultiply(pApparentToActualTransform, pGSLApparentVector, pGSLActualVector);

            Dump3("ApparentVector", pGSLApparentVector);
            Dump3("ActualVector", pGSLActualVector);

            TelescopeDirectionVector ActualTelescopeDirectionVector;
            ActualTelescopeDirectionVector.x = gsl_vector_get(pGSLActualVector, 0);
            ActualTelescopeDirectionVector.y = gsl_vector_get(pGSLActualVector, 1);
            ActualTelescopeDirectionVector.z = gsl_vector_get(pGSLActualVector, 2);
            ActualTelescopeDirectionVector.Normalise();
            AltitudeAzimuthFromTelescopeDirectionVector(ActualTelescopeDirectionVector, ActualAltAz);
            ln_get_equ_from_hrz(&ActualAltAz, &Position, ln_get_julian_from_sys(), &ActualRaDec);
            // libnova works in decimal degrees so conversion is needed here
            RightAscension = ActualRaDec.ra * 24.0 / 360.0;
            Declination    = ActualRaDec.dec;
            gsl_vector_free(pGSLActualVector);
            gsl_vector_free(pGSLApparentVector);
            break;
        }

        default:
        {
            gsl_matrix *pTransform;
            gsl_matrix *pComputedTransform = nullptr;
            // Scale the apparent telescope direction vector to make sure it traverses the unit sphere.
            TelescopeDirectionVector ScaledApparentVector = ApparentTelescopeDirectionVector * 2.0;
            // Shoot the scaled vector in the into the list of apparent facets
            // and use the conversuion matrix from the one it intersects
            ConvexHull::tFace CurrentFace = ApparentConvexHull.faces;
#ifdef CONVEX_HULL_DEBUGGING
            int ApparentFaces = 0;
#endif
            if (nullptr != CurrentFace)
            {
                do
                {
#ifdef CONVEX_HULL_DEBUGGING
                    ApparentFaces++;
#endif
                    // Ignore faces containg vertex 0 (nadir).
                    if ((0 == CurrentFace->vertex[0]->vnum) || (0 == CurrentFace->vertex[1]->vnum) ||
                        (0 == CurrentFace->vertex[2]->vnum))
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("Celestial to telescope - Ignoring apparent face %d", ApparentFaces);
#endif
                    }
                    else
                    {
#ifdef CONVEX_HULL_DEBUGGING
                        ASSDEBUGF("TelescopeToCelestial - Processing apparent face %d v1 %d v2 %d v3 %d", ApparentFaces,
                                  CurrentFace->vertex[0]->vnum, CurrentFace->vertex[1]->vnum,
                                  CurrentFace->vertex[2]->vnum);
#endif
                        if (RayTriangleIntersection(ScaledApparentVector,
                                                    SyncPoints[CurrentFace->vertex[0]->vnum - 1].TelescopeDirection,
                                                    SyncPoints[CurrentFace->vertex[1]->vnum - 1].TelescopeDirection,
                                                    SyncPoints[CurrentFace->vertex[2]->vnum - 1].TelescopeDirection))
                            break;
                    }
                    CurrentFace = CurrentFace->next;
                } while (CurrentFace != ApparentConvexHull.faces);
                if (CurrentFace == ApparentConvexHull.faces)
                {
                    // Find the three nearest points and build a transform
                    std::map<double, const AlignmentDatabaseEntry *> NearestMap;
                    for (InMemoryDatabase::AlignmentDatabaseType::const_iterator Itr = SyncPoints.begin();
                         Itr != SyncPoints.end(); Itr++)
                    {
                        NearestMap[((*Itr).TelescopeDirection - ApparentTelescopeDirectionVector).Length()] = &(*Itr);
                    }
                    // First compute local horizontal coordinates for the three sync points
                    std::map<double, const AlignmentDatabaseEntry *>::const_iterator Nearest = NearestMap.begin();
                    const AlignmentDatabaseEntry *pEntry1                                    = (*Nearest).second;
                    Nearest++;
                    const AlignmentDatabaseEntry *pEntry2 = (*Nearest).second;
                    Nearest++;
                    const AlignmentDatabaseEntry *pEntry3 = (*Nearest).second;
                    ln_hrz_posn ActualSyncPoint1;
                    ln_hrz_posn ActualSyncPoint2;
                    ln_hrz_posn ActualSyncPoint3;
                    ln_equ_posn RaDec1;
                    ln_equ_posn RaDec2;
                    ln_equ_posn RaDec3;
                    RaDec1.dec = pEntry1->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec1.ra  = pEntry1->RightAscension * 360.0 / 24.0;
                    RaDec2.dec = pEntry2->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec2.ra  = pEntry2->RightAscension * 360.0 / 24.0;
                    RaDec3.dec = pEntry3->Declination;
                    // libnova works in decimal degrees so conversion is needed here
                    RaDec3.ra = pEntry3->RightAscension * 360.0 / 24.0;
                    ln_get_hrz_from_equ(&RaDec1, &Position, pEntry1->ObservationJulianDate, &ActualSyncPoint1);
                    ln_get_hrz_from_equ(&RaDec2, &Position, pEntry2->ObservationJulianDate, &ActualSyncPoint2);
                    ln_get_hrz_from_equ(&RaDec3, &Position, pEntry3->ObservationJulianDate, &ActualSyncPoint3);

                    // Now express these coordinates as normalised direction vectors (a.k.a direction cosines)
                    TelescopeDirectionVector ActualDirectionCosine1 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint1);
                    TelescopeDirectionVector ActualDirectionCosine2 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint2);
                    TelescopeDirectionVector ActualDirectionCosine3 =
                        TelescopeDirectionVectorFromAltitudeAzimuth(ActualSyncPoint3);
                    pComputedTransform = gsl_matrix_alloc(3, 3);
                    CalculateTransformMatrices(pEntry1->TelescopeDirection, pEntry2->TelescopeDirection,
                                               pEntry3->TelescopeDirection, ActualDirectionCosine1,
                                               ActualDirectionCosine2, ActualDirectionCosine3, pComputedTransform,
                                               nullptr);
                    pTransform = pComputedTransform;
                }
                else
                    pTransform = CurrentFace->pMatrix;
            }
            else
                return false;

            // OK - got an intersection - CurrentFace is pointing at the face
            gsl_vector *pGSLApparentVector = gsl_vector_alloc(3);
            gsl_vector_set(pGSLApparentVector, 0, ApparentTelescopeDirectionVector.x);
            gsl_vector_set(pGSLApparentVector, 1, ApparentTelescopeDirectionVector.y);
            gsl_vector_set(pGSLApparentVector, 2, ApparentTelescopeDirectionVector.z);
            gsl_vector *pGSLActualVector = gsl_vector_alloc(3);
            MatrixVectorMultiply(pTransform, pGSLApparentVector, pGSLActualVector);
            TelescopeDirectionVector ActualTelescopeDirectionVector;
            ActualTelescopeDirectionVector.x = gsl_vector_get(pGSLActualVector, 0);
            ActualTelescopeDirectionVector.y = gsl_vector_get(pGSLActualVector, 1);
            ActualTelescopeDirectionVector.z = gsl_vector_get(pGSLActualVector, 2);
            ActualTelescopeDirectionVector.Normalise();
            AltitudeAzimuthFromTelescopeDirectionVector(ActualTelescopeDirectionVector, ActualAltAz);
            ln_get_equ_from_hrz(&ActualAltAz, &Position, ln_get_julian_from_sys(), &ActualRaDec);
            // libnova works in decimal degrees so conversion is needed here
            RightAscension = ActualRaDec.ra * 24.0 / 360.0;
            Declination    = ActualRaDec.dec;
            gsl_vector_free(pGSLActualVector);
            gsl_vector_free(pGSLApparentVector);
            if (nullptr != pComputedTransform)
                gsl_matrix_free(pComputedTransform);
            break;
        }
    }
    ASSDEBUGF("Telescope to Celestial - Actual Alt %lf Az %lf", ActualAltAz.alt, ActualAltAz.az);
    return true;
}

// Private methods

void BasicMathPlugin::Dump3(const char *Label, gsl_vector *pVector)
{
    ASSDEBUGF("Vector dump - %s", Label);
    ASSDEBUGF("%lf %lf %lf", gsl_vector_get(pVector, 0), gsl_vector_get(pVector, 1), gsl_vector_get(pVector, 2));
}

void BasicMathPlugin::Dump3x3(const char *Label, gsl_matrix *pMatrix)
{
    ASSDEBUGF("Matrix dump - %s", Label);
    ASSDEBUGF("Row 0 %lf %lf %lf", gsl_matrix_get(pMatrix, 0, 0), gsl_matrix_get(pMatrix, 0, 1),
              gsl_matrix_get(pMatrix, 0, 2));
    ASSDEBUGF("Row 1 %lf %lf %lf", gsl_matrix_get(pMatrix, 1, 0), gsl_matrix_get(pMatrix, 1, 1),
              gsl_matrix_get(pMatrix, 1, 2));
    ASSDEBUGF("Row 2 %lf %lf %lf", gsl_matrix_get(pMatrix, 2, 0), gsl_matrix_get(pMatrix, 2, 1),
              gsl_matrix_get(pMatrix, 2, 2));
}

/// Use gsl to compute the determinant of a 3x3 matrix
double BasicMathPlugin::Matrix3x3Determinant(gsl_matrix *pMatrix)
{
    gsl_permutation *pPermutation = gsl_permutation_alloc(3);
    gsl_matrix *pDecomp           = gsl_matrix_alloc(3, 3);
    int Signum;
    double Determinant;

    gsl_matrix_memcpy(pDecomp, pMatrix);

    gsl_linalg_LU_decomp(pDecomp, pPermutation, &Signum);

    Determinant = gsl_linalg_LU_det(pDecomp, Signum);

    gsl_matrix_free(pDecomp);
    gsl_permutation_free(pPermutation);

    return Determinant;
}

/// Use gsl to compute the inverse of a 3x3 matrix
bool BasicMathPlugin::MatrixInvert3x3(gsl_matrix *pInput, gsl_matrix *pInversion)
{
    bool Retcode                  = true;
    gsl_permutation *pPermutation = gsl_permutation_alloc(3);
    gsl_matrix *pDecomp           = gsl_matrix_alloc(3, 3);
    int Signum;

    gsl_matrix_memcpy(pDecomp, pInput);

    gsl_linalg_LU_decomp(pDecomp, pPermutation, &Signum);

    // Test for singularity
    if (0 == gsl_linalg_LU_det(pDecomp, Signum))
    {
        Retcode = false;
    }
    else
        gsl_linalg_LU_invert(pDecomp, pPermutation, pInversion);

    gsl_matrix_free(pDecomp);
    gsl_permutation_free(pPermutation);

    return Retcode;
}

/// Use gsl blas support to multiply two matrices together and put the result in a third.
/// For our purposes all the matrices should be 3 by 3.
void BasicMathPlugin::MatrixMatrixMultiply(gsl_matrix *pA, gsl_matrix *pB, gsl_matrix *pC)
{
    // Zeroise the output matrix
    gsl_matrix_set_zero(pC);

    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, pA, pB, 0.0, pC);
}

/// Use gsl blas support to multiply a matrix by a vector and put the result in another vector
/// For our purposes the the matrix should be 3x3 and vector 3.
void BasicMathPlugin::MatrixVectorMultiply(gsl_matrix *pA, gsl_vector *pB, gsl_vector *pC)
{
    // Zeroise the output vector
    gsl_vector_set_zero(pC);

    gsl_blas_dgemv(CblasNoTrans, 1.0, pA, pB, 0.0, pC);
}

bool BasicMathPlugin::RayTriangleIntersection(TelescopeDirectionVector &Ray, TelescopeDirectionVector &TriangleVertex1,
                                              TelescopeDirectionVector &TriangleVertex2,
                                              TelescopeDirectionVector &TriangleVertex3)
{
    // Use Möller-Trumbore

    //Find vectors for two edges sharing V1
    TelescopeDirectionVector Edge1 = TriangleVertex2 - TriangleVertex1;
    TelescopeDirectionVector Edge2 = TriangleVertex3 - TriangleVertex1;

    TelescopeDirectionVector P = Ray * Edge2; // cross product
    double Determinant         = Edge1 ^ P;   // dot product
    double InverseDeterminant  = 1.0 / Determinant;

    // If the determinant is negative the triangle is backfacing
    // If the determinant is close to 0, the ray misses the triangle
    if ((Determinant > -std::numeric_limits<double>::epsilon()) &&
        (Determinant < std::numeric_limits<double>::epsilon()))
        return false;

    // I use zero as ray origin so
    TelescopeDirectionVector T(-TriangleVertex1.x, -TriangleVertex1.y, -TriangleVertex1.z);

    // Calculate the u parameter
    double u = (T ^ P) * InverseDeterminant;

    if (u < 0.0 || u > 1.0)
        //The intersection lies outside of the triangle
        return false;

    //Prepare to test v parameter
    TelescopeDirectionVector Q = T * Edge1;

    //Calculate v parameter and test bound
    double v = (Ray ^ Q) * InverseDeterminant;

    if (v < 0.0 || u + v > 1.0)
        //The intersection lies outside of the triangle
        return false;

    double t = (Edge2 ^ Q) * InverseDeterminant;

    if (t > std::numeric_limits<double>::epsilon())
    {
        //ray intersection
        return true;
    }

    // No hit, no win
    return false;
}

} // namespace AlignmentSubsystem
} // namespace INDI
