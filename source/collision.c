#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <stdio.h>

#include "collision.h" 

float gaasCOLVectorDistance(struct ScePspFVector3 pos1, struct ScePspFVector3 pos2) {
	/* float result;
    __asm__ volatile ( //I have no idea what I'm doing
		"mtv     %1, S000\n" //pos1
		"mtv     %2, S001\n" //pos2

		"vmul.t  S010, S000, S000\n"
		"vmul.t  S011, S001, S001\n"

		"vadd.t  S012, S010, S011\n"

		"vsqrt.s S000, S012\n"

		"mfv     %0, S000\n"
	: "=r"(result) : "m"(pos1), "m"(pos2));
    return result; */
	return sqrtf((pos2.x-pos1.x)*(pos2.x-pos1.x) + (pos2.y-pos1.y)*(pos2.y-pos1.y) + (pos2.z-pos1.z)*(pos2.z-pos1.z));
}

float gaasCOLVectorDistance2D(struct ScePspFVector2 pos1, struct ScePspFVector2 pos2) {
	return sqrtf((pos2.x-pos1.x)*(pos2.x-pos1.x) + (pos2.y-pos1.y)*(pos2.y-pos1.y));
}

int gaasCOLSphereToSphere(struct SphereCollision Sph1, struct SphereCollision Sph2) {
	//Calculate the distance between the centers of both spheres
	float distance = gaasCOLVectorDistance(Sph1.pos, Sph2.pos);

	if(distance < Sph1.radius + Sph2.radius) {
		return 1; //collision detected
	} else {
		return 0;
	}
}

int gaasCOLAABBToAABB(struct BoundingBox Box1, struct BoundingBox Box2) { 
	if ((Box1.min.x <= Box2.max.x && Box1.max.x >= Box2.min.x) &&
	    (Box1.min.y <= Box2.max.y && Box1.max.y >= Box2.min.y) && 
		(Box1.min.z <= Box2.max.z && Box1.max.z >= Box2.min.z))
	{
		return 1; //collision detected
	} else {
		return 0;
	}
}

//returns whether given point is on the left or right side of the line
// p1x, p1y - point coords
// p2x, p2y - start of line
// p3x, p3y - end of line
float gaasCOLSign(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) {
	float result;
    __asm__ volatile ( //I have no idea what I'm doing
		"mtv     %1, S000\n" //p1x
		"mtv     %2, S001\n" //p1y
		"mtv     %3, S002\n" //p2x
		"mtv     %4, S010\n" //p2y
		"mtv     %5, S011\n" //p3x
		"mtv     %6, S012\n" //p3y

		"vsub.s  S020, S011, S000\n" //1st
		"vsub.s  S021, S012, S010\n" //2nd
		"vmul.s  S022, S021, S020\n" //res

		"vsub.s  S030, S011, S002\n" //1st
		"vsub.s  S031, S012, S001\n" //2nd
		"vmul.s  S032, S031, S030\n" //res

		"vsub.s  S000, S022, S032\n" //final

		"mfv     %0, S000\n"
	: "=r"(result) : "r"(p1x), "r"(p1y), "r"(p2x), "r"(p2y), "r"(p3x), "r"(p3y));
    return result;
    //return ((p1x - p3x)*(p2y - p3y))-((p2x - p3x)*(p1y - p3y));
} 

int gaasCOLPointInPolygon(float ptx, float pty, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y) {
    if (gaasCOLSign(ptx, pty, v1x, v1y, v2x, v2y) <= 0 && gaasCOLSign(ptx, pty, v2x, v2y, v3x, v3y) <= 0 && gaasCOLSign(ptx, pty, v3x, v3y, v1x, v1y) <= 0) {
		return 1;
	} else {
		return 0;
	}
}

int gaasCOLPointInQuad(float ptx, float pty, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y, float v4x, float v4y) {
    if (gaasCOLSign(ptx, pty, v1x, v1y, v2x, v2y) <= 0 && gaasCOLSign(ptx, pty, v2x, v2y, v3x, v3y) <= 0 && gaasCOLSign(ptx, pty, v3x, v3y, v4x, v4y) <= 0 && gaasCOLSign(ptx, pty, v4x, v4y, v1x, v1y) <= 0) {
		return 1;
	} else {
		return 0;
	}
} 

float gaasCOLDeg2Rad(double Deg) {
	return Deg*GU_PI/180;
}

float gaasCOLRad2Deg(double Rad) {
	return Rad*180/GU_PI;
}

ScePspFVector3 gaasCOLCross(float u1, float u2, float u3, float v1, float v2, float v3) {
	float uvi = u2 * v3 - u3 * v2;
	float uvj = u3 * v1 - u1 * v3;
	float uvk = u1 * v2 - u2 * v1;

	ScePspFVector3 Nep = {uvi, uvj, uvk};

	return Nep;
}