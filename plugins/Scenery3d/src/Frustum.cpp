#include "Frustum.hpp"
#include "Util.hpp"
#include <QtOpenGL>

Frustum::Frustum()
{
    for(unsigned int i=0; i<CORNERCOUNT; i++)
    {
        corners.push_back(Vec3f(0.0f, 0.0f, 0.0f));
        drawCorners.push_back(Vec3f(0.0f, 0.0f, 0.0f));
    }

    for(unsigned int i=0; i<PLANECOUNT; i++)
    {
        planes.push_back(Plane());
    }

    fov = 0.0f;
    aspect = 0.0f;
    zNear = 0.0f;
    zFar = 0.0f;
}

Frustum::~Frustum() {}

const Vec3f& Frustum::getCorner(Corner corner) const
{
    return corners[corner];
}

const Plane& Frustum::getPlane(FrustumPlane plane) const
{
    return planes[plane];
}

void Frustum::calcFrustum(Vec3d &p, Vec3d &l, Vec3d &u)
{
    Vec3d ntl, ntr, nbl, nbr, ftl, ftr, fbr, fbl;

    Vec3d X = l^u;
    X.normalize();

    Vec3d Z = l^X;
    Z.normalize();

    float tang = tanf((static_cast<float>(M_PI)/360.0f)*fov);
    float nh = zNear * tang;
    float nw = nh * aspect;
    float fh = zFar * tang;
    float fw = fh * aspect;

    Vec3d nc = p + l*zNear;
    Vec3d fc = p + l*zFar;

    ntl = nc + Z * nh - X * nw;
    ntr = nc + Z * nh + X * nw;
    nbl = nc - Z * nh - X * nw;
    nbr = nc - Z * nh + X * nw;

    ftl = fc + Z * fh - X * fw;
    ftr = fc + Z * fh + X * fw;
    fbl = fc - Z * fh - X * fw;
    fbr = fc - Z * fh + X * fw;

    corners[NTL] = vecdToFloat(ntl);
    corners[NTR] = vecdToFloat(ntr);
    corners[NBL] = vecdToFloat(nbl);
    corners[NBR] = vecdToFloat(nbr);
    corners[FTL] = vecdToFloat(ftl);
    corners[FTR] = vecdToFloat(ftr);
    corners[FBL] = vecdToFloat(fbl);
    corners[FBR] = vecdToFloat(fbr);

//    //Compute right vector
//    Vec3d r = u^l;

//    //Compute height and width of boundary on the near/far plane
//    float Hnear = tanf((static_cast<float>(M_PI)/360.0f)*fov)*zNear;
//    float Wnear = Hnear*aspect;

//    float Hfar = tanf((static_cast<float>(M_PI)/360.0f)*fov)*zFar;
//    float Wfar = Hfar*aspect;

//    Vec3d nearCenter = p + l*zNear;
//    Vec3d nbr = nearCenter - (u*Hnear) + (r*Wnear);
//    Vec3d ntr = nearCenter + (u*Hnear) + (r*Wnear);
//    Vec3d ntl = nearCenter + (u*Hnear) - (r*Wnear);
//    Vec3d nbl = nearCenter - (u*Hnear) - (r*Wnear);

//    Vec3d farCenter = p + l*zFar;
//    Vec3d fbr = farCenter - (u*Hfar) + (r*Wfar);
//    Vec3d ftr = farCenter + (u*Hfar) + (r*Wfar);
//    Vec3d ftl = farCenter + (u*Hfar) - (r*Wfar);
//    Vec3d fbl = farCenter - (u*Hfar) - (r*Wfar);

//    //Cast to float and save corners
//    corners[NBL] = vecdToFloat(nbl);
//    corners[NBR] = vecdToFloat(nbr);
//    corners[NTR] = vecdToFloat(ntr);
//    corners[NTL] = vecdToFloat(ntl);
//    corners[FBL] = vecdToFloat(fbl);
//    corners[FBR] = vecdToFloat(fbr);
//    corners[FTR] = vecdToFloat(ftr);
//    corners[FTL] = vecdToFloat(ftl);

    planes[TOP] = Plane(corners[NTR], corners[NTL], corners[FTL]);
    planes[BOTTOM] = Plane(corners[NBL], corners[NBR], corners[FBR]);
    planes[LEFT] = Plane(corners[NTL], corners[NBL], corners[FBL]);
    planes[RIGHT] = Plane(corners[NBR], corners[NTR], corners[FBR]);
    planes[NEARP] = Plane(corners[NTL], corners[NTR], corners[NBR]);
    planes[FARP] = Plane(corners[FTR], corners[FTL], corners[FBL]);
}

int Frustum::pointInFrustum(Vec3f &p)
{
    int result = INSIDE;
    for(int i=0; i<PLANECOUNT; i++)
    {
        if(planes[i].calcDistance(p) < 0)
        {
            return OUTSIDE;
        }
    }

    return result;
}

int Frustum::boxInFrustum(const AABB* bbox)
{
    int result = INSIDE;
    for(unsigned int i=0; i<PLANECOUNT; i++)
    {
        if(planes[i].calcDistance(bbox->positiveVertex(planes[i].normal)) < 0)
        {
            return OUTSIDE;
        }
        else if(planes[i].calcDistance(bbox->negativeVertex(planes[i].normal)) < 0)
        {
            return INTERSECT;
        }
    }

    return result;
}

void Frustum::saveCorners()
{
    for(unsigned int i=0; i<CORNERCOUNT; i++)
    {
        drawCorners[i] = corners[i];
    }
}

void Frustum::drawFrustum()
{
    Vec3d ntl = vecfToDouble(drawCorners[NTL]);
    Vec3d ntr = vecfToDouble(drawCorners[NTR]);
    Vec3d nbr = vecfToDouble(drawCorners[NBR]);
    Vec3d nbl = vecfToDouble(drawCorners[NBL]);
    Vec3d ftr = vecfToDouble(drawCorners[FTR]);
    Vec3d ftl = vecfToDouble(drawCorners[FTL]);
    Vec3d fbl = vecfToDouble(drawCorners[FBL]);
    Vec3d fbr = vecfToDouble(drawCorners[FBR]);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(5);

    glBegin(GL_LINE_LOOP);
        //near plane
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //far plane
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //bottom plane
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //top plane
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        //left plane
        glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
        glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
        glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
        glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
    glEnd();

    glBegin(GL_LINE_LOOP);
        // right plane
        glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
        glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
        glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
        glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
    glEnd();
}
