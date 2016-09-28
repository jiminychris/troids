/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Christopher LaBauve $
   $Notice: $
   ======================================================================== */

inline b32
ProcessIntersection(r32 Intersection, r32 *tMove)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        if(0.0f <= Intersection && Intersection < *tMove)
        {
            *tMove = Intersection;
            Result = true;
        }
    }
    return(Result);
}

inline b32
ProcessIntersection(circle_ray_intersection_result Intersection, r32 *tMove)
{
    b32 Result = false;
    if(HasIntersection(Intersection))
    {
        r32 t = Minimum(Intersection.t1, Intersection.t2);
        if(0.0f <= t && t < *tMove)
        {
            *tMove = t;
            Result = true;
        }
    }
    return(Result);
}

inline void
ProcessIntersection(arc_polygon_edge_intersection_result Intersection, r32 *tSpin, collision *Collision)
{
    r32 t;
    if(HasIntersection(Intersection.t1))
    {
        t = Intersection.t1;
        if(0.0f <= t && t < *tSpin)
        {
            *tSpin = t;
            Collision->Type = CollisionType_Angular;
            Collision->PointOfCollision = Intersection.P1;
        }
    }
    if(HasIntersection(Intersection.t2))
    {
        t = Intersection.t2;
        if(0.0f <= t && t < *tSpin)
        {
            *tSpin = t;
            Collision->Type = CollisionType_Angular;
            Collision->PointOfCollision = Intersection.P2;
        }
    }
}

inline void
ProcessIntersection(arc_circle_intersection_result Intersection, r32 *tSpin, collision *Collision)
{
    if(HasIntersection(Intersection))
    {
        r32 t = Intersection.t1;
        v2 PointOfCollision = Intersection.P1;
        if(Intersection.t2 < Intersection.t1)
        {
            t = Intersection.t2;
            PointOfCollision = Intersection.P2;
        }
        if(0.0f <= t && t < *tSpin)
        {
            *tSpin = t;
            Collision->Type = CollisionType_Angular;
            Collision->PointOfCollision = PointOfCollision;
        }
    }
}

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

inline circle_ray_intersection_result
RawCircleRayIntersection(v2 P, r32 RadiusSq, v2 A, v2 B)
{
    circle_ray_intersection_result Result = {NAN, NAN};
    v2 RelativeA = A - P;
    v2 RelativeB = B - P;
    v2 dRay = B - A;

    r32 RayLengthSq = LengthSq(dRay);
    r32 D = RelativeA.x*RelativeB.y - RelativeB.x*RelativeA.y;

    r32 Discriminant = RadiusSq*RayLengthSq - Square(D);
    if(Discriminant >= 0)
    {
        r32 Root = SquareRoot(Discriminant);
        Result.t1 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) - Root) / RayLengthSq;
        Result.t2 = (-(RelativeA.x*dRay.x + RelativeA.y*dRay.y) + Root) / RayLengthSq;
    }
    return(Result);
}

// NOTE(chris): Returns the proportional time of intersection between A and B.
inline circle_ray_intersection_result
CircleRayIntersection(v2 P, r32 Radius, v2 A, v2 B)
{
    circle_ray_intersection_result Result = {NAN, NAN};
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
            Result = RawCircleRayIntersection(P, RadiusSq, A, B);
        }
    }
    return(Result);
}

// NOTE(chris): Assuming all polygons are counter-clockwise. i.e. EdgeA, EdgeB, InteriorPoint are
// all clockwise.
inline r32
PolygonEdgeRayIntersection(v2 EdgeA, v2 EdgeB, v2 RayA, v2 RayB, v2 InteriorPoint)
{
    r32 Result = NAN;
    v2 EdgeNormal = Perp(EdgeA-EdgeB);
    
    v2 dRay = RayB - RayA;
    v2 dSegment = EdgeB - EdgeA;
    if(Inner(EdgeNormal, dRay) < 0.0f)
    {
        if(Inner(-EdgeNormal, RayA-EdgeB) >= 0.0f &&
           Inner(Perp(InteriorPoint-EdgeB), RayA-EdgeB) >= 0.0f &&
           Inner(Perp(EdgeA-InteriorPoint), RayA-InteriorPoint) >= 0.0f)
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
        
                r32 XMin = Minimum(EdgeA.x, EdgeB.x) - COLLISION_EPSILON;
                r32 XMax = Maximum(EdgeA.x, EdgeB.x) + COLLISION_EPSILON;
                r32 YMin = Minimum(EdgeA.y, EdgeB.y) - COLLISION_EPSILON;
                r32 YMax = Maximum(EdgeA.y, EdgeB.y) + COLLISION_EPSILON;
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

// NOTE(chris): Assuming all polygons are counter-clockwise. i.e. EdgeA, EdgeB, InteriorPoint are
// all clockwise.
inline arc_polygon_edge_intersection_result
ArcPolygonEdgeIntersection(v2 ArcCenter, r32 Radius, v2 StartP, r32 dTheta,
                           v2 EdgeA, v2 EdgeB, v2 InteriorPoint)
{
    arc_polygon_edge_intersection_result Result = {NAN, NAN};
    v2 EdgeNormal = Perp(EdgeA-EdgeB);
    
    v2 dSegment = EdgeB - EdgeA;

    // TODO(chris): This lets through things that are already touching
    r32 EdgeTestA = Inner(-EdgeNormal, StartP-EdgeB);
    r32 EdgeTestB = Inner(Perp(InteriorPoint-EdgeB), StartP-EdgeB);
    r32 EdgeTestC = Inner(Perp(EdgeA-InteriorPoint), StartP-InteriorPoint);
    b32 Inside = (EdgeTestA >= -COLLISION_EPSILON &&
                  EdgeTestB >= -COLLISION_EPSILON &&
                  EdgeTestC >= -COLLISION_EPSILON);
    v2 RotationDirection = Sign(dTheta)*Perp(StartP-ArcCenter);

    // TODO(chris): Do two passes, where the first one cannot sweep more than pi radians.
    // Not super important unless I plan on having things spinning that fast.
    Assert(Square(dTheta) < Square(Pi));
    
    if(!Inside)
    {
        r32 RadiusSq = Square(Radius);
        circle_ray_intersection_result RayIntersection =
            RawCircleRayIntersection(ArcCenter, RadiusSq, EdgeA, EdgeB);

        if(HasIntersection(RayIntersection))
        {
            v2 RelativeA = EdgeA - ArcCenter;
            v2 RelativeB = EdgeB - ArcCenter;
            v2 dAB = EdgeB-EdgeA;
            r32 InverseRadiusSq = 1.0f / RadiusSq;
            v2 RelativeStartP = StartP - ArcCenter;
            v2 StartPerp = Perp(RelativeStartP);
            r32 InverseAbsdTheta = 1.0f / AbsoluteValue(dTheta);
            if(0 <= RayIntersection.t1 && RayIntersection.t1 <= 1)
            {
                v2 RelativeEndP = RelativeA + dAB*RayIntersection.t1;
                r32 ThetaIntersect = InverseCos(Inner(RelativeStartP, RelativeEndP)*InverseRadiusSq);
                r32 Direction = Inner(StartPerp, RelativeEndP);
                if(Direction*dTheta < 0)
                {
                    ThetaIntersect = Tau-ThetaIntersect;
                }
                Result.t1 = ThetaIntersect*InverseAbsdTheta;
                Result.P1 = ArcCenter + RelativeEndP;
            }
            if(0 <= RayIntersection.t2 && RayIntersection.t2 <= 1)
            {
                v2 RelativeEndP = RelativeA + dAB*RayIntersection.t2;
                r32 ThetaIntersect = InverseCos(Inner(RelativeStartP, RelativeEndP)*InverseRadiusSq);
                r32 Direction = Inner(StartPerp, RelativeEndP);
                if(Direction*dTheta < 0)
                {
                    ThetaIntersect = Tau-ThetaIntersect;
                }
                Result.t2 = ThetaIntersect*InverseAbsdTheta;
                Result.P2 = ArcCenter + RelativeEndP;
            }
        }
    }
    else if(Inner(RotationDirection, EdgeNormal) <= 0.0f)
    {
        Result.t1 = Result.t2 = 0.0f;
        Result.P1 = Result.P2 = StartP;
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
    
    v2 RotationDirection = Sign(dTheta)*Perp(StartP-ArcCenter);
    // TODO(chris): Do two passes, where the first one cannot sweep more than pi radians.
    // Not super important unless I plan on having things spinning that fast.
    Assert(Square(dTheta) < Square(Pi));

    v2 CircleCenterToStartP = StartP-CircleCenter;
    r32 CircleRadiusSq = Square(CircleRadius);
    b32 Inside = LengthSq(CircleCenterToStartP) <= CircleRadiusSq;

    if(!Inside)
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
                v2 RelativeEndP = CircleIntersection.P1 - ArcCenter;
                r32 Dot = Inner(RelativeStartP, RelativeEndP);
                r32 ThetaIntersect = InverseCos(Dot*InverseRadiusSq);
                r32 Direction = Inner(StartPerp, RelativeEndP);
                if(Direction*dTheta < 0)
                {
                    ThetaIntersect = Tau-ThetaIntersect;
                }
                Result.t1 = ThetaIntersect*InverseAbsdTheta;
                Result.P1 = CircleIntersection.P1;
            }
            {
                v2 RelativeEndP = CircleIntersection.P2 - ArcCenter;
                r32 Dot = Inner(RelativeStartP, RelativeEndP);
                r32 ThetaIntersect = InverseCos(Dot*InverseRadiusSq);
                r32 Direction = Inner(StartPerp, RelativeEndP);
                if(Direction*dTheta < 0)
                {
                    ThetaIntersect = Tau-ThetaIntersect;
                }
                Result.t2 = ThetaIntersect*InverseAbsdTheta;
                Result.P2 = CircleIntersection.P2;
            }
        }
    }
    else if(Inner(RotationDirection, CircleCenterToStartP) <= 0.0f)
    {
        Result.t1 = Result.t2 = 0.0f;
        Result.P1 = Result.P2 = StartP;
    }
    return(Result);
}

internal b32
ResolveCollision(entity *Entity, entity *OtherEntity)
{
    b32 Result = false;
    u32 Pair = (Entity->Type|OtherEntity->Type);
    switch(Pair)
    {
        case EntityPair_AsteroidBullet:
        {
            Entity->Destroyed = OtherEntity->Destroyed = true;
            Result = true;
        } break;

        case EntityPair_AsteroidShip:
        {
            Entity->Destroyed = OtherEntity->Destroyed = true;
            Result = true;
        } break;

        default:
        {
        }
    }

    return(Result);
}

internal void
ResolveLinearCollision(collision *Collision, entity *Entity, entity *OtherEntity)
{
    if(!ResolveCollision(Entity, OtherEntity))
    {
        r32 Efficiency = 0.1f;
        v3 DeflectionVector = {};
        switch(Collision->Type)
        {
            case CollisionType_Circle:
            {
                DeflectionVector = V3(Collision->Deflection, 0.0f);
            } break;

            case CollisionType_Line:
            {
                DeflectionVector = V3(Perp(Collision->A - Collision->B), 0.0f);
            } break;

            case CollisionType_None:
            {
            } break;

            InvalidDefaultCase;
        }

#if COLLISION_PHYSICAL
        // TODO(chris): Take into account point of collision and spin around center of mass
        r32 Mass = Entity->Mass;
        v3 dP = Project(Entity->dP, DeflectionVector);
        r32 OtherMass = OtherEntity->Mass;
        v3 OtherdP = Project(OtherEntity->dP, DeflectionVector);
        r32 InvMassSum = Efficiency/(Mass + OtherMass);
        r32 MassDifference = Mass - OtherMass;

        v3 NewdP = (dP*MassDifference + 2*OtherMass*OtherdP)*InvMassSum;
        v3 OtherNewdP = (-OtherdP*MassDifference + 2*Mass*dP)*InvMassSum;

        Entity->dP += NewdP - dP;
        OtherEntity->dP += OtherNewdP - OtherdP;
#else
        r32 DeflectionMagnitudeSq = LengthSq(DeflectionVector);
        if(DeflectionMagnitudeSq > 0.0f)
        {
            v2 Adjustment = (-DeflectionVector.xy *
                             (Inner(Entity->dP.xy, DeflectionVector.xy) /
                              DeflectionMagnitudeSq));
            Assert(!IsNaN(Adjustment.x));
            Assert(!IsNaN(Adjustment.y));
            Entity->dP += 1.1f*V3(Adjustment, 0);
        }
#endif
    }
}

internal void
ResolveAngularCollision(collision *Collision, entity *Entity, entity *OtherEntity)
{
    if(!ResolveCollision(Entity, OtherEntity))
    {
        r32 Efficiency = 0.4f;
        switch(Collision->Type)
        {
            case CollisionType_Angular:
            {
#if COLLISION_PHYSICAL
                v3 PointOfCollision = V3(Collision->PointOfCollision, 0.0f);
                v3 DeflectionVector = OtherEntity->P - PointOfCollision;
                v3 PerpRadius = V3(Perp((PointOfCollision-Entity->P).xy), 0.0f);
                v3 AngularVelocity = Entity->dYaw*PerpRadius;
                // TODO(chris): Take into account point of collision and spin around center of mass
                r32 Mass = Entity->Mass;
                r32 OtherMass = OtherEntity->Mass;
                v3 dP = Project(AngularVelocity, DeflectionVector);
                v3 OtherdP = Project(OtherEntity->dP, DeflectionVector);
                r32 InvMassSum = Efficiency/(Mass + OtherMass);
                r32 MassDifference = Mass - OtherMass;

                v3 NewdP = (dP*MassDifference + 2*OtherMass*OtherdP)*InvMassSum;
                v3 OtherNewdP = (-OtherdP*MassDifference + 2*Mass*dP)*InvMassSum;

                v3 NewAngularVelocity = AngularVelocity + NewdP - dP;
                Entity->dYaw = Length(NewAngularVelocity)/Length(PerpRadius);
                OtherEntity->dP += OtherNewdP - Project(OtherEntity->dP, DeflectionVector);
#else
                Entity->dYaw = -0.0001f*Entity->dYaw;
#endif
            } break;
        }
    }
}
