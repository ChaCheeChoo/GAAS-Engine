#ifndef COLLISION
#define COLLISION

#include <psptypes.h>
#include <stddef.h>

typedef struct SphereCollision {
	ScePspFVector3 pos; //Sphere center point
	float radius; //Sphere radius
}SphereCollision;

typedef struct BoundingBox {
    ScePspFVector3 min; //Contains lowest corner of the box
    ScePspFVector3 max; //Contains highest corner of the box
}BoundingBox;

/**
 *  Calculates the distance between two vectors 3d vectors
**/
float gaasCOLVectorDistance(struct ScePspFVector3 pos1, struct ScePspFVector3 pos2);

/**
 * Checks if two spheres are colliding
**/
int gaasCOLSphereToSphere(struct SphereCollision Sph1, struct SphereCollision Sph2);

/**
 * Checks if two axis-aligned bounding boxes are colliding 
**/
int gaasCOLAABBToAABB(struct BoundingBox Box1, struct BoundingBox Box2);

/**
 * Finds if 2d point is on the left or right side of line
 * p1x, p1y - position of point
 * p2x, p2y - start of line
 * p3x, p3y - end of line
 * 
 * returns: side of point and its distance away from line
**/
float gaasCOLSign(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y);

/**
 * Finds if 2d point is inside a polygon
 * ptx, pty - position of point
 * p1x, p1y - first point of polygon
 * p2x, p2y - second point of polygon
 * p3x, p3y - third point of polygon
 * 
 * returns: 1 - point is inside; 2 - point is outside
**/
int gaasCOLPointInPolygon(float ptx, float pty, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y);

/**
 * Finds if 2d point is inside a quad
 * ptx, pty - position of point
 * p1x, p1y - first point of quad
 * p2x, p2y - second point of quad
 * p3x, p3y - third point of quad
 * p3x, p3y - fourth point of quad
 * 
 * returns: 1 - point is inside; 2 - point is outside
**/
int gaasCOLPointInQuad(float ptx, float pty, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y, float v4x, float v4y);

/**
 * Converts Degrees to Radians
 * Radians are the main unit of measurment used for rotations in PSPGU
 * Deg - degrees
 * 
 * returns: radians
**/
float gaasCOLDeg2Rad(double Deg);

/**
 * Converts Radians to Degrees
 * Radians are the main unit of measurment used for rotations in PSPGU
 * Rad - radians
 * 
 * returns: degrees
**/
float gaasCOLRad2Deg(double Rad);

/**
 * Finds cross product 
 * 
 * returns: 3d vector
**/
ScePspFVector3 gaasCOLCross(float u1, float u2, float u3, float v1, float v2, float v3);

#endif