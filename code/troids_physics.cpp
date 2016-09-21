/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

inline b32
ProcessIntersection(r32 Intersection, r32 *tMove, r32 tMax, v2 dP)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
#if COLLISION_NORMALIZED_EPSILON_TEST
        r32 NormalizedEpsilonTest = LengthSq(Intersection*dP);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            Intersection = 0.0f;
        }
#endif
        if(0.0f <= Intersection && Intersection < tMax)
        {
            *tMove = Intersection;
            Result = true;
        }
    }
    return(Result);
}

inline b32
ProcessIntersection(circle_ray_intersection_result Intersection, r32 *tMove, r32 tMax, v2 dP)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        r32 t = Minimum(Intersection.t1, Intersection.t2);
#if COLLISION_NORMALIZED_EPSILON_TEST
        r32 NormalizedEpsilonTest = LengthSq(t*dP);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            t = 0.0f;
        }
#endif
        if(0.0f <= t && t < tMax)
        {
            *tMove = t;
            Result = true;
        }
    }
    return(Result);
}

inline b32
ProcessIntersection(arc_triangle_edge_intersection_result Intersection, r32 Radius,
                    r32 *tSpin, r32 tMax, r32 dSpin)
{
    b32 Result = false;
    r32 t;
    if(HasIntersection(Intersection.t1))
    {
        t = Intersection.t1;
#if COLLISION_NORMALIZED_EPSILON_TEST
        r32 NormalizedEpsilonTest = ArcLengthSq(Radius, t*dSpin);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            t = 0.0f;
        }
#endif
        if(0.0f <= t && t < tMax)
        {
            *tSpin = t;
            Result = true;
        }
    }
    if(HasIntersection(Intersection.t2))
    {
        t = Intersection.t2;
#if COLLISION_NORMALIZED_EPSILON_TEST
        r32 NormalizedEpsilonTest = ArcLengthSq(Radius, t*dSpin);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            t = 0.0f;
        }
#endif
        if(0.0f <= t && t < tMax)
        {
            *tSpin = t;
            Result = true;
        }
    }
    return(Result);
}

inline b32
ProcessIntersection(arc_circle_intersection_result Intersection, r32 Radius,
                    r32 *tSpin, r32 tMax, r32 dSpin)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        r32 t = Minimum(Intersection.t1, Intersection.t2);
#if COLLISION_NORMALIZED_EPSILON_TEST
        r32 NormalizedEpsilonTest = ArcLengthSq(Radius, t*dSpin);
        if(-COLLISION_EPSILON < NormalizedEpsilonTest && NormalizedEpsilonTest < COLLISION_EPSILON)
        {
            t = 0.0f;
        }
#endif
        if(0.0f <= t && t < tMax)
        {
            *tSpin = t;
            Result = true;
        }
    }
    return(Result);
}

#if TROIDS_INTERNAL
#define AssertPointOutsideCircle(Point, Center, RadiusSq)       \
    {                                                           \
        r32 OverlapDelta = LengthSq(Point - Center) - RadiusSq; \
        b32 Overlapping = OverlapDelta < -COLLISION_EPSILON;    \
        Assert(!Overlapping);                                   \
    }
#else
#define AssertPointOutsideCircle(...)
#endif

inline b32
BoundingCirclesIntersect(entity *Entity, entity *OtherEntity, v3 NewP)
{
    b32 Result = false;
    r32 HitRadius = Entity->BoundingRadius + OtherEntity->BoundingRadius;
    v2 HitP = OtherEntity->P.xy;
    v2 A = Entity->P.xy;
    v2 B = NewP.xy;
    
    v2 RelativeA = A - HitP;
    v2 RelativeB = B - HitP;
    v2 dRay = B - A;
    v2 RayToCenter = HitP-A;
    r32 RadiusSq = Square(HitRadius);
    if(LengthSq(RayToCenter) <= RadiusSq)
    {
        Result = true;
    }
    else
    {
        r32 RayLengthSq = LengthSq(dRay);
        r32 D = RelativeA.x*RelativeB.y - RelativeB.x*RelativeA.y;

        r32 Discriminant = RadiusSq*RayLengthSq - Square(D);
        if(Discriminant >= 0)
        {
            r32 Root = SquareRoot(Discriminant);
            r32 t1 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) - Root) / RayLengthSq;
            r32 t2 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) + Root) / RayLengthSq;
            Result = (0.0f <= t1 && t1 <= 1.0f) || (0.0f <= t2 && t2 <= 1.0f);
        }
    }
    return(Result);
}

// NOTE(chris): Returns the proportional time of intersection between A and B.
inline circle_ray_intersection_result
CircleRayIntersection(v2 P, r32 Radius, v2 A, v2 B)
{
    circle_ray_intersection_result Result = {NAN, NAN};
    v2 RelativeA = A - P;
    v2 RelativeB = B - P;
    v2 dRay = B - A;
    v2 RayToCenter = P-A;
    if(Inner(RayToCenter, dRay) > 0.0f)
    {
        r32 RadiusSq = Square(Radius);
        if(LengthSq(RayToCenter) <= RadiusSq)
        {
            Result.t1 = Result.t2 = 0.0f;
        }
        else
        {
            r32 RayLengthSq = LengthSq(dRay);
            r32 D = RelativeA.x*RelativeB.y - RelativeB.x*RelativeA.y;

            r32 Discriminant = RadiusSq*RayLengthSq - Square(D);
            if(Discriminant >= 0)
            {
                r32 Root = SquareRoot(Discriminant);
                Result.t1 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) - Root) / RayLengthSq;
                Result.t2 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) + Root) / RayLengthSq;
            }
        }
    }
    return(Result);
}

// NOTE(chris): Assuming all triangles are counter-clockwise. i.e. EdgeA, EdgeB, InteriorPoint are
// all clockwise.
inline r32
TriangleEdgeRayIntersection(v2 EdgeA, v2 EdgeB, v2 RayA, v2 RayB, v2 InteriorPoint)
{
    r32 Result = NAN;
    v2 EdgeNormal = Perp(EdgeA-EdgeB);
    
    v2 dRay = RayB - RayA;
    v2 dSegment = EdgeB - EdgeA;
    if(Inner(EdgeNormal, dRay) < 0.0f)
    {
        if(Inner(-EdgeNormal, RayA-EdgeB) > 0.0f &&
           Inner(Perp(InteriorPoint-EdgeB), RayA-EdgeB) > 0.0f &&
           Inner(Perp(EdgeA-InteriorPoint), RayA-InteriorPoint) > 0.0f)
        {
            Result = 0.0f;
        }
        else
        {
            r32 a = dSegment.y;
            r32 b = -dSegment.x;
            r32 Divisor = a*dRay.x + b*dRay.y;
            if(Divisor != 0)
            {
                r32 t = (dSegment.y*(EdgeA.x - RayA.x) + dSegment.x*(RayA.y - EdgeA.y)) / Divisor;

                r32 x = RayA.x + dRay.x*t;
                r32 y = RayA.y + dRay.y*t;
        
                r32 XMin = Minimum(EdgeA.x, EdgeB.x);
                r32 XMax = Maximum(EdgeA.x, EdgeB.x);
                r32 YMin = Minimum(EdgeA.y, EdgeB.y);
                r32 YMax = Maximum(EdgeA.y, EdgeB.y);
                if(XMin <= x && x <= XMax && YMin <= y && y <= YMax)
                {
                    Result = t;
                }
            }
        }
    }
    return(Result);
}

inline r32
ArcLength(r32 Radius, r32 dTheta)
{
    r32 Proportion = dTheta/Tau;
    r32 Circumference = Tau*Radius;
    r32 Result = Circumference*Proportion;
    return(Result);
}

inline r32
ArcLengthSq(r32 Radius, r32 dTheta)
{
    r32 Proportion = dTheta/Tau;
    r32 Circumference = Tau*Radius;
    r32 ArcLength = Circumference*Proportion;
    r32 Result = ArcLength*ArcLength;
    return(Result);
}

// NOTE(chris): Assuming all triangles are counter-clockwise. i.e. EdgeA, EdgeB, InteriorPoint are
// all clockwise.
inline arc_triangle_edge_intersection_result
ArcTriangleEdgeIntersection(v2 ArcCenter, r32 Radius, v2 StartP, r32 dTheta,
                            v2 EdgeA, v2 EdgeB, v2 InteriorPoint)
{
    arc_triangle_edge_intersection_result Result = {NAN, NAN};
    v2 EdgeNormal = Perp(EdgeA-EdgeB);
    
    v2 RotationDirection = Sign(dTheta)*Perp(StartP-ArcCenter);
    v2 dSegment = EdgeB - EdgeA;
    // TODO(chris): Do two passes, where the first one cannot sweep more than pi radians.
    // Not super important unless I plan on having things spinning that fast.
    Assert(Square(dTheta) < Square(Pi));
    if(Inner(EdgeNormal, RotationDirection) < 0.0f)
    {
        if(Inner(-EdgeNormal, StartP-EdgeB) > 0.0f &&
           Inner(Perp(InteriorPoint-EdgeB), StartP-EdgeB) > 0.0f &&
           Inner(Perp(EdgeA-InteriorPoint), StartP-InteriorPoint) > 0.0f)
        {
            Result.t1 = Result.t2 = 0.0f;
        }
        else
        {
            circle_ray_intersection_result RayIntersection = CircleRayIntersection(ArcCenter, Radius,
                                                                                   EdgeA, EdgeB);

            if(HasIntersection(RayIntersection))
            {
                v2 RelativeA = EdgeA - ArcCenter;
                v2 RelativeB = EdgeB - ArcCenter;
                v2 dAB = EdgeB-EdgeA;
                r32 InverseRadiusSq = 1.0f / (Radius*Radius);
                v2 RelativeStartP = StartP - ArcCenter;
                v2 StartPerp = Perp(RelativeStartP);
                r32 InverseAbsdTheta = 1.0f / AbsoluteValue(dTheta);
                if(0 <= RayIntersection.t1 && RayIntersection.t1 <= 1)
                {
                    v2 EndP = RelativeA + dAB*RayIntersection.t1;
                    r32 ThetaIntersect = InverseCos(Inner(RelativeStartP, EndP)*InverseRadiusSq);
                    r32 Direction = Inner(StartPerp, EndP);
                    if(Direction*dTheta < 0)
                    {
                        ThetaIntersect = Tau-ThetaIntersect;
                    }
                    Result.t1 = ThetaIntersect*InverseAbsdTheta;
                }
                if(0 <= RayIntersection.t2 && RayIntersection.t2 <= 1)
                {
                    v2 EndP = RelativeA + dAB*RayIntersection.t2;
                    r32 ThetaIntersect = InverseCos(Inner(RelativeStartP, EndP)*InverseRadiusSq);
                    r32 Direction = Inner(StartPerp, EndP);
                    if(Direction*dTheta < 0)
                    {
                        ThetaIntersect = Tau-ThetaIntersect;
                    }
                    Result.t2 = ThetaIntersect*InverseAbsdTheta;
                }
            }
        }
    }
    return(Result);
}

inline circle_circle_intersection_result
CircleCircleIntersection(v2 A, r32 RadiusA, v2 B, r32 RadiusB)
{
    circle_circle_intersection_result Result = {V2(NAN, NAN), V2(NAN, NAN)};
    v2 AB = B - A;
    r32 TransformAngle = -InverseTan(AB.y, AB.x);
    v2 RelativeB = RotateZ(AB, TransformAngle);
    Assert(RelativeB.x != 0);
    if(RelativeB.x - RadiusB <= RadiusA)
    {
        v2 P1, P2;
        P1.x = P2.x = (Square(RelativeB.x)-Square(RadiusB)+Square(RadiusA))*0.5f/RelativeB.x;
        P1.y = SquareRoot(Square(RadiusA)-Square(P1.x));
        P2.y = -P1.y;

        Result.P1 = RotateZ(P1, -TransformAngle) + A;
        Result.P2 = RotateZ(P2, -TransformAngle) + A;
    }
    
    return(Result);
}

inline arc_circle_intersection_result
ArcCircleIntersection(v2 ArcCenter, r32 ArcRadius, v2 StartP, r32 dTheta,
                      v2 CircleCenter, r32 CircleRadius)
{
    arc_circle_intersection_result Result = {NAN, NAN};
    
    v2 StartPToCircleCenter = CircleCenter - StartP;
    v2 RotationDirection = Sign(dTheta)*Perp(StartP-ArcCenter);
    // TODO(chris): Do two passes, where the first one cannot sweep more than pi radians.
    // Not super important unless I plan on having things spinning that fast.
    Assert(Square(dTheta) < Square(Pi));
    if(Inner(RotationDirection, StartPToCircleCenter) > 0.0f)
    {
        r32 CircleRadiusSq = Square(CircleRadius);
        if(LengthSq(StartPToCircleCenter) <= CircleRadiusSq)
        {
            Result.t1 = Result.t2 = 0.0f;
        }
        else
        {
            circle_circle_intersection_result CircleIntersection =
                CircleCircleIntersection(ArcCenter, ArcRadius, CircleCenter, CircleRadius);
    
            if(HasIntersection(CircleIntersection))
            {
                v2 RelativeStartP = StartP - ArcCenter;
                r32 InverseRadiusSq = 1.0f / Square(ArcRadius);
                r32 InverseAbsdTheta = 1.0f / AbsoluteValue(dTheta);
                v2 StartPerp = Perp(RelativeStartP);

                {
                    v2 EndP = CircleIntersection.P1 - ArcCenter;
                    r32 Dot = Inner(RelativeStartP, EndP);
                    r32 ThetaIntersect = InverseCos(Dot*InverseRadiusSq);
                    r32 Direction = Inner(StartPerp, EndP);
                    if(Direction*dTheta < 0)
                    {
                        ThetaIntersect = Tau-ThetaIntersect;
                    }
                    Result.t1 = ThetaIntersect*InverseAbsdTheta;
                }
                {
                    v2 EndP = CircleIntersection.P2 - ArcCenter;
                    r32 Dot = Inner(RelativeStartP, EndP);
                    r32 ThetaIntersect = InverseCos(Dot*InverseRadiusSq);
                    r32 Direction = Inner(StartPerp, EndP);
                    if(Direction*dTheta < 0)
                    {
                        ThetaIntersect = Tau-ThetaIntersect;
                    }
                    Result.t2 = ThetaIntersect*InverseAbsdTheta;
                }
            }
        }
    }
    return(Result);
}
